#include "XenRoll/PluginEditor.h"
#include "XenRoll/SettingsPanel.h"

namespace audio_plugin {
SettingsPanel::SettingsPanel(Parameters *params, AudioPluginAudioProcessorEditor *editor) {
    setVisible(false);
    setAlwaysOnTop(true);

    startingOctaveLabel = std::make_unique<juce::Label>();
    juce::Font currentFont = startingOctaveLabel->getFont();
    currentFont.setHeight(Theme::medium);
    startingOctaveLabel->setFont(currentFont);
    startingOctaveLabel->setText("Starting Octave for manual playing:", juce::dontSendNotification);
    addAndMakeVisible(startingOctaveLabel.get());

    startingOctaveCombo = std::make_unique<juce::ComboBox>();
    for (int i = params->min_start_octave; i <= params->max_start_octave; ++i)
        startingOctaveCombo->addItem(juce::String(i), i + 1);
    startingOctaveCombo->setSelectedId(params->start_octave + 1);
    startingOctaveLabel->attachToComponent(startingOctaveCombo.get(), true);
    startingOctaveCombo->onChange = [this, params]() {
        params->start_octave = startingOctaveCombo->getSelectedId() - 1;
    };
    addAndMakeVisible(startingOctaveCombo.get());

    a4FreqLabel = std::make_unique<juce::Label>();
    a4FreqLabel->setText("A4 Frequency (Hz):", juce::dontSendNotification);
    a4FreqLabel->setFont(currentFont);
    addAndMakeVisible(a4FreqLabel.get());

    a4FreqSlider = std::make_unique<juce::Slider>();
    a4FreqLabel->attachToComponent(a4FreqSlider.get(), true);
    a4FreqSlider->setRange(params->min_A4Freq, params->max_A4Freq, 0.01);
    a4FreqSlider->setValue(params->A4Freq.load());
    a4FreqSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 75, rowHeight);
    a4FreqSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    a4FreqSlider->onDragEnd = [this, params, editor]() {
        params->A4Freq.store(a4FreqSlider->getValue());
        editor->A4freqChanged();    };
    addAndMakeVisible(a4FreqSlider.get());

    heightCoefLabel = std::make_unique<juce::Label>();
    heightCoefLabel->setFont(currentFont);
    heightCoefLabel->setText("Note Rectangle Height Coefficient:", juce::dontSendNotification);
    addAndMakeVisible(heightCoefLabel.get());

    heightCoefSlider = std::make_unique<juce::Slider>();
    heightCoefLabel->attachToComponent(heightCoefSlider.get(), true);
    heightCoefSlider->setRange(params->min_noteRectHeightCoef, params->max_noteRectHeightCoef,
                               0.001);
    heightCoefSlider->setValue(params->noteRectHeightCoef);
    heightCoefSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, rowHeight);
    heightCoefSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    heightCoefSlider->onDragEnd = [this, params]() {
        params->noteRectHeightCoef = float(heightCoefSlider->getValue());
    };
    addAndMakeVisible(heightCoefSlider.get());

    themeTypeLabel = std::make_unique<juce::Label>();
    themeTypeLabel->setFont(currentFont);
    themeTypeLabel->setText("Color theme:", juce::dontSendNotification);
    addAndMakeVisible(themeTypeLabel.get());

    themeTypeCombo = std::make_unique<juce::ComboBox>();
    themeTypeCombo->addItemList(Theme::getThemeNames(), 1);
    themeTypeCombo->setSelectedId(static_cast<int>(params->themeType));
    themeTypeLabel->attachToComponent(themeTypeCombo.get(), true);
    themeTypeCombo->onChange = [this, params, editor]() {
        params->themeType = static_cast<Theme::ThemeType>(themeTypeCombo->getSelectedId());
        editor->updateTheme();
    };
    addAndMakeVisible(themeTypeCombo.get());

    playDraggedNotesLabel = std::make_unique<juce::Label>();
    playDraggedNotesLabel->setText("Play notes when you drag them:", juce::dontSendNotification);
    playDraggedNotesLabel->setFont(currentFont);
    addAndMakeVisible(playDraggedNotesLabel.get());

    playDraggedNotesCheckbox = std::make_unique<juce::ToggleButton>();
    playDraggedNotesLabel->attachToComponent(playDraggedNotesCheckbox.get(), true);
    playDraggedNotesCheckbox->setToggleState(params->playDraggedNotes,
                                                juce::dontSendNotification);
    playDraggedNotesCheckbox->onStateChange = [this, params]() {
        params->playDraggedNotes = playDraggedNotesCheckbox->getToggleState();
    };
    playDraggedNotesCheckbox->setSize(rowHeight, rowHeight);
    addAndMakeVisible(playDraggedNotesCheckbox.get());
}

void SettingsPanel::resized() {
    auto area = getLocalBounds().reduced(padding);

    // Starting octave
    auto octaveRow = area.removeFromTop(rowHeight+padding);
    startingOctaveLabel->setBounds(octaveRow.removeFromLeft(labelWidth));
    startingOctaveCombo->setBounds(octaveRow);

    // A4 frequency
    auto freqRow = area.removeFromTop(rowHeight+padding);
    a4FreqLabel->setBounds(freqRow.removeFromLeft(labelWidth));
    a4FreqSlider->setBounds(freqRow);

    // Height coefficient
    auto heightRow = area.removeFromTop(rowHeight+padding);
    heightCoefLabel->setBounds(heightRow.removeFromLeft(labelWidth));
    heightCoefSlider->setBounds(heightRow);

    // Color theme
    auto themeRow = area.removeFromTop(rowHeight+padding);
    themeTypeLabel->setBounds(themeRow.removeFromLeft(labelWidth));
    themeTypeCombo->setBounds(themeRow);

    // Play dragged notes
    auto playRow = area.removeFromTop(rowHeight+padding);
    playDraggedNotesLabel->setBounds(playRow.removeFromLeft(labelWidth));
    playDraggedNotesCheckbox->setBounds(playRow);
}

void SettingsPanel::paint(juce::Graphics &g) { g.fillAll(Theme::darker); }
} // namespace audio_plugin