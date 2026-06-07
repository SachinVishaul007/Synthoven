#include "PluginProcessor.h"
#include "PluginEditor.h"

ChopAudioProcessor::ChopAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats(); // WAV + AIFF always; FLAC/Ogg if enabled in the build
    readAheadThread.startThread();
}

ChopAudioProcessor::~ChopAudioProcessor()
{
    stopAudition();
    transport.setSource (nullptr);
    readAheadThread.stopThread (2000);
}

void ChopAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    transport.prepareToPlay (samplesPerBlock, sampleRate);
}

void ChopAudioProcessor::releaseResources()
{
    transport.releaseResources();
}

bool ChopAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    const auto in = layouts.getMainInputChannelSet();
    if (! in.isDisabled() && in != out)
        return false;

    return true;
}

void ChopAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    juce::AudioSourceChannelInfo info (&buffer, 0, buffer.getNumSamples());
    transport.getNextAudioBlock (info); // fills with audition audio (silence when stopped)

    // A mono source only fills channel 0 — mirror it to the right channel.
    if (buffer.getNumChannels() >= 2 && numSourceChannels.load() == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());
}

void ChopAudioProcessor::auditionFile (const juce::File& file)
{
    stopAudition();

    auto* reader = formatManager.createReaderFor (file);
    if (reader == nullptr)
        return;

    numSourceChannels.store ((int) reader->numChannels);

    auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
    // setSource locks internally, so this is safe to call while audio is running.
    transport.setSource (newSource.get(),
                         32768,                    // read-ahead buffer
                         &readAheadThread,
                         reader->sampleRate,        // resample to host rate
                         (int) reader->numChannels);
    readerSource = std::move (newSource);

    transport.setPosition (0.0);
    transport.start();
}

void ChopAudioProcessor::stopAudition()
{
    transport.stop();
    transport.setPosition (0.0);
}

juce::AudioProcessorEditor* ChopAudioProcessor::createEditor()
{
    return new ChopAudioProcessorEditor (*this);
}

void ChopAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml ("CHOPSETTINGS");
    xml.setAttribute ("libraryFolder", libraryFolder.getFullPathName());
    copyXmlToBinary (xml, destData);
}

void ChopAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
    {
        if (xmlState->hasTagName ("CHOPSETTINGS"))
        {
            libraryFolder = juce::File (xmlState->getStringAttribute ("libraryFolder"));
            triggerLibraryScan();
        }
    }
}

void ChopAudioProcessor::setLibraryFolder (const juce::File& folder)
{
    libraryFolder = folder;
    triggerLibraryScan();
}

void ChopAudioProcessor::triggerLibraryScan()
{
    if (libraryFolder.isDirectory())
        scannerThread.startScan (libraryFolder);
}

void ChopAudioProcessor::setScannedFiles (juce::Array<juce::File> files)
{
    const juce::ScopedLock sl (localFilesLock);
    localLibraryFiles = std::move (files);
}

juce::Array<juce::File> ChopAudioProcessor::getScannedFiles() const
{
    const juce::ScopedLock sl (localFilesLock);
    return localLibraryFiles;
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChopAudioProcessor();
}
