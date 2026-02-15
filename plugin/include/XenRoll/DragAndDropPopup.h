#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class DragAndDropPopup : public juce::Component {
  public:
    DragAndDropPopup() {
        setAlwaysOnTop(true);
        setInterceptsMouseClicks(false, false);
        setVisible(false);
    }

    void paint(juce::Graphics &g) override {
        g.setColour(Theme::activated.withAlpha(alpha));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);

        g.setColour(Theme::darkest);
        g.setFont(Theme::big);
        g.drawText("Import", getLocalBounds(), juce::Justification::centred, false);

        g.setColour(Theme::darkest);
        g.setFont(Theme::medium);
        g.drawText(".mid / .mid + .scl / .notes", getLocalBounds().removeFromTop(getHeight() - 50),
                   juce::Justification::centredBottom, false);
    }

  private:
    const float alpha = 0.85f; ///< opacity

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragAndDropPopup)
};
} // namespace audio_plugin