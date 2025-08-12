#pragma once

#include "BinaryData.h"
#include "DissonanceMeter.h"
#include "DissonancePlot.h"
#include "IntegerInput.h"
#include "Parameters.h"
#include "PartialsFinder.h"
#include "PartialsPlot.h"
#include "SVGButton.h"
#include "Theme.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class DissonancePanel : public juce::Component {
  public:
    DissonancePanel(Parameters *params, std::shared_ptr<DissonanceMeter> dissonanceMeter);
    ~DissonancePanel() override;

    void resized() override;
    void paint(juce::Graphics &g) override;

  private:
    Parameters *params;
    std::shared_ptr<DissonanceMeter> dissonanceMeter;
    std::unique_ptr<PartialsPlot> partialsPlot;
    std::unique_ptr<DissonancePlot> dissonancePlot;

    std::unique_ptr<juce::Label> plotPartialsOctaveLabel, plotPartialsCentsLabel,
        plotDissonanceOctaveLabel, plotDissonanceCentsLabel, strategyLabel, fftSizeLabel,
        dBThresholdLabel, compactnessLabel, roughnessLabel, dissonancePowLabel;
    std::unique_ptr<IntegerInput> plotPartialsOctaveInput, plotPartialsCentsInput,
        plotDissonanceOctaveInput, plotDissonanceCentsInput;
    std::unique_ptr<juce::Slider> plotPartialsTotalCentsSlider, plotDissonanceTotalCentsSlider,
        dBThresholdSlider, roughCompactFracSlider, dissonancePowSlider;
    std::unique_ptr<juce::ComboBox> strategyComboBox, fftSizeComboBox;
    std::unique_ptr<SVGButton> switchFindPartialsModeButton, plotPartialsInterpButton,
        refreshButton, trashButton, removePartialsButton, importPartialsButton,
        exportPartialsButton;

    std::unique_ptr<juce::FileChooser> partialsFileChooser;

    const int padding = 15;
    const int lowComponentHeight = 32;
    const int octaveInputWidth = 25;
    const int centsInputWidth = 60;
    const int strategyComboBoxWidth = 340;
    const int fftSizeComboBoxWidth = 90;
    const int sliderTextBoxWidth = 50;
    const int roughCompactFracSliderWidth = 100;

    bool ignoreUpdatePartials = false;
    bool ignoreUpdateDissonance = false;
    void updatePartialsPlotTotalCents(int newTotalCents, int ind);
    void updateDissonancePlotTotalCents(int newTotalCents, int ind);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DissonancePanel)
};
} // namespace audio_plugin