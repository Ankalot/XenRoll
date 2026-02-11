#pragma once

#include "DissonanceMeter.h"
#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class DissonancePlot : public juce::Component {
  public:
    DissonancePlot(Parameters *params, std::shared_ptr<DissonanceMeter> dissonanceMeter)
        : params(params), dissonanceMeter(dissonanceMeter) {
        totalCents = params->plotDissonanceTotalCents;
        threadPool = std::make_unique<juce::ThreadPool>(1);
        updateDissonanceCurve();
    }

    ~DissonancePlot() {
        if (threadPool) {
            threadPool->removeAllJobs(true, -1);
            currentJobId++;
        }
    }

    void paint(juce::Graphics &g) override {
        g.fillAll(Theme::brighter);
        if (loading) {
            drawLoadScreen(g);
        } else {
            drawAxes(g);
            drawDissonanceCurve(g);
        }
    }

    void resized() override { plotArea = getLocalBounds().reduced(margin, margin); }

    void updateTotalCents() {
        totalCents = params->plotDissonanceTotalCents;
        updateDissonanceCurve();
    }

    void updateDissonanceCurve() {
        uint64_t myJobId = ++currentJobId;
        loading = true;
        repaint();

        threadPool->addJob([this, myJobId]() {
            int localTotalCents = totalCents;

            std::array<float, 401> localCurve;
            for (int i = 0; i < 401; ++i) {
                if (currentJobId != myJobId) return;  // Check if we're still current
                localCurve[i] = dissonanceMeter->calcDissonance(localTotalCents, localTotalCents + i * 3);
            }
            
            {
                std::scoped_lock lock(mtx);
                if (currentJobId != myJobId) return;
                dissonanceCurve = localCurve;
                loading = false;
            }           

            // Schedule repaint on the message thread
            juce::MessageManager::callAsync([this]() { repaint(); });
        });
    }

  private:
    Parameters *params;
    std::shared_ptr<DissonanceMeter> dissonanceMeter;
    std::atomic<int> totalCents = 0;
    std::array<float, 401> dissonanceCurve{0.0f}; // i-th index = i*3 cents
    std::mutex mtx;
    juce::Rectangle<int> plotArea;
    const int margin = 50;

    std::atomic<bool> loading{false};
    std::unique_ptr<juce::ThreadPool> threadPool;
    std::atomic<uint64_t> currentJobId{0};

    void drawAxes(juce::Graphics &g) {
        g.setColour(Theme::darkest);

        g.drawRect(plotArea, 1);

        // Draw cents ticks
        g.setFont(Theme::small_);
        for (int cents = 0; cents <= 1200; cents += 100) {
            float xPos = plotArea.getX() + centsToX(cents);
            g.drawLine(xPos, plotArea.getY(), xPos, plotArea.getBottom(), 1.0f);

            // Label
            g.drawText(juce::String(cents), xPos - 20, plotArea.getBottom() + 2, 40, 15,
                       juce::Justification::centred, false);
        }

        // Draw dissonance ticks
        float roughTicks[] = {-1.0f, -0.8f, -0.6f, -0.4f, -0.2f, 0.0f,
                              0.2f,  0.4f,  0.6f,  0.8f,  1.0f};
        for (auto rough : roughTicks) {
            float yPos = plotArea.getBottom() - (rough + 1.0f) / 2 * plotArea.getHeight();
            g.drawLine(plotArea.getX(), yPos, plotArea.getRight(), yPos, 0.5f);

            g.drawText(juce::String(rough, 2), 0, yPos - 8, 50, 15, juce::Justification::right,
                       false);
        }

        // Plot label
        g.setFont(Theme::big);
        g.drawText("Dissonance", 0, 15, getWidth(), Theme::big, juce::Justification::centred,
                   false);

        // Axis labels
        g.setFont(Theme::medium);
        g.drawText("Interval (cents)", 0, getHeight() - Theme::medium - 5, getWidth(),
                   Theme::medium, juce::Justification::centred, false);
    }

    // is triggered only when loading = false
    void drawDissonanceCurve(juce::Graphics &g) {
        g.setColour(Theme::darkest);

        float lastX = plotArea.getX() + centsToX(0);
        float lastY = plotArea.getBottom() - (dissonanceCurve[0] + 1.0f) / 2 * plotArea.getHeight();
        for (int i = 1; i < 401; ++i) {
            float currX = plotArea.getX() + centsToX(i * 3);
            float currY =
                plotArea.getBottom() - (dissonanceCurve[i] + 1.0f) / 2 * plotArea.getHeight();

            g.drawLine(lastX, lastY, currX, currY, Theme::narrow);

            lastX = currX;
            lastY = currY;
        }
    }

    void drawLoadScreen(juce::Graphics &g) {
        g.setFont(50.0f);
        g.drawText("LOADING...", plotArea, juce::Justification::centred);
    }

    float centsToX(int cents) const { return float(cents) / 1200 * plotArea.getWidth(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DissonancePlot)
};

} // namespace audio_plugin