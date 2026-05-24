// ========================================== Algorithm ==========================================
// Inspired by "Spectral Analysis, Editing, and Resynthesis: Methods and Applications" (2009)
// by Michael Kateley Klingbeil. Link:
// https://www.klingbeil.com/data/Klingbeil_Dissertation_web.pdf
//      From this dissertation I took:
// 1) 2.4.1 STFT Frequency Interpolation
// 2) 3.1.1 Analysis Window
// 3) 3.1.2 Amplitude Thresholds
//
// I don't analyze partials relative to time (partial tracking) since it is quite complicated and
// redundant for my task. I analyze frequencies only in single window. Methods for finding best
// window position are my own, made up from the head.

#pragma once

#include "XenRoll/data/PartialsTypes.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {
// NOT thread safe, for now this is not required as I only use it in the threadpool
class PartialsFinder {
  public:
    PartialsFinder();
    void setdBThr(float newdBThr);
    void setPosFindStrat(PartialsFindPosStrat pfs);
    void setSampleRate_(double newSampleRate);
    void setFFTSize(int newFFTSize);
    partialsVec findPartials(juce::AudioBuffer<float> buffer);

  private:
    PartialsFindPosStrat posFindStrat = PartialsFindPosStrat::maxSpectralFlatness;
    int fftSize = 8192;
    double sampleRate = 44100; ///< in Hz
    float dBThr = -70.0f;

    std::unique_ptr<juce::dsp::FFT> fft;

    int findPosMidrangeRMS(const juce::AudioBuffer<float> &buffer);
    int findPosMedianRMS(const juce::AudioBuffer<float> &buffer);
    int findPosMinRMSfluct(const juce::AudioBuffer<float> &buffer);
    int findPosMaxSpectralFlatness(const juce::AudioBuffer<float> &buffer);
    int findPosPeakSample(const juce::AudioBuffer<float> &buffer);

    float findSpectralFlatness(const juce::AudioBuffer<float> &buffer, int startSample);
    std::vector<float> findRMSes(const juce::AudioBuffer<float> &buffer);
    float findRMS(const juce::AudioBuffer<float> &buffer, int startSample, int numSamples);

    /**
     * @brief Compute dB threshold for a given frequency
     * @param freq Frequency in Hz
     * @return Threshold in dB
     */
    float computedBThreshold(float freq);
};
} // namespace audio_plugin