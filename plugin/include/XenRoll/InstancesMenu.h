#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief Small menu for selecting other instances of this plugin whose notes should be viewed (as
 * ghost notes)
 */
class InstancesMenu : public juce::Component {
  public:
    /**
     * @brief Construct an InstancesMenu
     * @param chIndex Current MIDI channel index (0-15)
     * @param params Pointer to parameters
     * @param editor Pointer to the plugin editor
     */
    InstancesMenu(const int chIndex, Parameters *params, AudioPluginAudioProcessorEditor *editor);

    ~InstancesMenu() override {};

    void paint(juce::Graphics &g) override {
        g.fillAll(Theme::darker);
        g.setColour(Theme::darkest);
        g.drawRect(getLocalBounds(), Theme::wider);
    }

    // void resized() override {};

  private:
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;
    const int chIndex;
    std::unique_ptr<juce::Label> titleLabel;
    // actually there are 15 of them but for the sake of simplicity i make array with size 16
    std::array<std::unique_ptr<juce::Label>, 16> channelsLabels;
    std::array<std::unique_ptr<juce::ToggleButton>, 16> channelsCheckboxes;

    bool onTab = false;
    bool onMenu = false;

    const int timerMs = 200;
    const int width = 200;
    const int titleHeight = 32;
    const int rowHeight = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstancesMenu)
};
} // namespace audio_plugin