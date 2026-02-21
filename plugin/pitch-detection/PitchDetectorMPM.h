#pragma once

#include <algorithm>
#include <cmath>
#include <juce_dsp/juce_dsp.h>
#include <utility>
#include <vector>

namespace pitch_detection {
/**
 * ((prevPrevTime, prevPrevPitch), (prevTime, prevPitch)).
 * Time in ms, pitch in Hz.
 */
using PrevPitches = std::pair<std::pair<long long, float>, std::pair<long long, float>>;

/**
 * @brief McLeod Pitch Method (MPM) for real-time pitch detection
 * Adapted from https://github.com/sevagh/pitch-detection to work with JUCE
 * Modified for vocal pitch detection!
 */
class PitchDetectorMPM {
  public:
    PitchDetectorMPM(int fftSize = 4096);
    ~PitchDetectorMPM();

    /**
     * @brief Detect pitch from audio buffer
     * @param audioBuffer Audio buffer to analyze (mono)
     * @param sampleRate Sample rate in Hz
     * @param wasSilence If was silence before
     * @return Detected frequency in Hz, or 0.0f if no clear pitch detected
     */
    float detectPitch(const std::vector<float> &audioBuffer, double sampleRate, bool wasSilence);

    /**
     * @brief Reset the detector state
     */
    void reset();

    /**
     * @brief Set voice frequency range for filtering
     * @param minFreq Minimum frequency in Hz (default: 70 Hz)
     * @param maxFreq Maximum frequency in Hz (default: 1500 Hz)
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

    // Buffers for autocorrelation (pre-allocated for real-time)
    std::vector<float> fftBuffer;
    std::vector<float> nsdfBuffer; // Normalized Square Difference Function
    std::vector<int> peakBuffer;   // Reusable buffer for peak positions
    int peakCount;                 // Actual number of peaks in peakBuffer

    // Parameters for detecting and correcting false jumps
    float prevPitch = 0.0f;
    float prevPrevPitch = 0.0f;
    const int maxNumFixes = 3;
    int numFixes = 0;

    void resetJumpsDetection();

    /**
     * @brief Calculate Normalized Square Difference Function (NSDF)
     * @param audioBuffer Input audio buffer
     * @return NSDF values (stored in nsdfBuffer, returns reference)
     */
    const std::vector<float>& calculateNSDF(const std::vector<float> &audioBuffer);

    /**
     * @brief Find peaks in NSDF
     * @param nsdf NSDF values
     * @return Vector of peak positions (stored in peakBuffer, returns reference)
     */
    const std::vector<int>& peakPicking(const std::vector<float> &nsdf);

    /**
     * @brief Parabolic interpolation for better frequency estimation
     * @param data Data array
     * @param peakIndex Index of the peak
     * @return Pair of (interpolated position, interpolated value)
     */
    std::pair<float, float> parabolicInterpolation(const std::vector<float> &data, int peakIndex);
};

} // namespace pitch_detection
