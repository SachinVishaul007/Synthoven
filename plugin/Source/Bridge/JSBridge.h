#pragma once
#include <JuceHeader.h>
#include "../Library/SampleLibrary.h"
#include "../Library/SamplePlayer.h"

class JSBridge
{
public:
    JSBridge(SampleLibrary& library, SamplePlayer& player);

    // Entry point — routes action strings to handlers
    juce::var handleMessage(const juce::String& action, const juce::var& payload);

private:
    SampleLibrary& library;
    SamplePlayer&  player;

    juce::var handleGetSamples       (const juce::var& payload);
    juce::var handleSearch           (const juce::var& payload);
    juce::var handleAddSample        (const juce::var& payload);
    juce::var handleDeleteSample     (const juce::var& payload);
    juce::var handleUpdateSample     (const juce::var& payload);
    juce::var handleUpload           (const juce::var& payload);
    juce::var handlePlay             (const juce::var& payload);
    juce::var handleStop             (const juce::var& payload);
    juce::var handleToggleFavorite   (const juce::var& payload);
    juce::var handleGetPlaybackState (const juce::var& payload);

    static juce::var ok(const juce::var& data = {});
    static juce::var err(const juce::String& message);
};
