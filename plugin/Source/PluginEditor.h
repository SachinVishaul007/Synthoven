#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Library/SampleLibrary.h"
#include "Bridge/JSBridge.h"

class ChopEditor : public juce::AudioProcessorEditor
{
public:
    explicit ChopEditor(ChopProcessor&);
    ~ChopEditor() override;

    void resized() override;

private:
    ChopProcessor& processor;
    SampleLibrary  library;
    JSBridge       bridge;

    juce::WebBrowserComponent webView;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void handleBrowseForFiles(juce::WebBrowserComponent::NativeFunctionCompletion);
    void handleSelectLibraryFolder(juce::WebBrowserComponent::NativeFunctionCompletion);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopEditor)
};
