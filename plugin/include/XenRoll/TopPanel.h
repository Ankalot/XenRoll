#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief The panel above the main one. Contains bars numbers and a playhead
 */
class TopPanel : public juce::Component {
  public:
    TopPanel(int bar_width_px, const int topPanel_height_px,
             AudioPluginAudioProcessorEditor *editor, Parameters *params);

    void changeBeatWidthPx(int new_bar_widt_px) {
        bar_width_px = new_bar_widt_px;
        this->setSize(params->get_num_bars() * bar_width_px, topPanel_height_px);
        repaint();
    }

    void numBarsChanged() {
        this->setSize(params->get_num_bars() * bar_width_px, topPanel_height_px);
        repaint();
    }

    void setPlayHeadTime(float newPlayHeadTime) {
        playHeadTime = newPlayHeadTime;
        repaint();
    }

    void mouseDown(const juce::MouseEvent &event) override;

    /**
     * @brief Adapt line thickness based on zoom level
     * @param inputThickness Input thickness
     * @return Adapted thickness
     */
    float adaptSize(float inputThickness) {
        return inputThickness * std::min(1.0f, bar_width_px * 1.0f / init_bar_width_px);
    }

    /**
     * @brief Adapt font size based on zoom level
     * @param inputThickness Input font size
     * @return Adapted font size
     */
    float adaptFont(float inputThickness) {
        return inputThickness * std::min(1.0f, (bar_width_px + 1.5f * init_bar_width_px) * (0.4f) /
                                                   init_bar_width_px);
    }

    void paint(juce::Graphics &g) override;

  private:
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    const int topPanel_height_px;
    int init_bar_width_px;
    int bar_width_px;
    float playHeadTime = 0.0f;

    const float zonePoint_collision_width_px = 14;

    const int playHeadWidth = 40;
    const int playHeadHeight = 30;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopPanel)
};
} // namespace audio_plugin