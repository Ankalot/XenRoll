#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

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

    void paint(juce::Graphics &g) override;

  private:
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    const int topPanel_height_px;
    int bar_width_px;
    float playHeadTime = 0.0f;

    const float zonePoint_collision_width_px = 14;

    const int playHeadWidth = 40;
    const int playHeadHeight = 30;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopPanel)
};
} // namespace audio_plugin