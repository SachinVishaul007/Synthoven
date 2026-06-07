#include "MelSpectrogramExtractor.h"
#include <cmath>
#include <algorithm>

MelSpectrogramExtractor::MelSpectrogramExtractor()
    : fft(10) // 2^10 = 1024
{
    // Create Hann window
    hannWindow.resize(fftSize);
    for (int i = 0; i < fftSize; ++i)
    {
        hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * 3.1415926535f * i / (fftSize - 1)));
    }

    createMelFilterbank();
}

MelSpectrogramExtractor::~MelSpectrogramExtractor()
{
}

void MelSpectrogramExtractor::createMelFilterbank()
{
    int numBins = fftSize / 2 + 1;
    melFilters.assign(numMelBins, std::vector<float>(numBins, 0.0f));

    float minHz = 0.0f;
    float maxHz = (float)targetSampleRate / 2.0f;

    auto hzToMel = [](float hz) {
        return 2595.0f * std::log10(1.0f + hz / 700.0f);
    };

    auto melToHz = [](float mel) {
        return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
    };

    float minMel = hzToMel(minHz);
    float maxMel = hzToMel(maxHz);

    std::vector<float> melPoints(numMelBins + 2);
    for (int i = 0; i < numMelBins + 2; ++i)
    {
        float t = (float)i / (float)(numMelBins + 1);
        melPoints[i] = minMel + t * (maxMel - minMel);
    }

    std::vector<int> fftBins(numMelBins + 2);
    for (int i = 0; i < numMelBins + 2; ++i)
    {
        float hz = melToHz(melPoints[i]);
        fftBins[i] = (int)std::round((hz * fftSize) / targetSampleRate);
    }

    for (int m = 1; m <= numMelBins; ++m)
    {
        int binLeft = fftBins[m - 1];
        int binCenter = fftBins[m];
        int binRight = fftBins[m + 1];

        // Left slope
        if (binCenter > binLeft)
        {
            for (int k = binLeft; k < binCenter; ++k)
            {
                if (k < numBins)
                    melFilters[m - 1][k] = (float)(k - binLeft) / (float)(binCenter - binLeft);
            }
        }

        // Right slope
        if (binRight > binCenter)
        {
            for (int k = binCenter; k <= binRight; ++k)
            {
                if (k < numBins)
                    melFilters[m - 1][k] = (float)(binRight - k) / (float)(binRight - binCenter);
            }
        }

        // Apply Slaney normalization
        float hzLeft = melToHz(melPoints[m - 1]);
        float hzRight = melToHz(melPoints[m + 1]);
        float scale = 2.0f / (hzRight - hzLeft);

        for (int k = 0; k < numBins; ++k)
        {
            melFilters[m - 1][k] *= scale;
        }
    }
}

void MelSpectrogramExtractor::resampleAudio(const float* sourceData, double sourceSampleRate, int sourceLength, std::vector<float>& targetData)
{
    if (std::abs(sourceSampleRate - targetSampleRate) < 0.01)
    {
        targetData.assign(sourceData, sourceData + sourceLength);
        return;
    }
    double ratio = sourceSampleRate / targetSampleRate;
    int targetLength = (int)(sourceLength / ratio);
    targetData.resize(targetLength);
    for (int i = 0; i < targetLength; ++i)
    {
        double sourcePos = i * ratio;
        int index = (int)sourcePos;
        double alpha = sourcePos - index;
        if (index + 1 < sourceLength)
        {
            targetData[i] = (float)((1.0 - alpha) * sourceData[index] + alpha * sourceData[index + 1]);
        }
        else if (index < sourceLength)
        {
            targetData[i] = sourceData[index];
        }
        else
        {
            targetData[i] = 0.0f;
        }
    }
}

bool MelSpectrogramExtractor::extractFromFile(const juce::File& audioFile, std::vector<float>& outputFeatures)
{
    outputFeatures.clear();

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
    if (reader == nullptr)
        return false;

    int numChannels = (int)reader->numChannels;
    int sourceLength = (int)reader->lengthInSamples;
    double sourceSampleRate = reader->sampleRate;

    if (sourceLength <= 0 || numChannels <= 0)
        return false;

    // Read channel 0
    juce::AudioBuffer<float> buffer(1, sourceLength);
    reader->read(&buffer, 0, sourceLength, 0, true, false);

    // Resample to 48000 Hz
    std::vector<float> resampledAudio;
    resampleAudio(buffer.getReadPointer(0), sourceSampleRate, sourceLength, resampledAudio);

    int resampledLen = (int)resampledAudio.size();
    if (resampledLen == 0)
        return false;

    // Pad / Repeat audio to exactly 10s + FFT size (481,024 samples)
    std::vector<float> paddedAudio(targetDurationSamples + fftSize, 0.0f);
    for (int i = 0; i < targetDurationSamples + fftSize; ++i)
    {
        paddedAudio[i] = resampledAudio[i % resampledLen];
    }

    // Allocate flat vector for 64 rows * 1001 columns
    outputFeatures.resize(numMelBins * numFrames, 0.0f);

    std::vector<float> fftBuffer(fftSize * 2, 0.0f);
    std::vector<float> powerSpectrum(fftSize / 2 + 1, 0.0f);

    for (int frame = 0; frame < numFrames; ++frame)
    {
        int startSample = frame * hopSize;
        
        // Window the frame
        std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
        for (int i = 0; i < fftSize; ++i)
        {
            fftBuffer[i] = paddedAudio[startSample + i] * hannWindow[i];
        }

        // Perform FFT
        fft.performRealOnlyForwardTransform(fftBuffer.data());

        // Compute Power Spectrum
        powerSpectrum[0] = fftBuffer[0] * fftBuffer[0]; // DC
        for (int k = 1; k < fftSize / 2; ++k)
        {
            float real = fftBuffer[2 * k];
            float imag = fftBuffer[2 * k + 1];
            powerSpectrum[k] = real * real + imag * imag;
        }
        powerSpectrum[fftSize / 2] = fftBuffer[1] * fftBuffer[1]; // Nyquist

        // Apply Mel Filterbank & Compute Log
        for (int mel = 0; mel < numMelBins; ++mel)
        {
            float melValue = 0.0f;
            for (int k = 0; k < fftSize / 2 + 1; ++k)
            {
                melValue += powerSpectrum[k] * melFilters[mel][k];
            }

            // Log-mel compression: log(mel + 1e-7)
            float logMel = std::log(melValue + 1e-7f);

            // Store in output features (row-major order: mel index then frame index, or vice-versa)
            // Let's store as [numMelBins][numFrames] or [numFrames][numMelBins]
            // Standard Hugging Face CLAP expects shape [numFrames, numMelBins] i.e. 1001 x 64
            // So row-major: frame * numMelBins + mel
            outputFeatures[frame * numMelBins + mel] = logMel;
        }
    }

    return true;
}
