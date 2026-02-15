#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

// Is needed for scrolling
class AlgoDescrViewport : public juce::Viewport {
  public:
    AlgoDescrViewport() { updateColors(); }

    /**
     * @brief Update scrollbar colors to match current theme
     */
    void updateColors() {
        getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, Theme::bright);
    }
};

class PitchMemorySettingsPanel : public juce::Component {
  public:
    PitchMemorySettingsPanel(Parameters *params, AudioPluginAudioProcessorEditor *editor);

    void resized() override;
    void paint(juce::Graphics &g) override;

    /**
     * @brief Update colors to match current theme
     */
    void updateColors() { algoDescrViewport->updateColors(); }

  private:
    std::unique_ptr<juce::Label> TVvalForZeroHVLabel, TVaddInfluenceLabel, TVminNonzeroLabel,
        showOnlyHarmonicityLabel;
    std::unique_ptr<juce::Slider> TVvalForZeroHVSlider, TVaddInfluenceSlider, TVminNonzeroSlider;
    std::unique_ptr<AlgoDescrViewport> algoDescrViewport;  // Destroyed before algoDescrLabel
    std::unique_ptr<juce::Label> algoDescrLabel;
    std::unique_ptr<juce::ToggleButton> showOnlyHarmonicityCheckbox;

    const int padding = 15;
    const int textBoxWidth = 50;
    const int rowHeight = 32;
    const int labelWidth = 400;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchMemorySettingsPanel)
};
} // namespace audio_plugin