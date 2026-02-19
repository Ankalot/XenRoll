#pragma once

#include <algorithm>
#include <cmath>
#include <juce_dsp/juce_dsp.h>
#include <utility>
#include <vector>

namespace pitch_detection {

/**
 * @brief McLeod Pitch Method (MPM) for real-time pitch detection
 * Adapted from https://github.com/sevagh/pitch-detection to work with JUCE
 */
class PitchDetectorMPM {
  public:
    PitchDetectorMPM(int fftSize = 8192);
    ~PitchDetectorMPM();

    /**
     * @brief Detect pitch from audio buffer
     * @param audioBuffer Audio buffer to analyze (mono)
     * @param sampleRate Sample rate in Hz
     * @return Detected frequency in Hz, or 0.0f if no clear pitch detected
     */
    float detectPitch(const std::vector<float> &audioBuffer, double sampleRate);

    /**
     * @brief Reset the detector state
     */
    void reset();

    /**
     * @brief Set voice frequency range for filtering
     * @param minFreq Minimum frequency in Hz (default: 70 Hz)
     * @param maxFreq Maximum frequency in Hz (default: 1300 Hz)
     */
    void setVoiceRange(float minFreq, float maxFreq);

  private:
    // FFT configuration
    int fftSize;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    // Voice range filtering (in Hz)
    float minVoiceFreq;
    float maxVoiceFreq;

    // MPM parameters
    static constexpr float MPM_CUTOFF = 0.93f;
    static constexpr float MPM_SMALL_CUTOFF = 0.5f;

    // Buffers for autocorrelation
    std::vector<float> fftBuffer;
    std::vector<float> nsdfBuffer; // Normalized Square Difference Function

    /**
     * @brief Calculate Normalized Square Difference Function (NSDF)
     * @param audioBuffer Input audio buffer
     * @return NSDF values
     */
    std::vector<float> calculateNSDF(const std::vector<float> &audioBuffer);

    /**
     * @brief Find peaks in NSDF
     * @param nsdf NSDF values
     * @return Vector of peak positions
     */
    std::vector<int> peakPicking(const std::vector<float> &nsdf);

    /**
     * @brief Parabolic interpolation for better frequency estimation
     * @param data Data array
     * @param peakIndex Index of the peak
     * @return Pair of (interpolated position, interpolated value)
     */
    std::pair<float, float> parabolicInterpolation(const std::vector<float> &data, int peakIndex);
};

} // namespace pitch_detection
