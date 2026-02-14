#include "XenRoll/DissonanceMeter.h"

namespace audio_plugin {
DissonanceMeter::DissonanceMeter(const partialsVec &partials, int partialsTotalCents, float A4freq,
                                 float alpha, float beta)
    : A4freq(A4freq), alpha(alpha), beta(beta) {
    setPartials(partials, partialsTotalCents);
    makeRatios();
}

void DissonanceMeter::setPartials(const partialsVec &partials, int partialsTotalCents) {
    std::unique_lock<std::shared_mutex> lock(mtx);
    // Transpose
    partialsA4 = partials;
    if (partialsA4.empty())
        return;
    for (auto [freq, amp] : partialsA4) {
        freq = freq * std::pow(2, (partialsTotalCents - A4totalCents) / 1200.0);
    }

    // Delete some partials if there are too much
    if (partialsA4.size() > maxNumPartials) {
        std::partial_sort(partialsA4.begin(), partialsA4.begin() + maxNumPartials, partialsA4.end(),
                          [](const auto &p1, const auto &p2) { return p1.second > p2.second; });
        partialsA4.resize(maxNumPartials);
    }

    // Normalize amps
    auto max_it =
        std::max_element(partialsA4.begin(), partialsA4.end(),
                         [](const auto &p1, const auto &p2) { return p1.second < p2.second; });
    float maxAmp = max_it->second;
    if (maxAmp != 0) {
        for (auto &[freq, amp] : partialsA4) {
            amp /= maxAmp;
        }
    }

    minMaxRoughness.clear();
}

void DissonanceMeter::setAlpha(float newAlpha) {
    std::unique_lock<std::shared_mutex> lock(mtx);
    alpha = newAlpha;
}

void DissonanceMeter::setBeta(float newBeta) {
    std::unique_lock<std::shared_mutex> lock(mtx);
    beta = newBeta;
}

void DissonanceMeter::setA4freq(float newA4freq) {
    std::unique_lock<std::shared_mutex> lock(mtx);
    A4freq = newA4freq;
}

void DissonanceMeter::setCompactnessModel(const std::string &newCompactnessModel) {
    if ((newCompactnessModel != "Tenney") && (newCompactnessModel != "Geom"))
        return;
    std::unique_lock<std::shared_mutex> lock(mtx);
    compactnessModel = newCompactnessModel;
}

void DissonanceMeter::makeRatios() {
    for (int a = 1; a < maxNumDen + 1; ++a) {
        for (int b = 1; b < maxNumDen + 1; ++b) {
            if (std::gcd(a, b) == 1) {
                ratios.push_back(Ratio(a, b));
            }
        }
    }
}

float DissonanceMeter::calcGeomIndNorm(const Ratio &r) {
    if ((r.num == 1) && (r.den == 1)) {
        return 1;
    }
    return sqrt(2) / sqrt(r.num * r.den);
}

float DissonanceMeter::calcERB(float f) { return 24.7 * std::log10(4.37 * f / 1000 + 1); }

float DissonanceMeter::calcDL(float f) { return calcERB(f) * 0.35; }

float DissonanceMeter::calcSigma(float f) { return std::log2(calcDL(f) / f + 1) * 1200; }

float DissonanceMeter::calcCompactnessGeom(int totalCents1, int totalCents2) {
    float f1 = A4freq * std::pow(2, (totalCents1 - A4totalCents) / 1200.0);
    int cents = totalCents2 - totalCents1;
    float f2 = f1 * std::pow(2, cents / 1200.0);
    float sigma = calcSigma(f2);

    float compactness = 0;
    for (const auto &r : ratios) {
        float ratioCents = r.cents();
        if ((cents - 2 * sigma < ratioCents) && (ratioCents < cents + 2 * sigma)) {
            float geomInd = calcGeomIndNorm(r);
            float ratioComp = geomInd * std::exp(-0.5 * std::pow((cents - ratioCents) / sigma, 2));
            if (ratioComp > compactness) {
                compactness = ratioComp;
            }
        }
    }

    compactness = compactness * 2 - 1.0f;

    return compactness;
}

float DissonanceMeter::calcTenney(const Ratio &r) { return std::log2(r.num * r.den); }

float DissonanceMeter::calcCompactnessTenneyNotScaled(int totalCents1, int totalCents2) {
    float f1 = A4freq * std::pow(2, (totalCents1 - A4totalCents) / 1200.0);
    int cents = totalCents2 - totalCents1;
    float f2 = f1 * std::pow(2, cents / 1200.0);
    float k = std::max(calcSigma(f2) * tenneyParWidthCoef, 8.0f);

    float compactness = 1e9;
    for (const auto &r : ratios) {
        float ratioCents = r.cents();
        if ((cents - 100 < ratioCents) && (ratioCents < cents + 100)) {
            float tenneyVal = calcTenney(r);
            float ratioComp = tenneyVal + (cents - ratioCents) * (cents - ratioCents) / k / k;
            if (ratioComp < compactness) {
                compactness = ratioComp;
            }
        }
    }

    return compactness;
}

float DissonanceMeter::calcCompactnessTenney(int totalCents1, int totalCents2) {
    float compactness = calcCompactnessTenneyNotScaled(totalCents1, totalCents2);

    auto it = minMaxCompTenney.find(totalCents1);
    float minC, maxC;
    if (it == minMaxCompTenney.end()) {
        minC = 1e9f;
        maxC = -1.0f;
        for (int dcents = 10; dcents < 200; ++dcents) {
            float r = calcCompactnessTenneyNotScaled(totalCents1, totalCents1 + dcents);
            if (r > maxC) {
                maxC = r;
            }
        }
        for (int dcents = 1100; dcents < 1300; ++dcents) {
            float r = calcCompactnessTenneyNotScaled(totalCents1, totalCents1 + dcents);
            if (r < minC) {
                minC = r;
            }
        }
        /*float r = calcCompactnessTenneyNotScaled(f1, 0);
        if (r < minR) {
            minR = r;
        }*/
        minMaxCompTenney.insert({totalCents1, {minC, maxC}});
    } else {
        minC = it->second.first;
        maxC = it->second.second;
    }

    // Scale compactness to [-1; +1] range
    if (compactness > maxC) {
        compactness = maxC;
    } else if (compactness < minC) {
        compactness = minC;
    }
    if (maxC - minC == 0) {
        return 1;
    }
    compactness = 1 - 2 * (compactness - minC) / (maxC - minC);

    return compactness;
}

float DissonanceMeter::calcRoughnessNotScaled(int totalCents1, int totalCents2,
                                              bool includeSumsDiffs, float diffWeight,
                                              float sumWeight, float minSumDiffsAmp) {
    partialsVec partials;
    partials.reserve(partialsA4.size() * 2);

    // Find partials of two tones
    float ratio1 = std::pow(2, (totalCents1 - A4totalCents) / 1200.0);
    float ratio2 = std::pow(2, (totalCents2 - A4totalCents) / 1200.0);
    for (const auto &[freq, amp] : partialsA4) {
        float freq1 = freq * ratio1;
        if ((20 < freq1) && (freq1 < 20000)) {
            partials.push_back({freq1, amp});
        }
        float freq2 = freq * ratio2;
        if ((20 < freq2) && (freq2 < 20000)) {
            partials.push_back({freq2, amp});
        }
    }

    // Find sum and diff partials
    if (includeSumsDiffs) {
        int numPartials = partials.size();
        for (int i = 0; i < numPartials; ++i) {
            for (int j = i + 1; j < numPartials; ++j) {
                float freqDiff = abs(partials[i].first - partials[j].first);
                if (20 < freqDiff) {
                    float ampDiff = diffWeight * std::min(partials[i].second, partials[j].second);
                    if (ampDiff > minSumDiffsAmp)
                        partials.push_back({freqDiff, ampDiff});
                }
                float freqSum = partials[i].first + partials[j].first;
                if (freqSum < 20000) {
                    float ampSum = sumWeight * std::min(partials[i].second, partials[j].second);
                    if (ampSum > minSumDiffsAmp)
                        partials.push_back({freqSum, ampSum});
                }
            }
        }
    }

    // Sort partials by frequency
    std::sort(partials.begin(), partials.end(),
              [](const auto &p1, const auto &p2) { return p1.first < p2.first; });

    // Find roughness
    const float Dstar = 0.24f;
    const float S1 = 0.0207f;
    const float S2 = 18.96f;
    const float C1 = 5.0f;
    const float C2 = -5.0f;
    const float A1 = -3.51f;
    const float A2 = -5.75f;
    float D = 0.0f;
    const int N = partials.size();
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = i + 1; j < N; ++j) {
            const auto &p1 = partials[i];
            const auto &p2 = partials[j];

            const float Fmin = p1.first;
            const float Fdif = p2.first - p1.first;
            const float a = std::min(p1.second, p2.second);

            const float S = Dstar / (S1 * Fmin + S2);
            const float term1 = C1 * std::exp(A1 * S * Fdif);
            const float term2 = C2 * std::exp(A2 * S * Fdif);

            D += a * (term1 + term2);
        }
    }

    return D;
}

float DissonanceMeter::calcRoughness(int totalCents1, int totalCents2, bool includeSumsDiffs,
                                     float diffWeight, float sumWeight, float minSumDiffsAmp) {
    // Find not scaled roughness
    float roughness = calcRoughnessNotScaled(totalCents1, totalCents2, includeSumsDiffs, diffWeight,
                                             sumWeight, minSumDiffsAmp);

    // Find min and max values of roughness for totalCents1
    auto it = minMaxRoughness.find(totalCents1);
    float minR, maxR;
    if (it == minMaxRoughness.end()) {
        minR = 1e9f;
        maxR = -1.0f;
        for (int dcents = 10; dcents < 200; ++dcents) {
            float r = calcRoughnessNotScaled(totalCents1, totalCents1 + dcents, includeSumsDiffs,
                                             diffWeight, sumWeight, minSumDiffsAmp);
            if (r > maxR) {
                maxR = r;
            }
        }
        for (int dcents = 1100; dcents < 1300; ++dcents) {
            float r = calcRoughnessNotScaled(totalCents1, totalCents1 + dcents, includeSumsDiffs,
                                             diffWeight, sumWeight, minSumDiffsAmp);
            if (r < minR) {
                minR = r;
            }
        }
        /*float r = calcRoughnessNotScaled(totalCents1, totalCents1, includeSumsDiffs,
                                         diffWeight, sumWeight, minSumDiffsAmp);
        if (r < minR) {
            minR = r;
        }*/
        minMaxRoughness.insert({totalCents1, {minR, maxR}});
    } else {
        minR = it->second.first;
        maxR = it->second.second;
    }

    // Scale roughness to [-1; +1] range
    if (roughness > maxR) {
        roughness = maxR;
    } else if (roughness < minR) {
        roughness = minR;
    }
    if (maxR - minR == 0) {
        return -1;
    }
    roughness = 2 * (roughness - minR) / (maxR - minR) - 1;

    return roughness;
}

float DissonanceMeter::calcDissonance(int totalCents1, int totalCents2) {
    std::shared_lock<std::shared_mutex> lock(mtx);

    float roughness = 0;
    if (alpha != 0) {
        roughness = calcRoughness(totalCents1, totalCents2);
    }

    float compactness = 0;
    if (alpha != 1.0) {
        if (compactnessModel == "Geom")
            compactness = calcCompactnessGeom(totalCents1, totalCents2);
        else if (compactnessModel == "Tenney")
            compactness = calcCompactnessTenney(totalCents1, totalCents2);
    }

    float dissonance = alpha * roughness + (1 - alpha) * (-compactness);

    dissonance = std::pow((dissonance + 1) / 2, beta) * 2 - 1;

    return dissonance;
}
} // namespace audio_plugin