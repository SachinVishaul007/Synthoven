#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class AudioInputDropTarget : public juce::Component,
                             public juce::FileDragAndDropTarget,
                             public juce::DragAndDropTarget
{
public:
    AudioInputDropTarget()
    {
        // Setup clear button
        clearButton.setButtonText (juce::String::fromUTF8 ("\xe2\x9c\x95")); // ✕ Unicode Clear Character
        clearButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        clearButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffb0b0b5));
        clearButton.onClick = [this] { clear(); };
        addAndMakeVisible (clearButton);
        clearButton.setVisible (false);
    }

    ~AudioInputDropTarget() override = default;

    void setInputFile (const juce::File& file)
    {
        currentFile = file;
        clearButton.setVisible (currentFile.existsAsFile());
        if (onFileChanged)
            onFileChanged (currentFile);
        repaint();
    }

    void clear()
    {
        currentFile = juce::File();
        clearButton.setVisible (false);
        if (onFileChanged)
            onFileChanged (currentFile);
        repaint();
    }

    const juce::File& getInputFile() const { return currentFile; }

    std::function<void (const juce::File&)> onFileChanged;

    /** Resolves an internal "ChopLocalFile" drag (from the folder tree) to a file.
        Set by the editor to return the tree's currently dragged/selected file. */
    std::function<juce::File()> resolveTreeFile;

    static bool isAudioFile (const juce::File& file)
    {
        auto ext = file.getFileExtension().toLowerCase().removeCharacters (".");
        return ext == "wav" || ext == "mp3" || ext == "aiff" || ext == "aif" || ext == "flac";
    }

    // ── FileDragAndDropTarget (external / Finder drags) ─────────────────────
    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        for (auto& f : files)
            if (isAudioFile (juce::File (f)))
                return true;
        return false;
    }

    void fileDragEnter (const juce::StringArray&, int, int) override
    {
        isHovering = true;
        repaint();
    }

    void fileDragExit (const juce::StringArray&) override
    {
        isHovering = false;
        repaint();
    }

    void filesDropped (const juce::StringArray& files, int, int) override
    {
        isHovering = false;
        for (auto& f : files)
        {
            juce::File file (f);
            if (isAudioFile (file))
            {
                setInputFile (file);
                break;
            }
        }
        repaint();
    }

    // ── DragAndDropTarget (internal app drags: sample list & folder tree) ────
    juce::File fileFromDragDescription (const juce::var& description) const
    {
        const auto desc = description.toString();
        if (desc == "ChopLocalFile")              // folder-tree drag
            return resolveTreeFile ? resolveTreeFile() : juce::File();
        if (juce::File::isAbsolutePath (desc))    // sample-list drag (carries path)
            return juce::File (desc);
        return {};
    }

    bool isInterestedInDragSource (const SourceDetails& details) override
    {
        return isAudioFile (fileFromDragDescription (details.description));
    }

    void itemDragEnter (const SourceDetails&) override
    {
        isHovering = true;
        repaint();
    }

    void itemDragExit (const SourceDetails&) override
    {
        isHovering = false;
        repaint();
    }

    void itemDropped (const SourceDetails& details) override
    {
        isHovering = false;
        auto file = fileFromDragDescription (details.description);
        if (isAudioFile (file) && file.existsAsFile())
            setInputFile (file);
        else
            repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.fillAll (juce::Colour (0xff161619));

        // Highlight/Border
        if (isHovering)
        {
            g.setColour (juce::Colour (0xff19c3b3));
            g.drawRoundedRectangle (bounds.reduced (1.0f), 4.0f, 2.0f);
        }
        else if (currentFile.existsAsFile())
        {
            g.setColour (juce::Colour (0xff19c3b3).withAlpha (0.4f));
            g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
        }
        else
        {
            g.setColour (juce::Colour (0xff34343c));
            g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
        }

        // Content
        if (currentFile.existsAsFile())
        {
            // Draw selected file name
            g.setColour (juce::Colour (0xff19c3b3));
            g.fillEllipse (12.0f, bounds.getCentreY() - 3.0f, 6.0f, 6.0f);

            g.setColour (juce::Colours::white);
            g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
            g.drawText (currentFile.getFileName(), 
                        24, 0, getWidth() - 48, getHeight(),
                        juce::Justification::centredLeft, true);
        }
        else
        {
            // Draw placeholder text
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Italic")));
            g.drawText ("Drag & drop audio here...", 
                        12, 0, getWidth() - 24, getHeight(),
                        juce::Justification::centredLeft, true);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        clearButton.setBounds (bounds.removeFromRight (28).reduced (4));
    }

private:
    juce::File currentFile;
    juce::TextButton clearButton;
    bool isHovering = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioInputDropTarget)
};
