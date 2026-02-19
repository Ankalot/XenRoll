#include "PitchDetectorMPM.h"
#include <algorithm>
#include <limits>

namespace pitch_detection {

PitchDetectorMPM::PitchDetectorMPM(int fftSize)
    : fftSize(fftSize), minVoiceFreq(70.0f), maxVoiceFreq(1300.0f) {
    // Initialize FFT
    fft = std::make_unique<juce::dsp::FFT>(std::log2(fftSize));

    // Initialize window (Hann window)
    window = std::make_unique<juce::dsp::WindowingFunction<float>>(
        fftSize, juce::dsp::WindowingFunction<float>::hann);

    // Allocate buffers
    fftBuffer.resize(fftSize * 2);
    nsdfBuffer.resize(fftSize);
}

PitchDetectorMPM::~PitchDetectorMPM() {}

void PitchDetectorMPM::reset() {
    std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::fill(nsdfBuffer.begin(), nsdfBuffer.end(), 0.0f);
}

void PitchDetectorMPM::setVoiceRange(float minFreq, float maxFreq) {
    minVoiceFreq = minFreq;
    maxVoiceFreq = maxFreq;
}

std::vector<float> PitchDetectorMPM::calculateNSDF(const std::vector<float> &audioBuffer) {
    const int bufferSize = static_cast<int>(audioBuffer.size());
    const int nsdfSize = bufferSize / 2;

    std::vector<float> nsdf(nsdfSize, 0.0f);

    // Calculate autocorrelation using FFT
    // Copy audio data to FFT buffer and apply window
    std::copy(audioBuffer.begin(), audioBuffer.begin() + std::min(bufferSize, fftSize),
              fftBuffer.begin());
    std::fill(fftBuffer.begin() + std::min(bufferSize, fftSize), fftBuffer.end(), 0.0f);

    // Apply window
    window->multiplyWithWindowingTable(fftBuffer.data(), fftSize);

    // Perform FFT
    fft->performRealOnlyForwardTransform(fftBuffer.data(), false);

    // Compute autocorrelation by multiplying each frequency bin by its complex conjugate
    // and applying FFT normalization scale factor
    const float scale = 1.0f / static_cast<float>(fftSize);
    for (int i = 0; i < fftSize / 2 + 1; ++i) {
        int reIdx = 2 * i;
        int imIdx = reIdx + 1;
        float real = fftBuffer[reIdx];
        float imag = fftBuffer[imIdx];

        // Multiply by complex conjugate: (a + bi) * (a - bi) = a^2 + b^2
        // This gives us the power spectrum, which is the frequency-domain autocorrelation
        fftBuffer[reIdx] = (real * real + imag * imag) * scale;
        fftBuffer[imIdx] = 0.0f; // Imaginary part is zero for real autocorrelation
    }

    // Inverse FFT to get autocorrelation
    fft->performRealOnlyInverseTransform(fftBuffer.data());

    // Calculate NSDF (Normalized Square Difference Function)
    // NSDF(tau) = 2 * R(tau) / (R(0) + R(tau))
    // where R is autocorrelation
    float r0 = fftBuffer[0]; // Autocorrelation at lag 0

    if (r0 < 1e-10f) {
        return nsdf; // Return zeros if signal is too weak
    }

    for (int tau = 0; tau < nsdfSize; ++tau) {
        float rTau = fftBuffer[tau];
        float denominator = r0 + rTau;

        if (std::abs(denominator) < 1e-10f) {
            nsdf[tau] = 0.0f;
        } else {
            nsdf[tau] = 2.0f * rTau / denominator;
        }
    }

    return nsdf;
}

std::vector<int> PitchDetectorMPM::peakPicking(const std::vector<float> &nsdf) {
    std::vector<int> maxPositions;
    int pos = 0;
    int curMaxPos = 0;
    int size = static_cast<int>(nsdf.size());

    // Find first positive value
    while (pos < (size - 1) / 3 && nsdf[pos] > 0) {
        pos++;
    }

    // Find first negative value
    while (pos < size - 1 && nsdf[pos] <= 0.0f) {
        pos++;
    }

    if (pos == 0) {
        pos = 1;
    }

    // Find peaks
    while (pos < size - 1) {
        if (nsdf[pos] > nsdf[pos - 1] && nsdf[pos] >= nsdf[pos + 1]) {
            if (curMaxPos == 0 || nsdf[pos] > nsdf[curMaxPos]) {
                curMaxPos = pos;
            }
        }

        pos++;

        if (pos < size - 1 && nsdf[pos] <= 0) {
            if (curMaxPos > 0) {
                maxPositions.push_back(curMaxPos);
                curMaxPos = 0;
            }

            while (pos < size - 1 && nsdf[pos] <= 0.0f) {
                pos++;
            }
        }
    }

    if (curMaxPos > 0) {
        maxPositions.push_back(curMaxPos);
    }

    return maxPositions;
}

std::pair<float, float> PitchDetectorMPM::parabolicInterpolation(const std::vector<float> &data,
                                                                 int peakIndex) {
    if (peakIndex <= 0 || peakIndex >= static_cast<int>(data.size()) - 1) {
        return {static_cast<float>(peakIndex), data[peakIndex]};
    }

    float y1 = data[peakIndex - 1];
    float y2 = data[peakIndex];
    float y3 = data[peakIndex + 1];

    float denominator = 2.0f * (y1 - 2.0f * y2 + y3);

    if (std::abs(denominator) < 1e-10f) {
        return {static_cast<float>(peakIndex), y2};
    }

    float offset = (y1 - y3) / denominator;
    float x = static_cast<float>(peakIndex) + offset;
    float y = y2 - 0.25f * (y1 - y3) * offset;

    return {x, y};
}

float PitchDetectorMPM::detectPitch(const std::vector<float> &audioBuffer, double sampleRate) {
    if (audioBuffer.empty() || sampleRate <= 0) {
        return 0.0f;
    }

    // Calculate NSDF
    std::vector<float> nsdf = calculateNSDF(audioBuffer);

    // Find peaks
    std::vector<int> maxPositions = peakPicking(nsdf);

    if (maxPositions.empty()) {
        return 0.0f;
    }

    // Find highest amplitude
    float highestAmplitude = -std::numeric_limits<float>::max();
    for (int pos : maxPositions) {
        highestAmplitude = std::max(highestAmplitude, nsdf[pos]);
    }

    if (highestAmplitude < MPM_SMALL_CUTOFF) {
        return 0.0f;
    }

    // Parabolic interpolation for all peaks above small cutoff
    std::vector<std::pair<float, float>> estimates;
    for (int pos : maxPositions) {
        if (nsdf[pos] > MPM_SMALL_CUTOFF) {
            auto x = parabolicInterpolation(nsdf, pos);
            estimates.push_back(x);
            highestAmplitude = std::max(highestAmplitude, x.second);
        }
    }

    if (estimates.empty()) {
        return 0.0f;
    }

    // Find the first peak above actual cutoff
    float actualCutoff = MPM_CUTOFF * highestAmplitude;
    float period = 0.0f;

    for (const auto &estimate : estimates) {
        if (estimate.second >= actualCutoff) {
            period = estimate.first;
            break;
        }
    }

    if (period < 1.0f) {
        return 0.0f;
    }

    // Calculate frequency
    float pitchEstimate = static_cast<float>(sampleRate) / period;

    // Apply voice range filtering
    if (pitchEstimate < minVoiceFreq || pitchEstimate > maxVoiceFreq) {
        return 0.0f;
    }

    return pitchEstimate;
}

} // namespace pitch_detection
