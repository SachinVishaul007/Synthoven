#include "SamplePlayer.h"

SamplePlayer::SamplePlayer()
{
    formatManager.registerBasicFormats();
}

SamplePlayer::~SamplePlayer()
{
    transport.setSource(nullptr);
}

bool SamplePlayer::loadFile(const juce::File& file)
{
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return false;

    transport.stop();
    transport.setSource(nullptr);
    readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
    transport.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
    return true;
}

void SamplePlayer::play()
{
    transport.setPosition(0.0);
    transport.start();
}

void SamplePlayer::stop()
{
    transport.stop();
}

bool SamplePlayer::isPlaying() const
{
    return transport.isPlaying();
}

double SamplePlayer::getPositionSeconds() const
{
    return transport.getCurrentPosition();
}

double SamplePlayer::getDurationSeconds() const
{
    return transport.getLengthInSeconds();
}

void SamplePlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void SamplePlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    transport.getNextAudioBlock(info);
}

void SamplePlayer::releaseResources()
{
    transport.releaseResources();
}
