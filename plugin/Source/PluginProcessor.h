#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <unordered_map>
#include "ChopApiClient.h"
#include "ClapModelManager.h"
#include "MelSpectrogramExtractor.h"

/**
 * Chop Sample Browser — a plugin that searches/generates/auditions samples from
 * the Chop backend and lets you drag them onto a DAW track. The audio engine is
 * a one-shot audition player: it streams a selected sample through the plugin's
 * output.
 */
class ChopAudioProcessor : public juce::AudioProcessor
{
public:
    ChopAudioProcessor();
    ~ChopAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Chop Sample Browser"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ── Audition control (call from the message thread) ────────────────────
    void auditionFile (const juce::File& file);
    void stopAudition();
    bool isAuditioning() const { return transport.isPlaying(); }

    ChopApiClient& getApiClient() { return apiClient; }

    juce::File getLibraryFolder() const { return libraryFolder; }
    void setLibraryFolder (const juce::File& folder);

    void triggerLibraryScan();
    void setScannedFiles (juce::Array<juce::File> files);
    juce::Array<juce::File> getScannedFiles() const;

    bool isLibraryScanning() const { return isScanning.load(); }
    int getLibraryScannedCount() const { return scannedCount.load(); }

    // ── Embedding cache & search ───────────────────────────────────────────
    bool isEmbeddingIndexing() const { return isEmbeddingScanning.load(); }
    int getEmbeddingIndexedCount() const { return embeddingIndexedCount.load(); }
    int getEmbeddingTotalCount() const { return embeddingTotalCount.load(); }

    ClapModelManager& getModelManager() { return modelManager; }
    bool isSemanticSearchEnabled() const { return semanticSearchEnabled.load(); }
    void setSemanticSearchEnabled (bool enabled) { semanticSearchEnabled.store (enabled); }

    void loadEmbeddingCache();
    void saveEmbeddingCache();
    juce::File getEmbeddingCacheFile() const;
    juce::String getRelativePath (const juce::File& file, const juce::File& root) const;
    juce::String normalizePath (const juce::String& path) const;

    juce::Array<juce::File> performSearch (const juce::String& query);

private:
    class LibraryScannerThread : public juce::Thread
    {
    public:
        LibraryScannerThread (ChopAudioProcessor& p)
            : juce::Thread ("LibraryScanner"), processor (p)
        {}

        ~LibraryScannerThread() override
        {
            stopThread (4000);
        }

        void startScan (const juce::File& folder)
        {
            stopThread (2000);
            rootFolder = folder;
            startThread();
        }

        void run() override
        {
            processor.isScanning = true;
            processor.scannedCount = 0;

            juce::Array<juce::File> files;
            scanDir (rootFolder, files);

            if (! threadShouldExit())
            {
                processor.setScannedFiles (std::move (files));
            }

            processor.isScanning = false;

            // Now run background embedding scan pass
            if (! threadShouldExit())
            {
                processor.isEmbeddingScanning = true;
                
                auto allFiles = processor.getScannedFiles();
                processor.embeddingTotalCount = allFiles.size();
                processor.embeddingIndexedCount = 0;

                int unsavedCount = 0;

                for (int i = 0; i < allFiles.size(); ++i)
                {
                    if (threadShouldExit())
                        break;

                    const auto& file = allFiles[i];
                    juce::String relPath = processor.getRelativePath (file, rootFolder);
                    juce::int64 modTime = file.getLastModificationTime().toMilliseconds();

                    bool needsEmbedding = false;
                    {
                        const juce::ScopedLock sl (processor.cacheLock);
                        auto it = processor.embeddingCache.find (relPath);
                        if (it == processor.embeddingCache.end() || it->second.lastModifiedTime != modTime)
                        {
                            needsEmbedding = true;
                        }
                    }

                    if (needsEmbedding)
                    {
                        // Sleep if model not loaded yet (e.g. still downloading)
                        while (! processor.modelManager.isModelLoaded() && ! threadShouldExit())
                        {
                            juce::Thread::sleep (500);
                        }

                        if (threadShouldExit())
                            break;

                        std::vector<float> spectrogram;
                        if (processor.extractor.extractFromFile (file, spectrogram))
                        {
                            std::vector<float> embedding;
                            if (processor.modelManager.getAudioEmbedding (spectrogram, embedding))
                            {
                                const juce::ScopedLock sl (processor.cacheLock);
                                processor.embeddingCache[relPath] = { modTime, std::move (embedding) };
                                unsavedCount++;
                            }
                        }
                    }

                    processor.embeddingIndexedCount = i + 1;

                    if (unsavedCount >= 20)
                    {
                        processor.saveEmbeddingCache();
                        unsavedCount = 0;
                    }
                }

                if (unsavedCount > 0)
                {
                    processor.saveEmbeddingCache();
                }

                processor.isEmbeddingScanning = false;
            }
        }

    private:
        void scanDir (const juce::File& dir, juce::Array<juce::File>& results)
        {
            if (threadShouldExit()) return;

            juce::Array<juce::File> subDirs;
            juce::Array<juce::File> localFiles;
            dir.findChildFiles (localFiles, juce::File::findFiles, false, "*.wav;*.mp3;*.aif;*.aiff;*.flac");

            for (auto& lf : localFiles)
            {
                results.add (lf);
                processor.scannedCount++;
            }

            dir.findChildFiles (subDirs, juce::File::findDirectories, false);

            for (auto& d : subDirs)
            {
                if (threadShouldExit()) return;
                scanDir (d, results);
            }
        }

        ChopAudioProcessor& processor;
        juce::File rootFolder;
    };

    ChopApiClient apiClient;

    juce::AudioFormatManager formatManager;
    juce::TimeSliceThread    readAheadThread { "chop-audio-read" };
    juce::AudioTransportSource transport;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    std::atomic<int> numSourceChannels { 0 };
    juce::File libraryFolder;

    std::atomic<bool> isScanning { false };
    std::atomic<int> scannedCount { 0 };

    juce::Array<juce::File> localLibraryFiles;
    juce::CriticalSection localFilesLock;
    LibraryScannerThread scannerThread { *this };

    // ── Embedding Caching & ML Search ──────────────────────────────────────
    ClapModelManager modelManager;
    MelSpectrogramExtractor extractor;

    struct CachedEmbedding
    {
        juce::int64 lastModifiedTime = 0;
        std::vector<float> embedding;
    };

    juce::CriticalSection cacheLock;
    std::unordered_map<juce::String, CachedEmbedding> embeddingCache;

    std::atomic<bool> isEmbeddingScanning { false };
    std::atomic<int> embeddingIndexedCount { 0 };
    std::atomic<int> embeddingTotalCount { 0 };
    std::atomic<bool> semanticSearchEnabled { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopAudioProcessor)
};
