#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

class MelSpectrogramExtractor
{
public:
    MelSpectrogramExtractor();
    ~MelSpectrogramExtractor();

    /**
     * Extracts a 64x1001 log-mel spectrogram from an audio file.
     * The output vector is flattened, containing 64 rows and 1001 columns.
     */
    bool extractFromFile(const juce::File& audioFile, std::vector<float>& outputFeatures);

private:
    static constexpr int targetSampleRate = 48000;
    static constexpr int numMelBins = 64;
    static constexpr int fftSize = 1024;
    static constexpr int hopSize = 480;
    static constexpr int numFrames = 1001;
    static constexpr int targetDurationSamples = 480000; // 10 seconds

    juce::dsp::FFT fft;
    std::vector<float> hannWindow;
    std::vector<std::vector<float>> melFilters; // [numMelBins][fftSize / 2 + 1]

    void createMelFilterbank();
    void applyHannWindow(const float* input, float* output);
    void resampleAudio(const float* sourceData, double sourceSampleRate, int sourceLength, std::vector<float>& targetData);
};
