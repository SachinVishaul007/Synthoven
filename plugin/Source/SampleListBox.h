#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include "Sample.h"

class ChopAudioProcessor;

/**
 * Scrollable list of samples. Clicking a row auditions it; dragging a row
 * performs an external file drag so the sample can be dropped onto a DAW track.
 */
class SampleListBox : public juce::Component,
                      public juce::ListBoxModel
{
public:
    explicit SampleListBox (ChopAudioProcessor& processor);

    void setSamples (juce::Array<Sample> newSamples);
    void appendSamples (const juce::Array<Sample>& extra);
    int  getNumSamples() const { return samples.size(); }

    /** Invoked (message thread) when a row is clicked. */
    std::function<void (const Sample&)> onAudition;
    /** Invoked with a status string (e.g. while preparing a drag). */
    std::function<void (const juce::String&)> onStatus;

    // ListBoxModel
    int  getNumRows() override;
    void paintListBoxItem (int, juce::Graphics&, int, int, bool) override {}
    juce::Component* refreshComponentForRow (int row, bool selected, juce::Component* existing) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    void resized() override;

    // Called by row components.
    void rowClicked (int row);
    void beginExternalDragForRow (int row);

private:
    ChopAudioProcessor& processor;
    juce::ListBox listBox { "samples", this };
    juce::Array<Sample> samples;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleListBox)
};
