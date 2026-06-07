#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class ChopAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 public juce::DragAndDropContainer
{
public:
    explicit ChopAudioProcessorEditor (ChopAudioProcessor&);
    ~ChopAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void selectLibraryFolder();
    void setLibraryFolder (const juce::File& folder);

    ChopAudioProcessor& processorRef;

    std::unique_ptr<juce::WebBrowserComponent> webView;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopAudioProcessorEditor)
};
