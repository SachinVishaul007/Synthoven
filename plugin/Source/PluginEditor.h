#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "SampleListBox.h"

class ChopAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    explicit ChopAudioProcessorEditor (ChopAudioProcessor&);
    ~ChopAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void doSearch();
    void doImport();
    void loadSection (const juce::String& path, const juce::String& label);
    void startPolling (const juce::String& jobId);
    void timerCallback() override;
    void setStatus (const juce::String& text);

    ChopAudioProcessor& processorRef;

    juce::TextEditor  searchBox;
    juce::TextButton  searchButton  { "Search" };
    juce::TextButton  importButton  { "Import" };
    juce::TextButton  stopButton    { "Stop" };

    juce::TextButton  libraryButton   { "Library" };
    juce::TextButton  favoritesButton { "Favorites" };
    juce::TextButton  generatedButton { "Generated" };
    juce::TextButton  recentButton    { "Recent" };

    juce::Label       titleLabel;
    juce::Label       statusLabel;

    std::unique_ptr<SampleListBox>   list;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::String pollingJobId;
    int pollAttempts = 0;
    static constexpr int maxPollAttempts = 40; // ~80s at 2s intervals

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopAudioProcessorEditor)
};
