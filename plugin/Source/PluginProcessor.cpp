#include "PluginProcessor.h"
#include "PluginEditor.h"

ChopProcessor::ChopProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

ChopProcessor::~ChopProcessor() {}

void ChopProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    samplePlayer.prepareToPlay(samplesPerBlock, sampleRate);
}

void ChopProcessor::releaseResources()
{
    samplePlayer.releaseResources();
}

void ChopProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::AudioSourceChannelInfo info(buffer);
    samplePlayer.getNextAudioBlock(info);
}

juce::AudioProcessorEditor* ChopProcessor::createEditor()
{
    return new ChopEditor(*this);
}

void ChopProcessor::getStateInformation(juce::MemoryBlock&) {}
void ChopProcessor::setStateInformation(const void*, int)   {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChopProcessor();
}
