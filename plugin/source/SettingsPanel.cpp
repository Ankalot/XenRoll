#include "XenRoll/SettingsPanel.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
SettingsPanel::SettingsPanel(Parameters *params, AudioPluginAudioProcessorEditor *editor)
    : params(params) {
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
    a4FreqLabel->setTooltip("If MPE tuning mode: either change A4 freq here, or change it in "
                            "the synthesizer, but not everywhere!");
    addAndMakeVisible(a4FreqLabel.get());

    a4FreqSlider = std::make_unique<juce::Slider>();
    a4FreqLabel->attachToComponent(a4FreqSlider.get(), true);
    a4FreqSlider->setRange(params->min_A4Freq, params->max_A4Freq, 0.01);
    a4FreqSlider->setValue(params->A4Freq.load());
    a4FreqSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 75, rowHeight);
    a4FreqSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    a4FreqSlider->onDragEnd = [this, params, editor]() {
        params->A4Freq.store(a4FreqSlider->getValue());
        editor->A4freqChanged();
    };
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

    constNoteRectHeightLabel = std::make_unique<juce::Label>();
    constNoteRectHeightLabel->setText("Note Rectangle Height is constant:",
                                      juce::dontSendNotification);
    constNoteRectHeightLabel->setFont(currentFont);
    addAndMakeVisible(constNoteRectHeightLabel.get());

    constNoteRectHeightCheckbox = std::make_unique<juce::ToggleButton>();
    constNoteRectHeightLabel->attachToComponent(constNoteRectHeightCheckbox.get(), true);
    constNoteRectHeightCheckbox->setToggleState(params->constNoteRectHeight,
                                                juce::dontSendNotification);
    constNoteRectHeightCheckbox->onStateChange = [this, params]() {
        params->constNoteRectHeight = constNoteRectHeightCheckbox->getToggleState();
    };
    constNoteRectHeightCheckbox->setSize(rowHeight, rowHeight);
    addAndMakeVisible(constNoteRectHeightCheckbox.get());

    themeTypeLabel = std::make_unique<juce::Label>();
    themeTypeLabel->setFont(currentFont);
    themeTypeLabel->setText("Color theme:", juce::dontSendNotification);
    addAndMakeVisible(themeTypeLabel.get());

    themeTypeCombo = std::make_unique<juce::ComboBox>();
    auto themeNames = Theme::getThemeNames();
    auto themeDescriptions = Theme::getThemeDescriptions();
    for (int i = 0; i < themeNames.size(); ++i) {
        juce::String itemText = themeNames[i] + " - " + themeDescriptions[i];
        themeTypeCombo->addItem(itemText, i + 1);
    }
    themeTypeCombo->setSelectedId(static_cast<int>(params->themeType));
    themeTypeLabel->attachToComponent(themeTypeCombo.get(), true);
    themeTypeCombo->onChange = [this, params, editor]() {
        params->themeType = static_cast<Theme::ThemeType>(themeTypeCombo->getSelectedId());
        editor->updateTheme();
    };
    addAndMakeVisible(themeTypeCombo.get());

    playDraggedNotesLabel = std::make_unique<juce::Label>();
    playDraggedNotesLabel->setText("Play notes when you place/drag them:",
                                   juce::dontSendNotification);
    playDraggedNotesLabel->setFont(currentFont);
    addAndMakeVisible(playDraggedNotesLabel.get());

    playDraggedNotesCheckbox = std::make_unique<juce::ToggleButton>();
    playDraggedNotesLabel->attachToComponent(playDraggedNotesCheckbox.get(), true);
    playDraggedNotesCheckbox->setToggleState(GlobalSettings::getInstance().getPlayDraggedNotes(),
                                             juce::dontSendNotification);
    playDraggedNotesCheckbox->onStateChange = [this]() {
        GlobalSettings::getInstance().setPlayDraggedNotes(
            playDraggedNotesCheckbox->getToggleState());
    };
    playDraggedNotesCheckbox->setSize(rowHeight, rowHeight);
    addAndMakeVisible(playDraggedNotesCheckbox.get());

    resetPitchBendOnNoteOffLabel = std::make_unique<juce::Label>();
    resetPitchBendOnNoteOffLabel->setText("Reset pitch bend on note off (MPE only):",
                                          juce::dontSendNotification);
    resetPitchBendOnNoteOffLabel->setFont(currentFont);
    addAndMakeVisible(resetPitchBendOnNoteOffLabel.get());

    resetPitchBendOnNoteOffCheckbox = std::make_unique<juce::ToggleButton>();
    resetPitchBendOnNoteOffLabel->attachToComponent(resetPitchBendOnNoteOffCheckbox.get(), true);
    resetPitchBendOnNoteOffCheckbox->setToggleState(params->resetPitchBendOnNoteOff,
                                                    juce::dontSendNotification);
    resetPitchBendOnNoteOffCheckbox->onStateChange = [this, params]() {
        params->resetPitchBendOnNoteOff = resetPitchBendOnNoteOffCheckbox->getToggleState();
    };
    resetPitchBendOnNoteOffCheckbox->setSize(rowHeight, rowHeight);
    addAndMakeVisible(resetPitchBendOnNoteOffCheckbox.get());

    semiBendRangeLabel = std::make_unique<juce::Label>();
    semiBendRangeLabel->setFont(currentFont);
    semiBendRangeLabel->setText("MPE pitch bend range (MPE only):", juce::dontSendNotification);
    semiBendRangeLabel->setTooltip(
        "1. It should be the same as in synth!\n2. Lower range means higher accuracy in tuning BUT "
        "lower max bend for bended notes.");
    addAndMakeVisible(semiBendRangeLabel.get());

    semiBendRangeCombo = std::make_unique<juce::ComboBox>();
    semiBendRangeCombo->addItem(juce::juce_wchar(0x00B1) + juce::String("12 semitones"), 12);
    semiBendRangeCombo->addItem(juce::juce_wchar(0x00B1) + juce::String("24 semitones"), 24);
    semiBendRangeCombo->addItem(juce::juce_wchar(0x00B1) + juce::String("48 semitones"), 48);
    semiBendRangeCombo->addItem(juce::juce_wchar(0x00B1) + juce::String("96 semitones"), 96);
    semiBendRangeCombo->setSelectedId(params->semiBendRangeMPE);
    semiBendRangeLabel->attachToComponent(semiBendRangeCombo.get(), true);
    semiBendRangeCombo->onChange = [this, params]() {
        params->semiBendRangeMPE = semiBendRangeCombo->getSelectedId();
    };
    addAndMakeVisible(semiBendRangeCombo.get());

    channelsEconomyModeMPELabel = std::make_unique<juce::Label>();
    channelsEconomyModeMPELabel->setText("MIDI channels economy mode (MPE only):",
                                         juce::dontSendNotification);
    channelsEconomyModeMPELabel->setFont(currentFont);
    channelsEconomyModeMPELabel->setTooltip(
        "If active: notes with no bend that have same {cents % 100} will occupy same MIDI "
        "channels. So the maximum number of simultaneously playing notes can be more than 15. It "
        "will cause errors if the synth does not support polyphony on each MIDI channel in MPE "
        "mode!");
    addAndMakeVisible(channelsEconomyModeMPELabel.get());

    channelsEconomyModeMPECheckbox = std::make_unique<juce::ToggleButton>();
    channelsEconomyModeMPELabel->attachToComponent(channelsEconomyModeMPECheckbox.get(), true);
    channelsEconomyModeMPECheckbox->setToggleState(params->channelsEconomyModeMPE,
                                                   juce::dontSendNotification);
    channelsEconomyModeMPECheckbox->onStateChange = [this, params, editor]() {
        params->channelsEconomyModeMPE = channelsEconomyModeMPECheckbox->getToggleState();
        editor->changedChannelsEconomyModeMPE();
    };
    channelsEconomyModeMPECheckbox->setSize(rowHeight, rowHeight);
    addAndMakeVisible(channelsEconomyModeMPECheckbox.get());
}

void SettingsPanel::resized() {
    auto area = getLocalBounds().reduced(padding);

    // Starting octave
    auto octaveRow = area.removeFromTop(rowHeight + padding);
    startingOctaveCombo->setBounds(octaveRow.withTrimmedLeft(labelWidth));

    // A4 frequency
    auto freqRow = area.removeFromTop(rowHeight + padding);
    a4FreqSlider->setBounds(freqRow.withTrimmedLeft(labelWidth));

    // Height coefficient
    auto heightRow = area.removeFromTop(rowHeight + padding);
    heightCoefSlider->setBounds(heightRow.withTrimmedLeft(labelWidth));

    // Const height of notes' rectangles
    auto constHeightRow = area.removeFromTop(rowHeight + padding);
    constNoteRectHeightCheckbox->setBounds(constHeightRow.withTrimmedLeft(labelWidth));

    // Color theme
    auto themeRow = area.removeFromTop(rowHeight + padding);
    themeTypeCombo->setBounds(themeRow.withTrimmedLeft(labelWidth));

    // Play dragged notes
    auto playRow = area.removeFromTop(rowHeight + padding);
    playDraggedNotesCheckbox->setBounds(playRow.withTrimmedLeft(labelWidth));

    // Reset pitch bend on note off (MPE)
    auto resetBendRow = area.removeFromTop(rowHeight + padding);
    resetPitchBendOnNoteOffCheckbox->setBounds(resetBendRow.withTrimmedLeft(labelWidth));

    // Semi bend range (MPE)
    auto bendRangeMPERow = area.removeFromTop(rowHeight + padding);
    semiBendRangeCombo->setBounds(bendRangeMPERow.withTrimmedLeft(labelWidth));

    // Channels economy mode (MPE)
    auto chEconMPERow = area.removeFromTop(rowHeight + padding);
    channelsEconomyModeMPECheckbox->setBounds(chEconMPERow.withTrimmedLeft(labelWidth));
}

void SettingsPanel::paint(juce::Graphics &g) { g.fillAll(params->theme.darker); }
} // namespace audio_plugin