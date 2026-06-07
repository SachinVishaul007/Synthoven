#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include "Sample.h"

/**
 * Talks to the Chop Spring Boot backend (default http://localhost:8080).
 *
 * Network work runs on a small thread pool; result callbacks are always
 * delivered on the message thread, so callers can touch the UI directly.
 * Override the base URL with the CHOP_API_BASE environment variable.
 */
class ChopApiClient
{
public:
    ChopApiClient();
    ~ChopApiClient();

    struct SearchResult
    {
        juce::Array<Sample> libraryResults;
        juce::String        generationJobId;
        juce::String        status;
    };

    struct JobResult
    {
        juce::String        status;   // queued | generating | complete | failed
        juce::Array<Sample> samples;
        juce::String        error;
    };

    using SearchCallback  = std::function<void (SearchResult, juce::String error)>;
    using JobCallback     = std::function<void (JobResult, juce::String error)>;
    using SamplesCallback = std::function<void (juce::Array<Sample>, juce::String error)>;

    /** POST /api/search — library hits + a generation job id to poll. */
    void search (const juce::String& query, SearchCallback cb);

    /** GET /api/generation/{jobId}. */
    void getGenerationJob (const juce::String& jobId, JobCallback cb);

    /**
     * POST /api/generation then poll until the job finishes. Delivers the final
     * JobResult (complete/failed) on the message thread. One call = prompt in,
     * generated samples out. durationSeconds/cfgScale tune the local Stable Audio
     * provider; category is an optional hint prepended to the prompt.
     */
    void generate (const juce::String& prompt, double durationSeconds, double cfgScale,
                   const juce::String& category, JobCallback cb);

    /** GET an endpoint returning a SampleDto[] (e.g. "/api/library/samples/library"). */
    void getSamples (const juce::String& path, SamplesCallback cb);

    /** POST /api/library/upload (multipart) — returns the imported samples. */
    void uploadFiles (const juce::Array<juce::File>& files, SamplesCallback cb);

    /** Stream a sample to a temp file (async); callback on the message thread. */
    void downloadSample (int id, const juce::String& format,
                         std::function<void (juce::File, juce::String error)> cb);

    /** Blocking download to a temp file, cached by id. Safe to call from any thread. */
    juce::File downloadSampleSync (int id, const juce::String& format, juce::String& error);

    juce::String getBaseUrl() const { return baseUrl; }
    void setBaseUrl (const juce::String& url) { baseUrl = url; }

private:
    juce::String baseUrl;
    juce::ThreadPool pool { 2 };

    juce::CriticalSection cacheLock;
    juce::HashMap<int, juce::File> downloadCache;

    static juce::String mimeForFormat (const juce::String& format);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopApiClient)
};
