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

            // Right column layout: duration/bpm metadata + mini-waveform + heart button
            auto rightSide = bounds.removeFromRight (180);
            
            // Heart button
            auto heartBox = rightSide.removeFromRight (30);
            
            // Mini-waveform area
            juce::Rectangle<int> miniWaveformRect (rightSide.getRight() - 70, bounds.getY() + (bounds.getHeight() - 20) / 2, 70, 20);
            rightSide.removeFromRight (70);
            
            // Remaining portion of rightSide (80px) is for duration/bpm metadata
            auto rowMetaArea = rightSide;

            // Name area (top half) and Tags area (bottom half)
            auto nameArea = bounds.removeFromTop (24);
            auto tagsArea = bounds; // remaining 24px high area

            // Draw Name
            g.setColour (juce::Colours::white.withAlpha (0.92f));
            g.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
            g.drawText (sample.name, nameArea, juce::Justification::centredLeft, true);

            // Prepare tag chips
            juce::StringArray tagsToDraw;
            for (auto& t : sample.tags)
            {
                if (t.isNotEmpty())
                    tagsToDraw.add (t.toLowerCase());
            }

            if (tagsToDraw.isEmpty())
            {
                juce::StringArray words;
                words.addTokens (sample.name, " -_", "");
                for (auto& w : words)
                {
                    if (w.equalsIgnoreCase ("kick") || w.equalsIgnoreCase ("bass") || 
                        w.equalsIgnoreCase ("snare") || w.equalsIgnoreCase ("hat") || 
                        w.equalsIgnoreCase ("clap") || w.equalsIgnoreCase ("synth") || 
                        w.equalsIgnoreCase ("loop") || w.equalsIgnoreCase ("vocal") || 
                        w.equalsIgnoreCase ("vox") || w.equalsIgnoreCase ("perc") || 
                        w.equalsIgnoreCase ("lead") || w.equalsIgnoreCase ("pad") ||
                        w.equalsIgnoreCase ("fx") || w.equalsIgnoreCase ("drum") ||
                        w.equalsIgnoreCase ("kickdrum") || w.equalsIgnoreCase ("snaredrum"))
                    {
                        tagsToDraw.add (w.toLowerCase());
                    }
                }
                if (tagsToDraw.isEmpty())
                {
                    if (sample.isGenerated())
                        tagsToDraw.add ("ai-gen");
                    else
                        tagsToDraw.add ("sample");
                }
            }

            // Draw tag chips
            float tagX = (float)tagsArea.getX();
            float tagY = (float)tagsArea.getY() + (tagsArea.getHeight() - 16.0f) / 2.0f;
            
            g.setFont (juce::Font (juce::FontOptions (10.0f)));
            for (const auto& tag : tagsToDraw)
            {
                if (tag.isEmpty()) continue;
                
                float textWidth = (float) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), tag);
                float chipWidth = textWidth + 12.0f; // 6px padding on each side
                
                // Draw rounded rectangle background
                g.setColour (juce::Colours::white.withAlpha (0.08f));
                g.fillRoundedRectangle (tagX, tagY, chipWidth, 16.0f, 4.0f);
                
                // Draw border
                g.setColour (juce::Colours::white.withAlpha (0.15f));
                g.drawRoundedRectangle (tagX, tagY, chipWidth, 16.0f, 4.0f, 1.0f);
                
                // Draw text
                g.setColour (juce::Colours::white.withAlpha (0.8f));
                g.drawText (tag, (int)tagX, (int)tagY, (int)chipWidth, 16, juce::Justification::centred, false);
                
                tagX += chipWidth + 6.0f; // 6px gap
                
                if (tagX > (float)nameArea.getRight() - 10.0f)
                    break;
            }

            // Draw Duration + BPM metadata
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.setFont (juce::Font (juce::FontOptions (11.0f)));
            juce::String meta = sample.durationText();
            if (sample.bpm > 0) meta << "\n" << sample.bpm << " bpm"; // Stack or keep compact
            g.drawText (meta, rowMetaArea, juce::Justification::centredRight, false);

            // Draw deterministic mini-waveform preview
            g.setColour (accent.withAlpha (0.5f));
            size_t hash = std::hash<std::string>{}(sample.name.toStdString());
            const int numBars = 16;
            float barWidth = (float)miniWaveformRect.getWidth() / numBars;
            float maxBarHeight = (float)miniWaveformRect.getHeight();
            
            for (int i = 0; i < numBars; ++i)
            {
                unsigned int seed = (unsigned int)(hash ^ i);
                seed = (seed ^ 61) ^ (seed >> 16);
                seed *= 9;
                seed = seed ^ (seed >> 4);
                seed *= 0x27d4eb2d;
                seed = seed ^ (seed >> 15);
                float val = (float)(seed % 100) / 100.0f;
                
                float envelope = 1.0f;
                float progress = (float)i / (float)(numBars - 1);
                if (progress < 0.2f)
                    envelope = progress / 0.2f;
                else
                    envelope = 1.0f - (progress - 0.2f) / 0.8f;
                
                float h = val * maxBarHeight * envelope;
                h = juce::jmax (2.0f, h);
                
                float x = (float)miniWaveformRect.getX() + i * barWidth + (barWidth - 1.5f) / 2.0f;
                float y = (float)miniWaveformRect.getY() + (maxBarHeight - h) / 2.0f;
                
                g.fillRoundedRectangle (x, y, 1.5f, h, 0.75f);
            }

            // Draw Heart (Favorite)
            if (sample.localFilePath.isNotEmpty())
            {
                heartArea = heartBox;
                bool isFav = owner.getProcessor().isFavorite (sample.localFilePath);
                g.setColour (isFav ? juce::Colours::red : juce::Colours::white.withAlpha(0.3f));
                g.setFont (juce::Font (juce::FontOptions (22.0f)));
                juce::String heartStr = juce::String::fromUTF8 (isFav ? "\xe2\x99\xa5" : "\xe2\x99\xa1");
                g.drawText (heartStr, heartArea, juce::Justification::centred, false);
            }
            else
            {
                heartArea = {};
            }
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            dragging = false;
            if (! heartArea.isEmpty() && heartArea.contains (e.getPosition()))
            {
                owner.getProcessor().toggleFavorite (sample.localFilePath);
                repaint();
                if (owner.onFavoriteToggled)
                    owner.onFavoriteToggled (sample);
                return;
            }
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
        juce::Rectangle<int> heartArea;
    };
}

SampleListBox::SampleListBox (ChopAudioProcessor& p) : processor (p)
{
    listBox.setRowHeight (56);
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

    // A compact single-item drag image so the cursor shows just this one file,
    // not a snapshot of the whole list (which looked like many files selected).
    auto makeDragImage = [] (const juce::String& name)
    {
        juce::Image img (juce::Image::ARGB, 220, 26, true);
        juce::Graphics g (img);
        g.setColour (juce::Colour (0xff19c3b3).withAlpha (0.92f));
        g.fillRoundedRectangle (img.getBounds().toFloat(), 4.0f);
        g.setColour (juce::Colour (0xff0b1f1d));
        g.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
        g.drawText (name, 10, 0, img.getWidth() - 20, img.getHeight(),
                    juce::Justification::centredLeft, true);
        return juce::ScaledImage (img);
    };

    if (sample.localFilePath.isNotEmpty())
    {
        if (onStatus) onStatus ("Dragging \"" + sample.name + "\"");
        // Internal JUCE drag carrying the file path. In-app targets (e.g. the
        // audio-to-audio source zone) receive it; if dragged out of the window,
        // the editor's shouldDropFilesWhenDraggedExternally exports it to the DAW.
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
            container->startDragging (sample.localFilePath, this, makeDragImage (sample.name), true);
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
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
            container->startDragging (file.getFullPathName(), this, makeDragImage (sample.name), true);
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
