#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class PopupMessage : public juce::Component, private juce::Timer {
  public:
    PopupMessage(int displayTimeMs, float alpha) : displayTimeMs(displayTimeMs), alpha(alpha) {
        setAlwaysOnTop(true);
        setInterceptsMouseClicks(false, false);
        setVisible(false);
    }

    void showMessage(const juce::String &text) {
        messageText = text;
        setVisible(true);
        startTimer(displayTimeMs);
        repaint();
    }

    const juce::String &getText() { return messageText; }

    void paint(juce::Graphics &g) override {
        g.setColour(Theme::darkest.withAlpha(alpha));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);

        g.setColour(Theme::brightest);
        g.setFont(Theme::big);
        g.drawText(messageText, getLocalBounds(), juce::Justification::centred, false);
    }

    void timerCallback() override {
        setVisible(false);
        messageText = "";
        stopTimer();
        repaint();
    }

  private:
    int displayTimeMs;
    juce::String messageText = "";
    float alpha;
};
} // namespace audio_plugin