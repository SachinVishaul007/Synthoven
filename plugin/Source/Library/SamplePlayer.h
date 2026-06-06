#pragma once
#include <JuceHeader.h>

class SamplePlayer : public juce::AudioSource
{
public:
    SamplePlayer();
    ~SamplePlayer() override;

    bool   loadFile(const juce::File& file);
    void   play();
    void   stop();
    bool   isPlaying()          const;
    double getPositionSeconds() const;
    double getDurationSeconds() const;

    // juce::AudioSource
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info)   override;
    void releaseResources()                                            override;

private:
    juce::AudioFormatManager                       formatManager;
    juce::AudioTransportSource                     transport;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
};
