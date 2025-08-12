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
#include <numeric>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

namespace audio_plugin {
using partialsVec = std::vector<std::pair<float, float>>;

// Minimalistic, no simplification, I don't need it
struct Ratio {
    int num, den;

    Ratio(int n, int d) : num(n), den(d) {}

    float cents() const { return 1200 * log2(to_float()); }

    double to_float() const { return static_cast<float>(num) / den; }
};

class DissonanceMeter {
  public:
    DissonanceMeter(const partialsVec &partials, int partialsTotalCents, float A4freq, float alpha,
                    float beta);

    float calcDissonance(int totalCents1, int totalCents2);

    void setPartials(const partialsVec &partials, int partialsTotalCents);
    void setAlpha(float newAlpha);
    void setBeta(float newBeta);
    void setA4freq(float newA4freq);
    // "Tenney" or "Geom"
    void setCompactnessModel(const std::string &newCompactnessModel);

  private:
    std::shared_mutex mtx;
    float A4freq = 440.0f;
    float alpha = 0.3f;
    float beta = 1.0f;

    // ============================== Compactness ==============================
    std::string compactnessModel = "Tenney"; // "Tenney" or "Geom"
    const int maxNumDen = 70;
    std::vector<Ratio> ratios;
    std::map<int, std::pair<float, float>> minMaxCompTenney;
    const float tenneyParWidthCoef = 0.82f;

    float calcCompactnessGeom(int totalCents1, int totalCents2);
    float calcCompactnessTenney(int totalCents1, int totalCents2);

    void makeRatios();
    float calcGeomIndNorm(const Ratio &r);
    float calcERB(float f);
    float calcDL(float f);
    float calcSigma(float f);
    float calcTenney(const Ratio &r);
    float calcCompactnessTenneyNotScaled(int totalCents1, int totalCents2);

    // =============================== Roughness ===============================
    const int A4totalCents = 4 * 1200 + 900;
    const int maxNumPartials = 15;
    // totalCents of lower tone -> min and max values of not scaled roughness for it
    std::map<int, std::pair<float, float>> minMaxRoughness;
    partialsVec partialsA4;

    float calcRoughness(int totalCents1, int totalCents2, bool includeSumsDiffs = true,
                        float diffWeight = 0.5, float sumWeight = 0.3, float minSumDiffsAmp = 0.0);
    float calcRoughnessNotScaled(int totalCents1, int totalCents2, bool includeSumsDiffs,
                                 float diffWeight, float sumWeight, float minSumDiffsAmp);
};
} // namespace audio_plugin