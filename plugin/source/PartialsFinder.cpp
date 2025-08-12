#include "XenRoll/PartialsFinder.h"

namespace audio_plugin {
PartialsFinder::PartialsFinder() { fft = std::make_unique<juce::dsp::FFT>(std::log2(fftSize)); }

void PartialsFinder::setdBThr(float newdBThr) { dBThr = newdBThr; }

float PartialsFinder::findRMS(const juce::AudioBuffer<float> &buffer, int startSample,
                              int numSamples) {
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float sample = buffer.getSample(0, startSample + i);
        sum += sample * sample;
    }
    return std::sqrt(sum / numSamples);
}

std::vector<float> PartialsFinder::findRMSes(const juce::AudioBuffer<float> &buffer) {
    int numSamples = buffer.getNumSamples();
    int numWindows = (numSamples - fftSize) / (fftSize / 2) + 1;
    std::vector<float> RMSes(numWindows);
    for (int i = 0; i < numWindows; ++i) {
        RMSes[i] = findRMS(buffer, i * fftSize / 2, fftSize);
    }
    return RMSes;
}

int PartialsFinder::findPosMidrangeRMS(const juce::AudioBuffer<float> &buffer) {
    std::vector<float> RMSes = findRMSes(buffer);

    auto [min_it, max_it] = std::minmax_element(RMSes.begin(), RMSes.end());
    float midrangeRMS = (*min_it + *max_it) / 2;

    int midrangeRMSpos = 0;
    float minErr = abs(RMSes[0] - midrangeRMS);
    for (int i = 1; i < RMSes.size(); ++i) {
        float thisErr = abs(RMSes[i] - midrangeRMS);
        if (thisErr < minErr) {
            midrangeRMSpos = i;
            minErr = thisErr;
        }
    }

    return midrangeRMSpos * fftSize / 2;
}

int PartialsFinder::findPosMedianRMS(const juce::AudioBuffer<float> &buffer) {
    std::vector<float> RMSes = findRMSes(buffer);

    float medianRMS;
    int numRMSes = RMSes.size();
    auto mid_it = RMSes.begin() + numRMSes / 2;
    std::nth_element(RMSes.begin(), mid_it, RMSes.end());
    if (numRMSes % 2 != 0) {
        medianRMS = *mid_it;
    } else {
        float mid_left = *std::max_element(RMSes.begin(), mid_it);
        medianRMS = (mid_left + *mid_it) / 2.0;
    }

    int medianRMSpos = 0;
    float minErr = abs(RMSes[0] - medianRMS);
    for (int i = 1; i < numRMSes; ++i) {
        float thisErr = abs(RMSes[i] - medianRMS);
        if (thisErr < minErr) {
            medianRMSpos = i;
            minErr = thisErr;
        }
    }

    return medianRMSpos * fftSize / 2;
}

int PartialsFinder::findPosMinRMSfluct(const juce::AudioBuffer<float> &buffer) {
    std::vector<float> RMSes = findRMSes(buffer);
    if (RMSes.size() < 3)
        return 0;
    auto max_it = std::max_element(RMSes.begin(), RMSes.end());

    const float maxRMStoUse = *max_it;
    const float minRMStoUse = maxRMStoUse / 2;

    int minRMSfluctPos = 0;
    float minFluct = std::numeric_limits<float>::max();
    for (int i = 1; i < RMSes.size() - 1; ++i) {
        if (RMSes[i] < minRMStoUse)
            continue;

        float thisFluct = abs(RMSes[i] - RMSes[i - 1]) + abs(RMSes[i] - RMSes[i + 1]);
        if (thisFluct < minFluct) {
            minFluct = thisFluct;
            minRMSfluctPos = i;
        }
    }

    return minRMSfluctPos * fftSize / 2;
}

float PartialsFinder::findSpectralFlatness(const juce::AudioBuffer<float> &buffer,
                                           int startSample) {

    // --- Step 1: Extract and window the analysis window ---
    std::vector<float> windowed(fftSize, 0.0f);
    const float *data = buffer.getReadPointer(0);
    juce::FloatVectorOperations::copy(windowed.data(), data + startSample, fftSize);

    juce::dsp::WindowingFunction<float> windowing(fftSize,
                                                  juce::dsp::WindowingFunction<float>::blackman);
    windowing.multiplyWithWindowingTable(windowed.data(), fftSize);

    // --- Step 2: Perform FFT ---
    std::vector<float> fftData(fftSize * 2);
    juce::FloatVectorOperations::copy(fftData.data(), windowed.data(), fftSize);
    fft->performRealOnlyForwardTransform(fftData.data(), false);

    // --- Step 3: Compute spectral flatness ---
    float logSum = 0.0f, arithmeticSum = 0.0f;
    int numBins = 0;

    for (int i = 1; i < fftSize / 2; ++i) {
        int reIdx = 2 * i;
        int imIdx = reIdx + 1;
        float magnitude =
            std::sqrt(fftData[reIdx] * fftData[reIdx] + fftData[imIdx] * fftData[imIdx]);

        if (magnitude > 1e-8f) {
            logSum += std::log(magnitude);
            arithmeticSum += magnitude;
            numBins++;
        }
    }

    return (numBins > 0) ? std::exp(logSum / numBins) / (arithmeticSum / numBins) : 0.0f;
}

int PartialsFinder::findPosPeakSample(const juce::AudioBuffer<float> &buffer) {
    const float *channelData = buffer.getReadPointer(0);
    int peakPos = 0;
    float peakVal = std::abs(channelData[0]);

    for (int i = 1; i < buffer.getNumSamples(); ++i) {
        float absVal = std::abs(channelData[i]);
        if (absVal > peakVal) {
            peakVal = absVal;
            peakPos = i;
        }
    }

    return juce::jlimit(0, buffer.getNumSamples() - fftSize, peakPos);
}

int PartialsFinder::findPosMaxSpectralFlatness(const juce::AudioBuffer<float> &buffer) {
    std::vector<float> RMSes = findRMSes(buffer);
    if (RMSes.size() < 3)
        return 0;
    auto max_it = std::max_element(RMSes.begin(), RMSes.end());

    const float maxRMStoUse = *max_it;
    const float minRMStoUse = maxRMStoUse / 2;

    int maxSpectralFlatnessPos = 0;
    float maxSpectralFlatness = findSpectralFlatness(buffer, 0);
    for (int i = 1; i < RMSes.size(); ++i) {
        if (RMSes[i] < minRMStoUse)
            continue;

        float thisSpectralFlatness = findSpectralFlatness(buffer, i * fftSize / 2);
        if (thisSpectralFlatness > maxSpectralFlatness) {
            maxSpectralFlatness = thisSpectralFlatness;
            maxSpectralFlatnessPos = i;
        }
    }

    return maxSpectralFlatnessPos * fftSize / 2;
}

float PartialsFinder::computedBThreshold(float freq) {
    const float a_L = 26.0f;
    const float a_R = 32.0f;
    const float b = 0.0075f;
    return dBThr + a_L + (a_R / (b - 1)) * (1 - pow(b, freq / 20000));
}

int findFirstNonZeroSample(const juce::AudioBuffer<float> &buffer, int channel = 0) {
    const float *data = buffer.getReadPointer(channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        if (std::abs(data[i]) > 1e-5f) { // Threshold for "zero" (-100 dB)
            return i;
        }
    }
    return -1; // Entire buffer is silent
}

int findLastNonZeroSample(const juce::AudioBuffer<float> &buffer, int channel = 0) {
    const float *data = buffer.getReadPointer(channel);
    for (int i = buffer.getNumSamples() - 1; i >= 0; --i) {
        if (std::abs(data[i]) > 1e-5f) { // Threshold for "zero" (-100 dB)
            return i;
        }
    }
    return -1; // Entire buffer is silent
}

void trimSilenceInPlace(juce::AudioBuffer<float> &buffer) {
    const int start = findFirstNonZeroSample(buffer);
    const int end = findLastNonZeroSample(buffer);

    if (start == -1 || end == -1) {
        buffer.setSize(buffer.getNumChannels(), 0);
        return;
    }

    const int numSamplesToKeep = end - start + 1;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float *data = buffer.getWritePointer(ch);
        std::memmove(data, data + start, numSamplesToKeep * sizeof(float));
    }

    buffer.setSize(buffer.getNumChannels(), numSamplesToKeep, false);
}

// Investigates only 0 channel
partialsVec PartialsFinder::findPartials(juce::AudioBuffer<float> buffer) {
    // 0. Get rid of zero samples at the beginning and end of the buffer
    trimSilenceInPlace(buffer);
    if (buffer.getNumSamples() < fftSize) {
        int oldSize = buffer.getNumSamples();
        buffer.setSize(buffer.getNumChannels(), fftSize, true);
        buffer.clear(oldSize, fftSize - oldSize);
    }

    /*
    int bufferSize = buffer.getNumSamples();
    int windowSize = juce::jmin(bufferSize, (int)std::pow(2, 15));
    int fftSize = (int)std::pow(2, std::ceil(std::log2(bufferSize)));
    fft = std::make_unique<juce::dsp::FFT>(std::log2(fftSize));

    std::vector<float> windowed(windowSize);
    const float *data = buffer.getReadPointer(0);
    juce::FloatVectorOperations::copy(windowed.data(), data, windowSize);

    juce::dsp::WindowingFunction<float> windowing(windowSize,
                                                  juce::dsp::WindowingFunction<float>::blackman);
    windowing.multiplyWithWindowingTable(windowed.data(), windowSize);
    */

    // 1. Find best position for window
    int startSample;
    switch (posFindStrat) {
    case PosFindStrat::midrangeRMS:
        startSample = findPosMidrangeRMS(buffer);
        break;
    case PosFindStrat::medianRMS:
        startSample = findPosMedianRMS(buffer);
        break;
    case PosFindStrat::minRMSfluct:
        startSample = findPosMinRMSfluct(buffer);
        break;
    case PosFindStrat::maxSpectralFlatness:
        startSample = findPosMaxSpectralFlatness(buffer);
        break;
    case PosFindStrat::peakSample:
        startSample = findPosPeakSample(buffer);
        break;
    default:
        startSample = (buffer.getNumSamples() - fftSize) / 2;
    }
    DBG("time for window: " << startSample / sampleRate);

    // 2. Extract and window the analysis window
    std::vector<float> windowed(fftSize);
    const float *data = buffer.getReadPointer(0);
    juce::FloatVectorOperations::copy(windowed.data(), data + startSample, fftSize);

    juce::dsp::WindowingFunction<float> windowing(fftSize,
                                                  juce::dsp::WindowingFunction<float>::blackman);
    windowing.multiplyWithWindowingTable(windowed.data(), fftSize);

    // 3. Perform FFT
    std::vector<float> fftData(fftSize * 2);
    std::copy(windowed.begin(), windowed.end(), fftData.begin());
    fft->performRealOnlyForwardTransform(fftData.data(), false);

    // 4. Find partials
    std::vector<float> magnitudes(fftSize / 2);
    std::vector<float> dbs(fftSize / 2);
    for (int i = 0; i < fftSize / 2; ++i) {
        const int reIdx = 2 * i;
        const int imIdx = reIdx + 1;
        magnitudes[i] =
            std::sqrt(fftData[reIdx] * fftData[reIdx] + fftData[imIdx] * fftData[imIdx]) / fftSize;
        dbs[i] = juce::Decibels::gainToDecibels(magnitudes[i]);
    }

    const float binWidth = sampleRate / fftSize;
    partialsVec partials;
    for (int i = 2; i < fftSize / 2 - 2; ++i) {
        if ((magnitudes[i] > magnitudes[i - 1]) && (magnitudes[i - 1] > magnitudes[i - 2]) &&
            (magnitudes[i] > magnitudes[i + 1]) && (magnitudes[i + 1] > magnitudes[i + 2])) {

            // Using parabolic interpolation
            float freq =
                i * binWidth +
                ((dbs[i - 1] - dbs[i + 1]) / (dbs[i - 1] - 2 * dbs[i] + dbs[i + 1])) / 2.0f;
            float real = fftData[2 * i] - (fftData[2 * (i - 1)] - fftData[2 * (i + 1)]) / 4.0f;
            float imag =
                fftData[2 * i + 1] - (fftData[2 * (i - 1) + 1] - fftData[2 * (i + 1) + 1]) / 4.0f;
            float magn = std::sqrt(real * real + imag * imag) / fftSize;
            float dB = juce::Decibels::gainToDecibels(magn);
            float dBThreshold = computedBThreshold(freq);

            if ((freq > 20) && (freq < 20000) && (dB > dBThreshold)) {
                partials.push_back({freq, magn});
            }
        }
    }

    return partials;
}

void PartialsFinder::setPosFindStrat(PosFindStrat pfs) { posFindStrat = pfs; }

void PartialsFinder::setSampleRate_(double newSampleRate) { sampleRate = newSampleRate; }

void PartialsFinder::setFFTSize(int newFFTSize) {
    newFFTSize = juce::roundToInt(std::pow(2, std::log2(fftSize)));
    if (fftSize != newFFTSize) {
        fftSize = newFFTSize;
        fft = std::make_unique<juce::dsp::FFT>(std::log2(fftSize));
    }
}
} // namespace audio_plugin