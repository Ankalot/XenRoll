#include "XenRoll/PitchMemorySettingsPanel.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
PitchMemorySettingsPanel::PitchMemorySettingsPanel(Parameters *params,
                                                   AudioPluginAudioProcessorEditor *editor) {
    setVisible(false);
    setAlwaysOnTop(true);

    const juce::String TV_0 = juce::String("TV") + juce::juce_wchar(0x2080);
    const juce::String TV_ADD = juce::String("TV") + juce::juce_wchar(0x2090);
    const juce::String TV_MIN = juce::String("TV") + juce::juce_wchar(0x2098);
    const juce::String TV_i = juce::String("TV") + juce::juce_wchar(0x1D62);
    const juce::String TV_j = juce::String("TV") + juce::juce_wchar(0x2C7C);
    const juce::String HV_i = juce::String("HV") + juce::juce_wchar(0x1D62);
    const juce::String SUM_j = juce::String("SUM") + juce::juce_wchar(0x2C7C);
    const juce::String DV_ij =
        juce::String("DV") + juce::juce_wchar(0x1D62) + juce::juce_wchar(0x2C7C);
    const juce::String bullet = juce::String::fromUTF8("\xE2\x80\xA2"); // •

    TVvalForZeroHVLabel = std::make_unique<juce::Label>();
    juce::Font currentFont = TVvalForZeroHVLabel->getFont();
    currentFont.setHeight(Theme::medium);
    TVvalForZeroHVLabel->setFont(currentFont);
    TVvalForZeroHVLabel->setText("Trace value for zero harmonicity value (" + TV_0 + "):",
                                 juce::dontSendNotification);
    addAndMakeVisible(TVvalForZeroHVLabel.get());

    TVaddInfluenceLabel = std::make_unique<juce::Label>();
    TVaddInfluenceLabel->setFont(currentFont);
    TVaddInfluenceLabel->setText("Additional influence on trace values (" + TV_ADD + "):",
                                 juce::dontSendNotification);
    addAndMakeVisible(TVaddInfluenceLabel.get());

    TVminNonzeroLabel = std::make_unique<juce::Label>();
    TVminNonzeroLabel->setFont(currentFont);
    TVminNonzeroLabel->setText("Min nonzero trace value (" + TV_MIN + "):",
                               juce::dontSendNotification);
    addAndMakeVisible(TVminNonzeroLabel.get());

    showOnlyHarmonicityLabel = std::make_unique<juce::Label>();
    showOnlyHarmonicityLabel->setFont(currentFont);
    showOnlyHarmonicityLabel->setText("Don't show traces (show only harmonicity)",
                               juce::dontSendNotification);
    addAndMakeVisible(showOnlyHarmonicityLabel.get());

    TVvalForZeroHVSlider = std::make_unique<juce::Slider>();
    TVvalForZeroHVLabel->attachToComponent(TVvalForZeroHVSlider.get(), true);
    TVvalForZeroHVSlider->setRange(params->min_pitchMemoryTVvalForZeroHV,
                                   params->max_pitchMemoryTVvalForZeroHV, 0.01);
    TVvalForZeroHVSlider->setValue(params->pitchMemoryTVvalForZeroHV);
    TVvalForZeroHVSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, textBoxWidth,
                                          rowHeight);
    TVvalForZeroHVSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    TVvalForZeroHVSlider->onDragEnd = [this, params, editor]() {
        float newVal = TVvalForZeroHVSlider->getValue();
        if (params->pitchMemoryTVvalForZeroHV != newVal) {
            params->pitchMemoryTVvalForZeroHV = newVal;
            editor->updatePitchMemory();
        }
    };
    addAndMakeVisible(TVvalForZeroHVSlider.get());

    TVaddInfluenceSlider = std::make_unique<juce::Slider>();
    TVaddInfluenceLabel->attachToComponent(TVaddInfluenceSlider.get(), true);
    TVaddInfluenceSlider->setRange(params->min_pitchMemoryTVaddInfluence,
                                   params->max_pitchMemoryTVaddInfluence, 0.01);
    TVaddInfluenceSlider->setValue(params->pitchMemoryTVaddInfluence);
    TVaddInfluenceSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, textBoxWidth,
                                          rowHeight);
    TVaddInfluenceSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    TVaddInfluenceSlider->onDragEnd = [this, params, editor]() {
        float newVal = TVaddInfluenceSlider->getValue();
        if (params->pitchMemoryTVaddInfluence != newVal) {
            params->pitchMemoryTVaddInfluence = newVal;
            editor->updatePitchMemory();
        }
    };
    addAndMakeVisible(TVaddInfluenceSlider.get());

    TVminNonzeroSlider = std::make_unique<juce::Slider>();
    TVminNonzeroLabel->attachToComponent(TVminNonzeroSlider.get(), true);
    TVminNonzeroSlider->setRange(params->min_pitchMemoryTVminNonzero,
                                 params->max_pitchMemoryTVminNonzero, 0.01);
    TVminNonzeroSlider->setValue(params->pitchMemoryTVminNonzero);
    TVminNonzeroSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, textBoxWidth, rowHeight);
    TVminNonzeroSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    TVminNonzeroSlider->onDragEnd = [this, params, editor]() {
        float newVal = TVminNonzeroSlider->getValue();
        if (params->pitchMemoryTVminNonzero != newVal) {
            params->pitchMemoryTVminNonzero = newVal;
            editor->updatePitchMemory();
        }
    };
    addAndMakeVisible(TVminNonzeroSlider.get());

    showOnlyHarmonicityCheckbox = std::make_unique<juce::ToggleButton>();
    showOnlyHarmonicityCheckbox->setToggleState(params->pitchMemoryShowOnlyHarmonicity,
                                                juce::dontSendNotification);
    showOnlyHarmonicityCheckbox->onStateChange = [this, params]() {
        params->pitchMemoryShowOnlyHarmonicity = showOnlyHarmonicityCheckbox->getToggleState();
    };
    showOnlyHarmonicityCheckbox->setSize(rowHeight, rowHeight);
    addAndMakeVisible(showOnlyHarmonicityCheckbox.get());

    algoDescrLabel = std::make_unique<juce::Label>();
    algoDescrLabel->setFont(currentFont);
    juce::String algoDescrText =
        "                   SHORT-TERM PITCH MEMORY DESCRIPTION (author: Ankalot)\n\n"
        "       Introduction:\n" +
        bullet +
        " Each note has harmonicity value (HV) in range [-1; +1] which is shown with "
        "color gradient {red; yellow; green}\n" +
        bullet +
        " Each note leaves a trace in pitch memory (measure of tonality), it has trace "
        "value (TV) in range [0; +1] which is shown with color gradient {black; white}.\n" +
        bullet +
        " HV and TV of the note are derived from interractions between that note and "
        "all TVs. Interactions are determined by the dissonance metric in the menu to the right of "
        "the button with this menu.\n\n"
        "       Algorithm:\n"
        "1. First note has TV = 1 and HV = 1.\n"
        "2. We add notes in the order they are played (if there are several simultaneous notes we "
        "use pitch descending order). When i-th note is added:\n"
        "   2.1 For each existing trace (let's consider j-th trace):\n"
        "       2.1.1 Calculate dissonance value (DV) in [-1; +1] range of interval which is"
        " formed by i-th note and\n                  the j-th trace (using dissonance metric). "
        "Let's call it " +
        DV_ij + ".\n";
    algoDescrText += "   2.2 " + HV_i + " = -" + SUM_j + "(" + TV_j + "*" + DV_ij + ")/" + SUM_j +
                     "(" + TV_j + ")\n";
    algoDescrText +=
        "   2.3 " + TV_i + " = { (" + HV_i + " + 1)*" + TV_0 + ";    if " + HV_i + " <= 0\n";
    algoDescrText += "                      { (1-" + TV_0 + ")*" + HV_i + " + " + TV_0 + ";  if " +
                     HV_i + " > 0\n";
    algoDescrText += "          if " + TV_i + " < " + TV_MIN + ":  " + TV_i + " := 0\n";
    algoDescrText += "   2.4 For each existing trace (let's consider j-th trace):\n";
    algoDescrText += "          " + TV_j + " = min(1.0, " + TV_j + "*2^(min(1.0, " + TV_i + " + " +
                     TV_ADD + ")*" + HV_i + "))\n";
    algoDescrText += "          if " + TV_j + " < " + TV_MIN + ":  " + TV_j + " := 0";
    algoDescrLabel->setText(algoDescrText, juce::dontSendNotification);

    algoDescrViewport = std::make_unique<AlgoDescrViewport>();
    algoDescrViewport->setScrollBarsShown(true, false);
    algoDescrViewport->setViewedComponent(algoDescrLabel.get(), false);
    addAndMakeVisible(algoDescrViewport.get());
}

void PitchMemorySettingsPanel::resized() {
    auto area = getLocalBounds().reduced(padding);

    auto row = area.removeFromTop(rowHeight + padding);
    TVvalForZeroHVLabel->setBounds(row.removeFromLeft(labelWidth));
    TVvalForZeroHVSlider->setBounds(row);

    row = area.removeFromTop(rowHeight + padding);
    TVaddInfluenceLabel->setBounds(row.removeFromLeft(labelWidth));
    TVaddInfluenceSlider->setBounds(row);

    row = area.removeFromTop(rowHeight + padding);
    TVminNonzeroLabel->setBounds(row.removeFromLeft(labelWidth));
    TVminNonzeroSlider->setBounds(row);

    row = area.removeFromTop(rowHeight + padding);
    showOnlyHarmonicityLabel->setBounds(row.removeFromLeft(labelWidth));
    showOnlyHarmonicityCheckbox->setBounds(row);

    area.removeFromTop(padding + juce::roundToInt(Theme::wider));
    algoDescrLabel->setSize(area.getWidth() - 10, 470);
    algoDescrViewport->setBounds(area);
}

void PitchMemorySettingsPanel::paint(juce::Graphics &g) {
    g.fillAll(Theme::darker);
    g.setColour(Theme::darkest);
    g.drawLine(padding, padding * 5 + rowHeight * 4 + Theme::wider, getWidth() - padding,
               padding * 5 + rowHeight * 4 + Theme::wider, Theme::wider);
}
} // namespace audio_plugin