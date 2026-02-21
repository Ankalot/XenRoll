#include "PitchDetectorMPM.h"
#include <algorithm>
#include <limits>

namespace pitch_detection {

PitchDetectorMPM::PitchDetectorMPM(int fftSize)
    : fftSize(fftSize), minVoiceFreq(70.0f), maxVoiceFreq(1500.0f), peakCount(0) {
    // Initialize FFT
    fft = std::make_unique<juce::dsp::FFT>(std::log2(fftSize));

    // Initialize window (Hann window)
    window = std::make_unique<juce::dsp::WindowingFunction<float>>(
        fftSize, juce::dsp::WindowingFunction<float>::hann);

    // Allocate buffers (pre-allocated to avoid real-time allocations)
    fftBuffer.resize(fftSize * 2);
    nsdfBuffer.resize(fftSize);
    peakBuffer.resize(fftSize / 2); // Maximum possible peaks
}

PitchDetectorMPM::~PitchDetectorMPM() {}

void PitchDetectorMPM::resetJumpsDetection() {
    prevPitch = 0.0f;
    prevPrevPitch = 0.0f;
    numFixes = 0;
}

void PitchDetectorMPM::reset() {
    std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::fill(nsdfBuffer.begin(), nsdfBuffer.end(), 0.0f);
    peakCount = 0;
    resetJumpsDetection();
}

void PitchDetectorMPM::setVoiceRange(float minFreq, float maxFreq) {
    minVoiceFreq = minFreq;
    maxVoiceFreq = maxFreq;
}

const std::vector<float>& PitchDetectorMPM::calculateNSDF(const std::vector<float> &audioBuffer) {
    const int bufferSize = static_cast<int>(audioBuffer.size());
    const int nsdfSize = bufferSize / 2;

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
        // Return zeros if signal is too weak
        std::fill(nsdfBuffer.begin(), nsdfBuffer.begin() + nsdfSize, 0.0f);
        return nsdfBuffer;
    }

    for (int tau = 0; tau < nsdfSize; ++tau) {
        float rTau = fftBuffer[tau];
        float denominator = r0 + rTau;

        if (std::abs(denominator) < 1e-10f) {
            nsdfBuffer[tau] = 0.0f;
        } else {
            nsdfBuffer[tau] = 2.0f * rTau / denominator;
        }
    }

    return nsdfBuffer;
}

const std::vector<int>& PitchDetectorMPM::peakPicking(const std::vector<float> &nsdf) {
    peakCount = 0;
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
            if (curMaxPos > 0 && peakCount < static_cast<int>(peakBuffer.size())) {
                peakBuffer[peakCount++] = curMaxPos;
                curMaxPos = 0;
            }

            while (pos < size - 1 && nsdf[pos] <= 0.0f) {
                pos++;
            }
        }
    }

    if (curMaxPos > 0 && peakCount < static_cast<int>(peakBuffer.size())) {
        peakBuffer[peakCount++] = curMaxPos;
    }

    // Resize to actual number of peaks (this is cheap since we're shrinking)
    // Note: In a real-time critical path, we could avoid this by tracking the count separately
    // but for simplicity and correctness, we keep the resize
    peakBuffer.resize(peakCount);

    return peakBuffer;
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

float PitchDetectorMPM::detectPitch(const std::vector<float> &audioBuffer, double sampleRate,
                                    bool wasSilence) {
    if (audioBuffer.empty() || sampleRate <= 0) {
        resetJumpsDetection();
        return 0.0f;
    }

    // Calculate NSDF (reuses nsdfBuffer)
    const std::vector<float>& nsdf = calculateNSDF(audioBuffer);

    // Find peaks (reuses peakBuffer)
    const std::vector<int>& maxPositions = peakPicking(nsdf);

    if (maxPositions.empty()) {
        resetJumpsDetection();
        return 0.0f;
    }

    // Find highest amplitude
    float highestAmplitude = -std::numeric_limits<float>::max();
    for (int i = 0; i < peakCount; ++i) {
        highestAmplitude = std::max(highestAmplitude, nsdf[maxPositions[i]]);
    }

    if (highestAmplitude < MPM_SMALL_CUTOFF) {
        resetJumpsDetection();
        return 0.0f;
    }

    // Parabolic interpolation for all peaks above small cutoff
    // Use fixed-size array to avoid dynamic allocation (max 20 peaks should be enough)
    struct Estimate {
        float period;
        float amplitude;
    };
    Estimate estimates[20];
    int estimateCount = 0;

    for (int i = 0; i < peakCount && estimateCount < 20; ++i) {
        int pos = maxPositions[i];
        if (nsdf[pos] > MPM_SMALL_CUTOFF) {
            auto x = parabolicInterpolation(nsdf, pos);
            estimates[estimateCount++] = Estimate{x.first, x.second};
            highestAmplitude = std::max(highestAmplitude, x.second);
        }
    }

    if (estimateCount == 0) {
        resetJumpsDetection();
        return 0.0f;
    }

    // Find the first peak above actual cutoff
    float actualCutoff = MPM_CUTOFF * highestAmplitude;
    float period = 0.0f;

    for (int i = 0; i < estimateCount; ++i) {
        if (estimates[i].amplitude >= actualCutoff) {
            period = estimates[i].period;
            break;
        }
    }

    if (period < 1.0f) {
        resetJumpsDetection();
        return 0.0f;
    }

    // Calculate frequency
    float pitchEstimate = static_cast<float>(sampleRate) / period;

    // Apply voice range filtering
    if (pitchEstimate < minVoiceFreq || pitchEstimate > maxVoiceFreq) {
        resetJumpsDetection();
        return 0.0f;
    }

    if (wasSilence) {
        resetJumpsDetection();
    } else {
        // Adjust the pitch if an unrealistic jump has occurred.
        if ((numFixes <= maxNumFixes) && (prevPitch != 0.0f) &&
            ((prevPitch / pitchEstimate > 1.9f) || (pitchEstimate / prevPitch > 1.9f))) {
            if (prevPrevPitch != 0.0f) {
                float newPitchEstimate = prevPrevPitch + 2 * (prevPitch - prevPrevPitch);
                pitchEstimate = juce::jlimit(minVoiceFreq, maxVoiceFreq, newPitchEstimate);
            } else {
                pitchEstimate = prevPitch;
            }
            numFixes++;
        } else {
            numFixes = 0;
        }
        prevPrevPitch = prevPitch;
    }
    prevPitch = pitchEstimate;

    return pitchEstimate;
}

} // namespace pitch_detection
