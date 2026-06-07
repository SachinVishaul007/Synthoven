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

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    // ── Audition control (call from the message thread) ────────────────────
    void auditionFile (const juce::File& file);
    void stopAudition();
    bool isAuditioning() const { return transport.isPlaying(); }

    ChopApiClient& getApiClient() { return apiClient; }

private:
    ChopApiClient apiClient;

    juce::AudioFormatManager formatManager;
    juce::TimeSliceThread    readAheadThread { "chop-audio-read" };
    juce::AudioTransportSource transport;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    std::atomic<int> numSourceChannels { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopAudioProcessor)
};
