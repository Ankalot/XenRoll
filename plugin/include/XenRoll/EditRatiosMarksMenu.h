#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief Small menu for managing ratios marks editing mode settings
 */
class EditRatiosMarksMenu : public juce::Component {
  public:
    EditRatiosMarksMenu(Parameters *params, AudioPluginAudioProcessorEditor *editor);

    ~EditRatiosMarksMenu() override {};

    void paint(juce::Graphics &g) override {
        g.fillAll(Theme::darker);
        g.setColour(Theme::darkest);
        g.drawRect(getLocalBounds(), Theme::wider);
    }

  private:
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    std::unique_ptr<juce::Label> maxDenLabel, goodEnoughErrorLabel;
    std::unique_ptr<juce::Slider> maxDenSlider, goodEnoughErrorSlider;

    std::unique_ptr<juce::Label> autoCorrectLabel;
    std::unique_ptr<juce::ToggleButton> autoCorrectCheckbox;

    const int width = 250;
    const int rowSkip = 10;
    const int horPadding = 15;
    const int vertPadding = 10;
    const int rowHeight = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditRatiosMarksMenu)
};
} // namespace audio_plugin