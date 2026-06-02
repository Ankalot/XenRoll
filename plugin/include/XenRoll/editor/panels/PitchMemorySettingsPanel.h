#pragma once

#include "XenRoll/data/Parameters.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

class PitchMemorySettingsPanel : public juce::Component {
  public:
    PitchMemorySettingsPanel(Parameters &params, AudioPluginAudioProcessorEditor &editor);

    void resized() override;
    void paint(juce::Graphics &g) override;

  private:
    Parameters &params;
    std::unique_ptr<juce::Label> TVvalForZeroHVLabel, TVaddInfluenceLabel, TVminNonzeroLabel,
        showOnlyHarmonicityLabel;
    std::unique_ptr<juce::Slider> TVvalForZeroHVSlider, TVaddInfluenceSlider, TVminNonzeroSlider;
    std::unique_ptr<juce::Viewport> algoDescrViewport; // Destroyed before algoDescrLabel
    std::unique_ptr<juce::Label> algoDescrLabel;
    std::unique_ptr<juce::ToggleButton> showOnlyHarmonicityCheckbox;

    const int padding = 8;
    const int textBoxWidth = 50;
    const int rowHeight = 28;
    const int labelWidth = 400;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchMemorySettingsPanel)
};
} // namespace audio_plugin