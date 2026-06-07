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
