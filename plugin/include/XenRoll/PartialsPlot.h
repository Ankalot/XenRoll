#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

// Todo: add info if this is transposed of smth or not
//       add info about num of partials

namespace audio_plugin {
using partialsVec = std::vector<std::pair<float, float>>;

class PartialsPlot : public juce::Component {
  public:
    PartialsPlot(Parameters *params, bool interpolate) : params(params), interpolate(interpolate) {
        totalCents = params->plotPartialsTotalCents;
        tonesPartials = params->get_tonesPartials();
        updatePartials();
    }

    void paint(juce::Graphics &g) override {
        g.fillAll(Theme::brighter);
        drawAxes(g);
        drawPartials(g);
    }

    void resized() override { plotArea = getLocalBounds().reduced(margin, margin); }

    void setInterpMode(bool newInterpolate) {
        interpolate = newInterpolate;
        updateTotalCents();
    }

    void updateTotalCents() {
        totalCents = params->plotPartialsTotalCents;
        updatePartials();
        repaint();
    }

    void updateTonesPartials() {
        tonesPartials = params->get_tonesPartials();
        updatePartials();
        repaint();
    }

  private:
    Parameters *params;
    std::map<int, partialsVec> tonesPartials;
    int totalCents = 0;
    partialsVec partials;
    int totalCentsRef = -1;
    bool interpolate;
    float maxAmp = 1.0f;
    juce::Rectangle<int> plotArea;
    const int margin = 50;
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;

    void updatePartials() {
        if (tonesPartials.empty()) {
            partials = {};
            totalCentsRef = -1;
            return;
        }

        auto it = tonesPartials.find(totalCents);
        if (it != tonesPartials.end()) {
            // Case 1: totalCents exists
            partials = it->second;
            totalCentsRef = -1;
        } else {
            // Case 2: Find totalCentsRef nearest to totalCents
            totalCentsRef = -1;
            int minDiff = INT_MAX;
            for (const auto &tonePartials : tonesPartials) {
                int currDiff = std::abs(tonePartials.first - totalCents);
                if (currDiff < minDiff) {
                    minDiff = currDiff;
                    totalCentsRef = tonePartials.first;
                }
            }
            partials = tonesPartials[totalCentsRef];
            if (interpolate) {
                // Used tranposed partials
                int dCents = totalCents - totalCentsRef;
                float scaleFactor = std::pow(2, dCents / 1200.0);
                for (auto &[freq, amp] : partials) {
                    freq *= scaleFactor;
                }
                // Remove partials with <20 or >20000 freq
                partials.erase(std::remove_if(partials.begin(), partials.end(),
                                              [](const auto &p) {
                                                  return p.first < 20.0f || p.first > 20000.0f;
                                              }),
                               partials.end());
            } else {
                // Use partials from totalCentsRef
                totalCents = totalCentsRef;
                totalCentsRef = -1;
            }
        }

        if (!partials.empty()) {
            maxAmp = 0.0f;
            minFreq = 20.0f;
            maxFreq = 20000.0f;
            for (const auto &[freq, amp] : partials) {
                if (amp > maxAmp)
                    maxAmp = amp;
                if (freq < minFreq)
                    minFreq = freq;
                if (freq > maxFreq)
                    maxFreq = freq;
            }
            minFreq = juce::jmax(20.0f, minFreq / 1.1f);
            maxFreq = juce::jmin(20000.0f, maxFreq * 1.1f);
        }
    }

    void drawAxes(juce::Graphics &g) {
        g.setColour(Theme::darkest);

        g.drawRect(plotArea, 1);

        const int numFreqTicks = 10;
        std::vector<float> freqTicks(numFreqTicks);
        float logMinFreq = log(minFreq);
        float logMaxFreq = log(maxFreq);
        float step = (logMaxFreq - logMinFreq) / (numFreqTicks - 1);
        for (int i = 0; i < numFreqTicks; ++i) {
            freqTicks[i] = round(exp(logMinFreq + i * step));
        }
        // float freqTicks[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};

        g.setFont(Theme::small_);
        for (auto freq : freqTicks) {
            if (freq < minFreq || freq > maxFreq)
                continue;

            float xPos = plotArea.getX() + freqToX(freq);
            g.drawLine(xPos, plotArea.getY(), xPos, plotArea.getBottom(), 1.0f);

            // Label
            juce::String label;
            if (freq >= 1000) {
                label = juce::String(freq / 1000.0f, 1) + "k";
            } else {
                label = juce::String(freq);
            }

            g.drawText(label, xPos - 20, plotArea.getBottom() + 2, 40, 15,
                       juce::Justification::centred, false);
        }

        // Draw amplitude ticks (linear scale)
        float ampTicks[] = {0.0f,          maxAmp * 0.2f, maxAmp * 0.4f,
                            maxAmp * 0.6f, maxAmp * 0.8f, maxAmp};

        for (auto amp : ampTicks) {
            float yPos = plotArea.getBottom() - amp / maxAmp * plotArea.getHeight();
            g.drawLine(plotArea.getX(), yPos, plotArea.getRight(), yPos, 0.5f);

            g.drawText(juce::String(amp, 2), 0, yPos - 8, 50, 15, juce::Justification::right,
                       false);
        }

        // Axis labels
        g.setFont(Theme::medium);

        g.drawText("Frequency (Hz)", 0, getHeight() - Theme::medium - 5, getWidth(), Theme::medium,
                   juce::Justification::centred, false);
    }

    void drawPartials(juce::Graphics &g) {
        g.setColour(Theme::darkest);

        // Plot label
        g.setFont(Theme::big);
        juce::String plotLabel = "Partials | " + juce::String(totalCents / 1200) + "oct " +
                                 juce::String(totalCents % 1200) + "¢";
        if (totalCentsRef != -1) {
            plotLabel += " | partials from " + juce::String(totalCentsRef / 1200) + "oct " +
                         juce::String(totalCentsRef % 1200) + "¢";
        }
        plotLabel += " | Num: " + juce::String(partials.size());

        g.drawText(plotLabel, 0, 15, getWidth(), Theme::big, juce::Justification::centred, false);

        // Partials
        for (const auto &[freq, amp] : partials) {
            if (freq < minFreq || freq > maxFreq)
                continue;

            float xPos = plotArea.getX() + freqToX(freq);
            float yPos = plotArea.getBottom() - amp / maxAmp * plotArea.getHeight();

            g.drawLine(xPos, plotArea.getBottom(), xPos, yPos, Theme::wider);
        }
    }

    float freqToX(float freq) const {
        // Convert frequency to logarithmic x-position
        float logMin = std::log10(minFreq);
        float logMax = std::log10(maxFreq);
        float normalized = (std::log10(freq) - logMin) / (logMax - logMin);
        return normalized * plotArea.getWidth();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PartialsPlot)
};

} // namespace audio_plugin