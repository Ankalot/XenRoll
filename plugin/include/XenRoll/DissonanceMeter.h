// ========================================== Algorithm ==========================================
// Inspired by this paper: https://doi.org/10.1140/epjp/s13360-022-03456-2
//
// Dissonance is a metric that takes as input two tones that form an interval and produces as
//     output a number in [-1; +1] range. Higher the value <=> more dissonant the interval.
// Consonance = -Dissonance
// Dissonance is a combination of Roughness and Compactness.
//     Dissonance = alpha * Roughness + (1 - alpha) * (-Compactness)
//     Then ((dissonance + 1)/2)^beta*2 - 1
//     where Roughness and Compactness are in [-1; +1] range too
// Compactness model:
//                                 Several options:
//     * Continuous extension of the Tenney norm by Sintel, scaled to [-1; +1] range.
//       https://en.xen.wiki/w/User:Sintel/Validation_of_common_consonance_measures
//       but with frequency dependent parameter k
//     * Continuous extension of geometric indicator, scaled to [-1; +1] range.
//       Max geom indicator value is for 2/1 ratio and geom indicator for 1/1 ratio
//       is set equal to 2/1.
// Roughness model:
//     Base is Sethares Roughness model:
//       matlab: https://sethares.engr.wisc.edu/comprog.html
//       python: https://gist.github.com/endolith/3066664
//     I noticed that adding difference and sum partials improves the result (according to the
//       data from the article from the very beginning). Also in brainstem recordings, when people
//       are stimulated by tone, peaks also occur at frequencies that are not initially in the
//       signal, but which are differences or sums of frequencies that are in the signal.
//     It is scaled to [-1; +1] range, where -1 is minimum value in range [1100; 1300] cents and +1
//       is maximum value in range [0; 200] cents (these mins and maxs are different for different
//       lower tones). Everything that is lower -1 is clamped to -1 (it shows up near 0 cents).

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <mutex>
#include <numeric>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

namespace audio_plugin {
// Vector of partials (frequency in Hz, amplitude)
using partialsVec = std::vector<std::pair<float, float>>;

/**
 * @brief Minimalistic class for storing fractions/ratios (without simplification).
 */
struct Ratio {
    int num, den;

    /**
     * @brief Construct a new Ratio object
     * @param n Numerator of a ratio
     * @param d Denominator of a ratio
     */
    Ratio(int n, int d) : num(n), den(d) {}

    /**
     * @brief Ratio to cents
     */
    float cents() const { return 1200 * log2(to_float()); }

    /**
     * @brief Ratio to float
     */
    float to_float() const { return static_cast<float>(num) / den; }
};

class DissonanceMeter {
  public:
    /**
     * @brief Construct a DissonanceMeter
     * @param partials Vector of partials (frequency in Hz, amplitude)
     * @param partialsTotalCents Total cents value of the tone the partials belong to
     * @param A4freq Reference A4 frequency in Hz
     * @param alpha Weight between roughness and compactness (0 = only compactness, 1 = only
     * roughness)
     * @param beta Power exponent for dissonance curve
     */
    DissonanceMeter(const partialsVec &partials, int partialsTotalCents, float A4freq, float alpha,
                    float beta);

    /**
     * @brief Calculate dissonance between two pitches
     * @param totalCents1 First pitch in total cents (1200 * octave + cents)
     * @param totalCents2 Second pitch in total cents
     * @return Dissonance value in range [-1, 1] (negative = consonant, positive = dissonant)
     */
    float calcDissonance(int totalCents1, int totalCents2);

    /**
     * @brief Set the partials for dissonance calculation
     * @param partials Vector of partials (frequency in Hz, amplitude)
     * @param partialsTotalCents Total cents value of the tone the partials belong to
     */
    void setPartials(const partialsVec &partials, int partialsTotalCents);

    /**
     * @brief Set the weight between roughness and compactness
     * @param newAlpha Weight value (0 = only compactness, 1 = only roughness)
     */
    void setAlpha(float newAlpha);

    /**
     * @brief Set the power exponent for dissonance curve
     * @param newBeta Power value (affects dissonance curve shape)
     */
    void setBeta(float newBeta);

    /**
     * @brief Set the reference A4 frequency
     * @param newA4freq A4 frequency in Hz
     */
    void setA4freq(float newA4freq);

    /**
     * @brief Set the compactness model type
     * @param newCompactnessModel Model name: "Tenney" or "Geom"
     */
    void setCompactnessModel(const std::string &newCompactnessModel);

  private:
    std::shared_mutex mtx;
    float A4freq = 440.0f;
    float alpha = 0.3f;
    float beta = 1.0f;

    // ============================== Compactness ==============================
    std::string compactnessModel = "Tenney"; ///< "Tenney" or "Geom"
    const int maxNumDen = 70;
    std::vector<Ratio> ratios;
    std::map<int, std::pair<float, float>> minMaxCompTenney;
    const float tenneyParWidthCoef = 0.82f;

    /**
     * @brief Calculate geometric compactness between two pitches
     * @param totalCents1 First pitch in total cents
     * @param totalCents2 Second pitch in total cents
     * @return Compactness value (-1 = minimal, 1 = maximal)
     */
    float calcCompactnessGeom(int totalCents1, int totalCents2);

    /**
     * @brief Calculate Tenney compactness between two pitches
     * @param totalCents1 First pitch in total cents
     * @param totalCents2 Second pitch in total cents
     * @return Scaled compactness value (-1 = minimal, 1 = maximal)
     */
    float calcCompactnessTenney(int totalCents1, int totalCents2);

    /**
     * @brief Generate all possible ratios up to maxNumDen
     */
    void makeRatios();

    /**
     * @brief Calculate geometric index normalization for a ratio
     * @param r Ratio to calculate
     * @return Normalized geometric index
     */
    float calcGeomIndNorm(const Ratio &r);

    /**
     * @brief Calculate Equivalent Rectangular Bandwidth (ERB) for a frequency
     * @param f Frequency in Hz
     * @return ERB value in Hz
     */
    float calcERB(float f);

    /**
     * @brief Calculate Difference Limen (DL) for a frequency
     * @param f Frequency in Hz
     * @return DL value
     */
    float calcDL(float f);

    /**
     * @brief Calculate Difference Limen (DL) in cents (sigma) for a frequency
     * @param f Frequency in Hz
     * @return Sigma value in Hz
     */
    float calcSigma(float f);

    /**
     * @brief Calculate Tenney harmonic distance for a ratio
     * @param r Ratio to calculate
     * @return Tenney harmonic distance
     */
    float calcTenney(const Ratio &r);

    /**
     * @brief Calculate unscaled Tenney compactness between two pitches
     * @param totalCents1 First pitch in total cents
     * @param totalCents2 Second pitch in total cents
     * @return Unscaled compactness value
     */
    float calcCompactnessTenneyNotScaled(int totalCents1, int totalCents2);

    // =============================== Roughness ===============================
    const int A4totalCents = 4 * 1200 + 900;
    const int maxNumPartials = 15;
    ///< totalCents of lower tone -> min and max values of not scaled roughness for it
    std::map<int, std::pair<float, float>> minMaxRoughness;
    partialsVec partialsA4;

    /**
     * @brief Calculate scaled roughness between two pitches
     * @param totalCents1 First pitch in total cents
     * @param totalCents2 Second pitch in total cents
     * @param includeSumsDiffs Include sum and difference frequencies in calculation
     * @param diffWeight Weight for difference frequencies (0-1)
     * @param sumWeight Weight for sum frequencies (0-1)
     * @param minSumDiffsAmp Minimum amplitude for sum/difference frequencies
     * @return Scaled roughness value (-1 = minimal, 1 = maximal)
     */
    float calcRoughness(int totalCents1, int totalCents2, bool includeSumsDiffs = true,
                        float diffWeight = 0.5, float sumWeight = 0.3, float minSumDiffsAmp = 0.0);

    /**
     * @brief Calculate unscaled roughness between two pitches
     * @param totalCents1 First pitch in total cents
     * @param totalCents2 Second pitch in total cents
     * @param includeSumsDiffs Include sum and difference frequencies in calculation
     * @param diffWeight Weight for difference frequencies (0-1)
     * @param sumWeight Weight for sum frequencies (0-1)
     * @param minSumDiffsAmp Minimum amplitude for sum/difference frequencies
     * @return Unscaled roughness value
     */
    float calcRoughnessNotScaled(int totalCents1, int totalCents2, bool includeSumsDiffs,
                                 float diffWeight, float sumWeight, float minSumDiffsAmp);
};
} // namespace audio_plugin