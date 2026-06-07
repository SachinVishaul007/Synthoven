#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class WaveformPreviewComponent : public juce::Component,
                                 public juce::ChangeListener,
                                 public juce::Timer
                                 
{
public:
    explicit WaveformPreviewComponent (ChopAudioProcessor& p)
        : processor (p),
          thumbnailCache (5),
          thumbnail (512, processor.getFormatManager(), thumbnailCache)
    {
        thumbnail.addChangeListener (this);

        // Play/Pause button
        playButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff19c3b3));
        playButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff0b1f1d));
        playButton.onClick = [this] { togglePlayPause(); };
        updatePlayButtonText();
        addAndMakeVisible (playButton);

        // Metadata label
        metadataLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));
        metadataLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
        metadataLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (metadataLabel);

        startTimer (40); // ~25 FPS playhead update
    }

    ~WaveformPreviewComponent() override
    {
        thumbnail.removeChangeListener (this);
        stopTimer();
    }

    void setSampleFile (const juce::File& file)
    {
        currentFile = file;
        thumbnail.setSource (new juce::FileInputSource (file));

        juce::String metadataText = "";
        if (file.existsAsFile())
        {
            if (auto* reader = processor.getFormatManager().createReaderFor (file))
            {
                double duration = (double)reader->lengthInSamples / reader->sampleRate;
                int channels = reader->numChannels;
                juce::String channelStr = (channels == 1) ? "Mono" : (channels == 2 ? "Stereo" : juce::String (channels) + "ch");
                
                metadataText << file.getFileExtension().toUpperCase() << "  |  "
                             << juce::String (reader->sampleRate / 1000.0, 1) << " kHz  |  "
                             << channelStr << "  |  "
                             << juce::String (duration, 2) << "s";
                
                delete reader;
            }
        }
        metadataLabel.setText (metadataText, juce::dontSendNotification);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        // Draw background
        g.fillAll (juce::Colour (0xff161619));

        // Draw waveform area background & border
        g.setColour (juce::Colour (0xff202024));
        g.fillRoundedRectangle (waveformArea.toFloat(), 4.0f);
        g.setColour (juce::Colour (0xff34343c));
        g.drawRoundedRectangle (waveformArea.toFloat(), 4.0f, 1.0f);

        if (thumbnail.getNumChannels() > 0)
        {
            // Draw waveform in teal accent
            g.setColour (juce::Colour (0xff19c3b3));
            thumbnail.drawChannels (g, waveformArea.reduced (2), 0.0, thumbnail.getTotalLength(), 1.0f);

            // Draw playhead
            double length = processor.getPlaybackLength();
            if (length > 0.0)
            {
                double pos = processor.getPlaybackPosition();
                double ratio = pos / length;
                float playheadX = (float)waveformArea.getX() + (float)waveformArea.getWidth() * (float)ratio;

                g.setColour (juce::Colours::white);
                g.drawVerticalLine ((int)playheadX, (float)waveformArea.getY(), (float)waveformArea.getBottom());
            }
        }
        else
        {
            g.setColour (juce::Colours::white.withAlpha (0.2f));
            g.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Italic")));
            g.drawText ("No sample loaded", waveformArea, juce::Justification::centred, false);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6);

        // Position play button
        int btnSize = 36;
        playButton.setBounds (r.getX(), r.getY() + (r.getHeight() - btnSize) / 2, btnSize, btnSize);
        r.removeFromLeft (btnSize + 12);

        // Position metadata label above the waveform
        auto topArea = r.removeFromTop (16);
        r.removeFromTop (4); // spacer between metadata and waveform
        
        metadataLabel.setBounds (topArea);
        metadataLabel.setJustificationType (juce::Justification::centredRight);

        // Waveform takes the remaining space (stretching all the way to the right)
        waveformArea = r;
    }

    void changeListenerCallback (juce::ChangeBroadcaster* source) override
    {
        if (source == &thumbnail)
            repaint();
    }

    void timerCallback() override
    {
        updatePlayButtonText();
        repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (waveformArea.contains (e.x, e.y))
        {
            scrubTo (e.x);
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (waveformArea.contains (e.x, e.y) || (e.x >= waveformArea.getX() && e.x <= waveformArea.getRight()))
        {
            scrubTo (e.x);
        }
    }

private:
    void togglePlayPause()
    {
        if (processor.isAuditioning())
            processor.pausePlayback();
        else
            processor.startPlayback();

        updatePlayButtonText();
    }

    void updatePlayButtonText()
    {
        if (processor.isAuditioning())
            playButton.setButtonText (juce::String::fromUTF8 ("\xe2\x8f\xb8")); // ⏸ (Pause symbol)
        else
            playButton.setButtonText (juce::String::fromUTF8 ("\xe2\x96\xb6")); // ▶ (Play symbol)
    }

    void scrubTo (int mouseX)
    {
        double length = processor.getPlaybackLength();
        if (length > 0.0 && waveformArea.getWidth() > 0)
        {
            float relativeX = (float)(mouseX - waveformArea.getX()) / (float)waveformArea.getWidth();
            relativeX = juce::jlimit (0.0f, 1.0f, relativeX);
            double targetPos = (double)relativeX * length;
            processor.setPlaybackPosition (targetPos);
            repaint();
        }
    }

    ChopAudioProcessor& processor;
    juce::File currentFile;

    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;

    juce::TextButton playButton;
    juce::Label metadataLabel;

    juce::Rectangle<int> waveformArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformPreviewComponent)
};
