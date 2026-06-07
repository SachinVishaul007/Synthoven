#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "ChopApiClient.h"

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopAudioProcessor)
};
