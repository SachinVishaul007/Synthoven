#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include "BpeTokenizer.h"

class ClapModelManager
{
public:
    ClapModelManager();
    ~ClapModelManager();

    /**
     * Checks if all model files are present. If not, starts the background download thread.
     * If they are present, attempts to load them.
     */
    void initialize();

    bool isModelLoaded() const { return modelsLoaded.load(); }
    bool isDownloading() const { return downloadInProgress.load(); }
    float getDownloadProgress() const { return downloadProgress.load(); }
    juce::String getStatusMessage() const;

    /**
     * Extracts a 512-dimension embedding vector from a log-mel spectrogram (1001 frames x 64 bins).
     */
    bool getAudioEmbedding(const std::vector<float>& spectrogram, std::vector<float>& outEmbedding);

    /**
     * Extracts a 512-dimension embedding vector from a text query.
     */
    bool getTextEmbedding(const juce::String& text, std::vector<float>& outEmbedding);

    /**
     * Computes the cosine similarity between two 512-dimensional vectors.
     */
    float computeSimilarity(const std::vector<float>& vecA, const std::vector<float>& vecB) const;

private:
    juce::File getModelsDirectory() const;
    bool loadModels();

    // Helper to query input/output names from ONNX session
    void querySessionTensorNames(const Ort::Session& session,
                                 std::vector<std::string>& inputNames,
                                 std::vector<std::string>& outputNames);

    BpeTokenizer tokenizer;

    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> textSession;
    std::unique_ptr<Ort::Session> audioSession;

    // Session tensor name storage
    std::vector<std::string> textInputNames;
    std::vector<std::string> textOutputNames;
    std::vector<std::string> audioInputNames;
    std::vector<std::string> audioOutputNames;

    std::atomic<bool> modelsLoaded { false };
    std::atomic<bool> downloadInProgress { false };
    std::atomic<float> downloadProgress { 0.0f };
    
    juce::CriticalSection statusLock;
    juce::String statusMessage;
    void setStatus(const juce::String& message, float progress);

    class ModelDownloadThread : public juce::Thread
    {
    public:
        ModelDownloadThread(ClapModelManager& manager)
            : juce::Thread("ModelDownloadThread"), owner(manager)
        {}

        void run() override;

    private:
        ClapModelManager& owner;
    };

    std::unique_ptr<ModelDownloadThread> downloadThread;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClapModelManager)
};
