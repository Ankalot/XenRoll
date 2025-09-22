#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

class InstancesMenu : public juce::Component {
  public:
    InstancesMenu(const int chIndex, Parameters *params, AudioPluginAudioProcessorEditor *editor);

    ~InstancesMenu() override {};

    void updateColors();

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