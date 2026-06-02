#pragma once

#include "XenRoll/data/Parameters.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief Small menu for changing the settings for clock diagram
 */
class ClockDiagramMenu : public juce::Component {
  public:
    ClockDiagramMenu(Parameters &params, AudioPluginAudioProcessorEditor &editor);

    ~ClockDiagramMenu() override {};

    void paint(juce::Graphics &g) override {
        g.fillAll(params.theme.darker);
        g.setColour(params.theme.darkest);
        g.drawRect(getLocalBounds().toFloat(), Theme::wider);
    }

  private:
    AudioPluginAudioProcessorEditor &editor;
    Parameters &params;

    std::unique_ptr<juce::Label> maxChordDtimeClockDiagramLabel;
    std::unique_ptr<juce::ComboBox> maxChordDtimeClockDiagramCombo;

    const int width = 250;
    const int horPadding = 15;
    const int vertPadding = 10;
    const int rowHeight = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClockDiagramMenu)
};
} // namespace audio_plugin