#include "XenRoll/PluginEditor.h"
#include "XenRoll/PitchMemorySettingsPanel.h"

namespace audio_plugin {
PitchMemorySettingsPanel::PitchMemorySettingsPanel(Parameters *params, AudioPluginAudioProcessorEditor *editor) {
    setVisible(false);
    setAlwaysOnTop(true);

    TVvalForZeroHVLabel = std::make_unique<juce::Label>();
    juce::Font currentFont = TVvalForZeroHVLabel->getFont();
    currentFont.setHeight(Theme::medium);
    TVvalForZeroHVLabel->setFont(currentFont);
    TVvalForZeroHVLabel->setText("Trace value for zero harmonicity value:", juce::dontSendNotification);
    addAndMakeVisible(TVvalForZeroHVLabel.get());

    TVaddInfluenceLabel = std::make_unique<juce::Label>();
    TVaddInfluenceLabel->setFont(currentFont);
    TVaddInfluenceLabel->setText("Additional influence on trace values:", juce::dontSendNotification);
    addAndMakeVisible(TVaddInfluenceLabel.get());

    TVminNonzeroLabel = std::make_unique<juce::Label>();
    TVminNonzeroLabel->setFont(currentFont);
    TVminNonzeroLabel->setText("Min nonzero trace value:", juce::dontSendNotification);
    addAndMakeVisible(TVminNonzeroLabel.get());

    TVvalForZeroHVSlider = std::make_unique<juce::Slider>();
    TVvalForZeroHVLabel->attachToComponent(TVvalForZeroHVSlider.get(), true);
    TVvalForZeroHVSlider->setRange(params->min_pitchMemoryTVvalForZeroHV, params->max_pitchMemoryTVvalForZeroHV, 0.01);
    TVvalForZeroHVSlider->setValue(params->pitchMemoryTVvalForZeroHV);
    TVvalForZeroHVSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, textBoxWidth, rowHeight);
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
    TVaddInfluenceSlider->setRange(params->min_pitchMemoryTVaddInfluence, params->max_pitchMemoryTVaddInfluence, 0.01);
    TVaddInfluenceSlider->setValue(params->pitchMemoryTVaddInfluence);
    TVaddInfluenceSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, textBoxWidth, rowHeight);
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
    TVminNonzeroSlider->setRange(params->min_pitchMemoryTVminNonzero, params->max_pitchMemoryTVminNonzero, 0.01);
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
}

void PitchMemorySettingsPanel::paint(juce::Graphics &g) { g.fillAll(Theme::darker); }
} // namespace audio_plugin