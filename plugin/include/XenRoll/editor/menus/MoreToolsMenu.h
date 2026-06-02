#pragma once

#include "XenRoll/data/Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief Small menu with buttons that represent some kind of tools for manipulating notes.
 */
class MoreToolsMenu : public juce::Component {
  public:
    MoreToolsMenu(AudioPluginAudioProcessorEditor &editor, Theme &theme);

    ~MoreToolsMenu() override {};

    void paint(juce::Graphics &g) override {
        g.fillAll(theme.darker);
        g.setColour(theme.darkest);
        g.drawRect(getLocalBounds().toFloat(), Theme::wider);
    }

  private:
    AudioPluginAudioProcessorEditor &editor;
    Theme &theme;
    std::unique_ptr<juce::Button> quantizeSelNotesButton, randSelNotesTimeButton,
        randSelNotesVelButton, deleteAllRatiosMarksButton, mirrorSelNotesHorButton,
        mirrorSelNotesVertButton;

    const int width = 260;
    const int rowHeight = 32;
    const int rowSkip = 0;
    const int horPadding = 2;
    const int vertPadding = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoreToolsMenu)
};
} // namespace audio_plugin