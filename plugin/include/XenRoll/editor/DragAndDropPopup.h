#pragma once

#include "XenRoll/data/Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_plugin {
class DragAndDropPopup : public juce::Component {
  public:
    DragAndDropPopup(Theme &theme) : theme(theme) {
        setInterceptsMouseClicks(false, false);
        setVisible(false);
    }

    void paint(juce::Graphics &g) override {
        g.setColour(theme.activated.withAlpha(alpha));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);

        g.setColour(theme.darkest);
        g.setFont(Theme::big);
        g.drawText("Import", getLocalBounds(), juce::Justification::centred, false);

        g.setColour(theme.darkest);
        g.setFont(Theme::medium);
        g.drawText(".mid / .mid + .scl / .notes", getLocalBounds().removeFromTop(getHeight() - 50),
                   juce::Justification::centredBottom, false);
    }

  private:
    Theme &theme;
    const float alpha = 0.85f; ///< opacity

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragAndDropPopup)
};
} // namespace audio_plugin