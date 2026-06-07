#include "SampleListBox.h"
#include "PluginProcessor.h"

namespace
{
    // One row: paints sample info and turns a mouse-drag into an external file drag.
    class SampleRowComponent : public juce::Component
    {
    public:
        explicit SampleRowComponent (SampleListBox& o) : owner (o) {}

        void set (int newRow, const Sample& s, bool isSelected)
        {
            row = newRow;
            sample = s;
            selected = isSelected;
            
            setTitle (s.name);
            juce::String desc = s.subtitle();
            if (s.bpm > 0) desc << ", " << juce::String (s.bpm) << " BPM";
            desc << ", duration " << s.durationText();
            if (isSelected) desc << ", selected";
            setDescription (desc);
            setAccessible (true);
            
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().reduced (8, 4);

            if (selected)
            {
                g.setColour (juce::Colour (0xff2a2a30));
                g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (3.0f), 4.0f);
            }

            const auto accent = sample.isGenerated() ? juce::Colour (0xff6c8fff)
                                                      : juce::Colour (0xffe0a458);

            // Type dot
            g.setColour (accent);
            g.fillEllipse ((float) bounds.getX(), (float) bounds.getCentreY() - 3.0f, 6.0f, 6.0f);
            bounds.removeFromLeft (16);

            auto rightCol = bounds.removeFromRight (90);

            // Name + subtitle
            auto nameArea = bounds.removeFromTop (bounds.getHeight() / 2);
            g.setColour (juce::Colours::white.withAlpha (0.92f));
            g.setFont (juce::Font (juce::FontOptions (13.0f)));
            g.drawText (sample.name, nameArea, juce::Justification::centredLeft, true);

            g.setColour (juce::Colours::white.withAlpha (0.45f));
            g.setFont (juce::Font (juce::FontOptions (11.0f)));
            g.drawText (sample.subtitle(), bounds, juce::Justification::centredLeft, true);

            // Duration + bpm
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.setFont (juce::Font (juce::FontOptions (11.0f)));
            juce::String meta = sample.durationText();
            if (sample.bpm > 0) meta << "  " << sample.bpm << " bpm";
            g.drawText (meta, rightCol, juce::Justification::centredRight, false);
        }

        void mouseDown (const juce::MouseEvent&) override
        {
            dragging = false;
            owner.rowClicked (row);
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (! dragging && e.getDistanceFromDragStart() > 8)
            {
                dragging = true;
                owner.beginExternalDragForRow (row);
            }
        }

    private:
        SampleListBox& owner;
        Sample sample;
        int  row = -1;
        bool selected = false;
        bool dragging = false;
    };
}

SampleListBox::SampleListBox (ChopAudioProcessor& p) : processor (p)
{
    listBox.setRowHeight (44);
    listBox.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff161619));
    addAndMakeVisible (listBox);
}

void SampleListBox::setSamples (juce::Array<Sample> newSamples)
{
    samples = std::move (newSamples);
    listBox.updateContent();
    listBox.deselectAllRows();
    repaint();
}

void SampleListBox::appendSamples (const juce::Array<Sample>& extra)
{
    samples.addArray (extra);
    listBox.updateContent();
    repaint();
}

int SampleListBox::getNumRows() { return samples.size(); }

juce::Component* SampleListBox::refreshComponentForRow (int row, bool selected, juce::Component* existing)
{
    auto* comp = dynamic_cast<SampleRowComponent*> (existing);

    if (! juce::isPositiveAndBelow (row, samples.size()))
    {
        delete existing;          // row no longer exists
        return nullptr;
    }

    if (comp == nullptr)
    {
        delete existing;
        comp = new SampleRowComponent (*this);
    }

    comp->set (row, samples.getReference (row), selected);
    return comp;
}

void SampleListBox::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    // Row components handle clicks; this is a fallback for keyboard/host paths.
    rowClicked (row);
}

void SampleListBox::rowClicked (int row)
{
    if (! juce::isPositiveAndBelow (row, samples.size()))
        return;

    listBox.selectRow (row);
    if (onAudition)
        onAudition (samples.getReference (row));
}

void SampleListBox::beginExternalDragForRow (int row)
{
    if (! juce::isPositiveAndBelow (row, samples.size()))
        return;

    const auto sample = samples.getReference (row);

    if (sample.localFilePath.isNotEmpty())
    {
        if (onStatus) onStatus ("Dragging \"" + sample.name + "\"");
        juce::StringArray paths;
        paths.add (sample.localFilePath);
        juce::DragAndDropContainer::performExternalDragDropOfFiles (
            paths, /*canMoveFiles*/ false, this);
        return;
    }

    if (onStatus) onStatus ("Preparing \"" + sample.name + "\" for drag…");

    // The external drag must start during this gesture, so we fetch the file
    // synchronously. On localhost this is near-instant, and repeat drags hit the
    // download cache.
    juce::String error;
    auto file = processor.getApiClient().downloadSampleSync (sample.id, sample.format, error);

    if (file.existsAsFile())
    {
        if (onStatus) onStatus ("Dragging \"" + sample.name + "\"");
        juce::StringArray paths;
        paths.add (file.getFullPathName());
        juce::DragAndDropContainer::performExternalDragDropOfFiles (
            paths, /*canMoveFiles*/ false, this);
    }
    else if (onStatus)
    {
        onStatus (error.isNotEmpty() ? error : "Could not prepare file for drag");
    }
}

void SampleListBox::resized()
{
    listBox.setBounds (getLocalBounds());
}
