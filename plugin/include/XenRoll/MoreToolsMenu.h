#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief Small menu with buttons that represent some kind of tools for manipulating notes.
 */
class MoreToolsMenu : public juce::Component {
  public:
    MoreToolsMenu(AudioPluginAudioProcessorEditor *editor);

    ~MoreToolsMenu() override {};

    void paint(juce::Graphics &g) override {
        g.fillAll(Theme::darker);
        g.setColour(Theme::darkest);
        g.drawRect(getLocalBounds(), Theme::wider);
    }

  private:
    AudioPluginAudioProcessorEditor *editor;
    std::unique_ptr<juce::Button> quantizeSelNotesButton, randSelNotesTimeButton,
        randSelNotesVelButton, deleteAllRatiosMarksButton;

    const int width = 260;
    const int rowHeight = 32;
    const int rowSkip = 0;
    const int horPadding = 2;
    const int vertPadding = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoreToolsMenu)
};
} // namespace audio_plugin