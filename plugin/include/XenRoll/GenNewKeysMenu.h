#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;


class GenNewKeysMenu : public juce::Component {
  public:
    GenNewKeysMenu(Parameters *params, AudioPluginAudioProcessorEditor *editor);

    ~GenNewKeysMenu() override {};

    void updateColors();

    void paint(juce::Graphics &g) override {
        g.fillAll(Theme::darker);
        g.setColour(Theme::darkest);
        g.drawRect(getLocalBounds(), Theme::wider);
    }

  private:
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    std::unique_ptr<juce::Label> numNewKeysLabel, minDistExistNewKeysLabel,
        minDistBetweenNewKeysLabel;
    std::unique_ptr<juce::Slider> numNewKeysSlider, minDistExistNewKeysSlider,
        minDistBetweenNewKeysSlider;

    std::unique_ptr<juce::Label> genNewKeysTacticsLabel;
    std::unique_ptr<juce::ComboBox> genNewKeysTacticsCombo;

    const int width = 250;
    const int rowSkip = 10;
    const int horPadding = 15;
    const int vertPadding = 10;
    const int rowHeight = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenNewKeysMenu)
};
} // namespace audio_plugin