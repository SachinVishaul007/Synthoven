#include "ChopApiClient.h"
#include <juce_events/juce_events.h> // MessageManager::callAsync

namespace
{
    // Run a blocking network task on the pool, deliver the result on the message thread.
    void runAsync (juce::ThreadPool& pool, std::function<void()> task)
    {
        pool.addJob (std::move (task));
    }

    std::unique_ptr<juce::InputStream> openGet (const juce::URL& url, int timeoutMs = 10000)
    {
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs (timeoutMs)
                           .withNumRedirectsToFollow (5);
        return url.createInputStream (options);
    }
}

ChopApiClient::ChopApiClient()
{
    baseUrl = juce::SystemStats::getEnvironmentVariable ("CHOP_API_BASE", "http://localhost:8080");
    // Normalise: strip any trailing slash.
    while (baseUrl.endsWithChar ('/'))
        baseUrl = baseUrl.dropLastCharacters (1);
}

ChopApiClient::~ChopApiClient()
{
    pool.removeAllJobs (true, 4000);
}

juce::String ChopApiClient::mimeForFormat (const juce::String& format)
{
    auto f = format.toLowerCase();
    if (f == "wav")               return "audio/wav";
    if (f == "mp3")               return "audio/mpeg";
    if (f == "aiff" || f == "aif") return "audio/aiff";
    if (f == "flac")              return "audio/flac";
    return "application/octet-stream";
}

void ChopApiClient::search (const juce::String& query, SearchCallback cb)
{
    auto base = baseUrl;
    runAsync (pool, [base, query, cb]
    {
        juce::String error;
        SearchResult result;

        auto* body = new juce::DynamicObject();
        body->setProperty ("query", query);
        body->setProperty ("generateMissing", true);
        auto json = juce::JSON::toString (juce::var (body));

        auto url = juce::URL (base + "/api/search").withPOSTData (json);
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                           .withConnectionTimeoutMs (15000)
                           .withExtraHeaders ("Content-Type: application/json");

        if (auto stream = url.createInputStream (options))
        {
            auto parsed = juce::JSON::parse (stream->readEntireStreamAsString());
            if (auto* o = parsed.getDynamicObject())
            {
                result.libraryResults  = Sample::arrayFromVar (o->getProperty ("libraryResults"));
                result.generationJobId = o->getProperty ("generationJobId").toString();
                result.status          = o->getProperty ("status").toString();
            }
            else
            {
                error = "Unexpected search response";
            }
        }
        else
        {
            error = "Could not reach backend at " + base;
        }

        juce::MessageManager::callAsync ([cb, result, error] { cb (result, error); });
    });
}

void ChopApiClient::getGenerationJob (const juce::String& jobId, JobCallback cb)
{
    auto base = baseUrl;
    runAsync (pool, [base, jobId, cb]
    {
        juce::String error;
        JobResult result;

        auto url = juce::URL (base + "/api/generation/" + juce::URL::addEscapeChars (jobId, false));
        if (auto stream = openGet (url))
        {
            auto parsed = juce::JSON::parse (stream->readEntireStreamAsString());
            if (auto* o = parsed.getDynamicObject())
            {
                result.status  = o->getProperty ("status").toString();
                result.error   = o->getProperty ("error").toString();
                result.samples = Sample::arrayFromVar (o->getProperty ("samples"));
            }
            else
            {
                error = "Unexpected generation response";
            }
        }
        else
        {
            error = "Could not poll generation job";
        }

        juce::MessageManager::callAsync ([cb, result, error] { cb (result, error); });
    });
}

void ChopApiClient::generate (const juce::String& prompt, double durationSeconds, double cfgScale,
                              const juce::String& category, JobCallback cb)
{
    auto base = baseUrl;
    runAsync (pool, [base, prompt, durationSeconds, cfgScale, category, cb]
    {
        juce::String error;
        JobResult result;

        // 1) Start the job.
        auto* body = new juce::DynamicObject();
        body->setProperty ("prompt", prompt);
        body->setProperty ("count", 1); // one per job; the editor loops for variations
        if (durationSeconds > 0.0) body->setProperty ("durationSeconds", durationSeconds);
        if (cfgScale > 0.0)        body->setProperty ("cfgScale", cfgScale);
        if (category.isNotEmpty()) body->setProperty ("category", category);
        auto json = juce::JSON::toString (juce::var (body));

        auto startUrl = juce::URL (base + "/api/generation").withPOSTData (json);
        auto startOpts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                             .withConnectionTimeoutMs (15000)
                             .withExtraHeaders ("Content-Type: application/json");

        juce::String jobId;
        if (auto stream = startUrl.createInputStream (startOpts))
        {
            auto parsed = juce::JSON::parse (stream->readEntireStreamAsString());
            if (auto* o = parsed.getDynamicObject())
                jobId = o->getProperty ("jobId").toString();
        }

        if (jobId.isEmpty())
        {
            error = "Could not start generation at " + base;
            juce::MessageManager::callAsync ([cb, result, error] { cb (result, error); });
            return;
        }

        // 2) Poll until the job reaches a terminal state.
        const auto jobUrl = juce::URL (base + "/api/generation/"
                                       + juce::URL::addEscapeChars (jobId, false));
        for (int attempt = 0; attempt < 90; ++attempt)
        {
            juce::Thread::sleep (1500);

            if (auto stream = openGet (jobUrl))
            {
                auto parsed = juce::JSON::parse (stream->readEntireStreamAsString());
                if (auto* o = parsed.getDynamicObject())
                {
                    result.status  = o->getProperty ("status").toString();
                    result.error   = o->getProperty ("error").toString();
                    result.samples = Sample::arrayFromVar (o->getProperty ("samples"));

                    if (result.status == "complete" || result.status == "failed")
                    {
                        if (result.status == "failed")
                            error = result.error.isNotEmpty() ? result.error : "Generation failed";
                        break;
                    }
                }
            }

            if (attempt == 89)
                error = "Generation timed out";
        }

        juce::MessageManager::callAsync ([cb, result, error] { cb (result, error); });
    });
}

void ChopApiClient::generateAudioToAudio (const juce::String& prompt, double durationSeconds, double cfgScale,
                                         const juce::String& category, const juce::File& initAudioFile, JobCallback cb)
{
    auto base = baseUrl;
    runAsync (pool, [base, prompt, durationSeconds, cfgScale, category, initAudioFile, cb]
    {
        juce::String error;
        JobResult result;

        // 1) Start the job.
        auto startUrl = juce::URL (base + "/api/generation/audio")
                            .withParameter ("prompt", prompt)
                            .withFileToUpload ("audio", initAudioFile, "audio/wav");

        if (durationSeconds > 0.0)
            startUrl = startUrl.withParameter ("durationSeconds", juce::String (durationSeconds, 1));
        if (cfgScale > 0.0)
            startUrl = startUrl.withParameter ("cfgScale", juce::String (cfgScale, 1));
        if (category.isNotEmpty())
            startUrl = startUrl.withParameter ("category", category);

        auto startOpts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                             .withConnectionTimeoutMs (30000);

        juce::String jobId;
        if (auto stream = startUrl.createInputStream (startOpts))
        {
            auto parsed = juce::JSON::parse (stream->readEntireStreamAsString());
            if (auto* o = parsed.getDynamicObject())
                jobId = o->getProperty ("jobId").toString();
        }

        if (jobId.isEmpty())
        {
            error = "Could not start audio-to-audio generation at " + base;
            juce::MessageManager::callAsync ([cb, result, error] { cb (result, error); });
            return;
        }

        // 2) Poll until the job reaches a terminal state.
        const auto jobUrl = juce::URL (base + "/api/generation/"
                                       + juce::URL::addEscapeChars (jobId, false));
        for (int attempt = 0; attempt < 90; ++attempt)
        {
            juce::Thread::sleep (1500);

            if (auto stream = openGet (jobUrl))
            {
                auto parsed = juce::JSON::parse (stream->readEntireStreamAsString());
                if (auto* o = parsed.getDynamicObject())
                {
                    result.status  = o->getProperty ("status").toString();
                    result.error   = o->getProperty ("error").toString();
                    result.samples = Sample::arrayFromVar (o->getProperty ("samples"));

                    if (result.status == "complete" || result.status == "failed")
                    {
                        if (result.status == "failed")
                            error = result.error.isNotEmpty() ? result.error : "Generation failed";
                        break;
                    }
                }
            }

            if (attempt == 89)
                error = "Generation timed out";
        }

        juce::MessageManager::callAsync ([cb, result, error] { cb (result, error); });
    });
}

void ChopApiClient::getSamples (const juce::String& path, SamplesCallback cb)
{
    auto base = baseUrl;
    runAsync (pool, [base, path, cb]
    {
        juce::String error;
        juce::Array<Sample> samples;

        auto url = juce::URL (base + path);
        if (auto stream = openGet (url))
            samples = Sample::arrayFromVar (juce::JSON::parse (stream->readEntireStreamAsString()));
        else
            error = "Could not load " + path;

        juce::MessageManager::callAsync ([cb, samples, error] { cb (samples, error); });
    });
}

void ChopApiClient::uploadFiles (const juce::Array<juce::File>& files, SamplesCallback cb)
{
    auto base = baseUrl;
    runAsync (pool, [base, files, cb]
    {
        juce::String error;
        juce::Array<Sample> imported;

        auto url = juce::URL (base + "/api/library/upload");
        for (auto& f : files)
            url = url.withFileToUpload ("files", f, mimeForFormat (f.getFileExtension().removeCharacters (".")));

        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                           .withConnectionTimeoutMs (120000);

        if (auto stream = url.createInputStream (options))
            imported = Sample::arrayFromVar (juce::JSON::parse (stream->readEntireStreamAsString()));
        else
            error = "Upload failed";

        juce::MessageManager::callAsync ([cb, imported, error] { cb (imported, error); });
    });
}

juce::File ChopApiClient::downloadSampleSync (int id, const juce::String& format, juce::String& error)
{
    {
        const juce::ScopedLock sl (cacheLock);
        if (downloadCache.contains (id))
        {
            auto cached = downloadCache[id];
            if (cached.existsAsFile())
                return cached;
        }
    }

    auto ext = format.isNotEmpty() ? format : juce::String ("wav");
    auto dest = juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("chop_sample_" + juce::String (id) + "." + ext);

    auto url = juce::URL (baseUrl + "/api/player/stream/" + juce::String (id));
    if (auto stream = openGet (url, 20000))
    {
        dest.deleteFile();
        if (auto out = std::unique_ptr<juce::FileOutputStream> (dest.createOutputStream()))
        {
            out->writeFromInputStream (*stream, -1);
            out->flush();
            out.reset();

            const juce::ScopedLock sl (cacheLock);
            downloadCache.set (id, dest);
            return dest;
        }
        error = "Could not write temp file";
    }
    else
    {
        error = "Could not stream sample " + juce::String (id)
              + " (generated/mock samples may have no playable file)";
    }
    return {};
}

void ChopApiClient::downloadSample (int id, const juce::String& format,
                                    std::function<void (juce::File, juce::String error)> cb)
{
    runAsync (pool, [this, id, format, cb]
    {
        juce::String error;
        auto file = downloadSampleSync (id, format, error);
        juce::MessageManager::callAsync ([cb, file, error] { cb (file, error); });
    });
}
