#include "XenRoll/editor/panels/SettingsPanel.h"
#include "XenRoll/editor/PluginEditor.h"

namespace audio_plugin {
SettingsPanel::SettingsPanel(Parameters *params, AudioPluginAudioProcessorEditor *editor)
    : params(params) {
    setVisible(false);
    setAlwaysOnTop(true);

    // ==================== BASIC SETTINGS ====================
    basicSettingsHeader = std::make_unique<juce::Label>();
    juce::Font headerFont = basicSettingsHeader->getFont();
    headerFont.setHeight(Theme::big);
    basicSettingsHeader->setFont(headerFont);
    basicSettingsHeader->setText("Basic Settings", juce::dontSendNotification);
    basicSettingsHeader->setColour(juce::Label::backgroundColourId, params->theme.darkest);
    addAndMakeVisible(basicSettingsHeader.get());

    startingOctaveLabel = std::make_unique<juce::Label>();
    juce::Font settingFont = startingOctaveLabel->getFont();
    settingFont.setHeight(Theme::medium);
    startingOctaveLabel->setFont(settingFont);
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
    a4FreqLabel->setFont(settingFont);
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

    playDraggedNotesLabel = std::make_unique<juce::Label>();
    playDraggedNotesLabel->setText("Play notes when you place/drag them:",
                                   juce::dontSendNotification);
    playDraggedNotesLabel->setFont(settingFont);
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

    horZoomOnCursorLabel = std::make_unique<juce::Label>();
    horZoomOnCursorLabel->setText("Horizontal zoom on cursor:", juce::dontSendNotification);
    horZoomOnCursorLabel->setFont(settingFont);
    horZoomOnCursorLabel->setTooltip(
        "If active: horizontal zoom will be centered on the mouse cursor.\n"
        "If inactive: horizontal zoom will be centered on the view.");
    addAndMakeVisible(horZoomOnCursorLabel.get());

    horZoomOnCursorCheckbox = std::make_unique<juce::ToggleButton>();
    horZoomOnCursorLabel->attachToComponent(horZoomOnCursorCheckbox.get(), true);
    horZoomOnCursorCheckbox->setToggleState(GlobalSettings::getInstance().getHorZoomOnCursor(),
                                            juce::dontSendNotification);
    horZoomOnCursorCheckbox->onStateChange = [this]() {
        GlobalSettings::getInstance().setHorZoomOnCursor(horZoomOnCursorCheckbox->getToggleState());
    };
    horZoomOnCursorCheckbox->setSize(rowHeight, rowHeight);
    addAndMakeVisible(horZoomOnCursorCheckbox.get());

    // ==================== VISUAL SETTINGS ====================
    visualSettingsHeader = std::make_unique<juce::Label>();
    visualSettingsHeader->setFont(headerFont);
    visualSettingsHeader->setText("Visual Settings", juce::dontSendNotification);
    visualSettingsHeader->setColour(juce::Label::backgroundColourId, params->theme.darkest);
    addAndMakeVisible(visualSettingsHeader.get());

    themeTypeLabel = std::make_unique<juce::Label>();
    themeTypeLabel->setFont(settingFont);
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
        basicSettingsHeader->setColour(juce::Label::backgroundColourId, params->theme.darkest);
        visualSettingsHeader->setColour(juce::Label::backgroundColourId, params->theme.darkest);
        mpeSettingsHeader->setColour(juce::Label::backgroundColourId, params->theme.darkest);
    };
    addAndMakeVisible(themeTypeCombo.get());

    noteRectRoundingLabel = std::make_unique<juce::Label>();
    noteRectRoundingLabel->setText("Note Rectangle Corner Rounding (!):",
                                   juce::dontSendNotification);
    noteRectRoundingLabel->setFont(settingFont);
    noteRectRoundingLabel->setTooltip(
        "A non-zero value may cause lag when there are many bent notes on screen simultaneously.");
    addAndMakeVisible(noteRectRoundingLabel.get());

    noteRectRoundingSlider = std::make_unique<juce::Slider>();
    noteRectRoundingLabel->attachToComponent(noteRectRoundingSlider.get(), true);
    noteRectRoundingSlider->setRange(GlobalSettings::min_noteRectRounding,
                                     GlobalSettings::max_noteRectRounding, 0.01);
    noteRectRoundingSlider->setValue(GlobalSettings::getInstance().getNoteRectRounding());
    noteRectRoundingSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, rowHeight);
    noteRectRoundingSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    noteRectRoundingSlider->setSkewFactorFromMidPoint(0.5f);
    noteRectRoundingSlider->onDragEnd = [this, editor]() {
        GlobalSettings::getInstance().setNoteRectRounding(noteRectRoundingSlider->getValue());
        editor->invalidateNotePathsCache();
    };
    addAndMakeVisible(noteRectRoundingSlider.get());

    heightCoefLabel = std::make_unique<juce::Label>();
    heightCoefLabel->setFont(settingFont);
    heightCoefLabel->setText("Note Rectangle Height Coefficient:", juce::dontSendNotification);
    addAndMakeVisible(heightCoefLabel.get());

    heightCoefSlider = std::make_unique<juce::Slider>();
    heightCoefLabel->attachToComponent(heightCoefSlider.get(), true);
    heightCoefSlider->setRange(params->min_noteRectHeightCoef, params->max_noteRectHeightCoef,
                               0.001);
    heightCoefSlider->setValue(params->noteRectHeightCoef);
    heightCoefSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, rowHeight);
    heightCoefSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    heightCoefSlider->onDragEnd = [this, params, editor]() {
        params->noteRectHeightCoef = static_cast<float>(heightCoefSlider->getValue());
        editor->invalidateNotePathsCache();
    };
    addAndMakeVisible(heightCoefSlider.get());

    constNoteRectHeightLabel = std::make_unique<juce::Label>();
    constNoteRectHeightLabel->setText("Note Rectangle Height is constant:",
                                      juce::dontSendNotification);
    constNoteRectHeightLabel->setFont(settingFont);
    addAndMakeVisible(constNoteRectHeightLabel.get());

    constNoteRectHeightCheckbox = std::make_unique<juce::ToggleButton>();
    constNoteRectHeightLabel->attachToComponent(constNoteRectHeightCheckbox.get(), true);
    constNoteRectHeightCheckbox->setToggleState(params->constNoteRectHeight,
                                                juce::dontSendNotification);
    constNoteRectHeightCheckbox->onStateChange = [this, params, editor]() {
        params->constNoteRectHeight = constNoteRectHeightCheckbox->getToggleState();
        editor->invalidateNotePathsCache();
    };
    constNoteRectHeightCheckbox->setSize(rowHeight, rowHeight);
    addAndMakeVisible(constNoteRectHeightCheckbox.get());

    // =============== MPE TUNING MODE SETTINGS ===============
    mpeSettingsHeader = std::make_unique<juce::Label>();
    mpeSettingsHeader->setFont(headerFont);
    mpeSettingsHeader->setText("MPE Tuning Mode Settings", juce::dontSendNotification);
    mpeSettingsHeader->setColour(juce::Label::backgroundColourId, params->theme.darkest);
    addAndMakeVisible(mpeSettingsHeader.get());

    resetPitchBendOnNoteOffLabel = std::make_unique<juce::Label>();
    resetPitchBendOnNoteOffLabel->setText("Reset pitch bend on note off:",
                                          juce::dontSendNotification);
    resetPitchBendOnNoteOffLabel->setFont(settingFont);
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
    semiBendRangeLabel->setFont(settingFont);
    semiBendRangeLabel->setText("MPE pitch bend range:", juce::dontSendNotification);
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
    channelsEconomyModeMPELabel->setText("MIDI channels economy mode:", juce::dontSendNotification);
    channelsEconomyModeMPELabel->setFont(settingFont);
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
    const int areaWidth = area.getWidth();

    // --- Basic Settings ---
    auto basicHeaderRow = area.removeFromTop(headerRowHeight);
    basicSettingsHeader->setBounds(basicHeaderRow.withTrimmedRight(areaWidth - labelWidth));
    area.removeFromTop(padding);

    auto octaveRow = area.removeFromTop(rowHeight);
    startingOctaveCombo->setBounds(octaveRow.withTrimmedLeft(labelWidth));
    area.removeFromTop(padding);

    auto freqRow = area.removeFromTop(rowHeight);
    a4FreqSlider->setBounds(freqRow.withTrimmedLeft(labelWidth));
    area.removeFromTop(padding);

    auto playRow = area.removeFromTop(rowHeight);
    playDraggedNotesCheckbox->setBounds(playRow.withTrimmedLeft(labelWidth));
    area.removeFromTop(padding);

    auto horZoomRow = area.removeFromTop(rowHeight);
    horZoomOnCursorCheckbox->setBounds(horZoomRow.withTrimmedLeft(labelWidth));
    area.removeFromTop(padding + sectionSpacing);

    // --- Visual Settings ---
    auto visualHeaderRow = area.removeFromTop(headerRowHeight);
    visualSettingsHeader->setBounds(visualHeaderRow.withTrimmedRight(areaWidth - labelWidth));
    area.removeFromTop(padding);

    auto themeRow = area.removeFromTop(rowHeight);
    themeTypeCombo->setBounds(themeRow.withTrimmedLeft(labelWidth));
    area.removeFromTop(padding);

    auto roundingRow = area.removeFromTop(rowHeight);
    noteRectRoundingSlider->setBounds(roundingRow.withTrimmedLeft(labelWidth));
    area.removeFromTop(padding);

    auto heightRow = area.removeFromTop(rowHeight);
    heightCoefSlider->setBounds(heightRow.withTrimmedLeft(labelWidth));
    area.removeFromTop(padding);

    auto constHeightRow = area.removeFromTop(rowHeight);
    constNoteRectHeightCheckbox->setBounds(constHeightRow.withTrimmedLeft(labelWidth));
    area.removeFromTop(padding + sectionSpacing);

    // --- MPE Tuning Mode Settings ---
    auto mpeHeaderRow = area.removeFromTop(headerRowHeight);
    mpeSettingsHeader->setBounds(mpeHeaderRow.withTrimmedRight(areaWidth - labelWidth));
    area.removeFromTop(padding);

    auto bendRangeMPERow = area.removeFromTop(rowHeight);
    semiBendRangeCombo->setBounds(bendRangeMPERow.withTrimmedLeft(labelWidth));
    area.removeFromTop(padding);

    auto resetBendRow = area.removeFromTop(rowHeight);
    resetPitchBendOnNoteOffCheckbox->setBounds(resetBendRow.withTrimmedLeft(labelWidth));
    area.removeFromTop(padding);

    auto chEconMPERow = area.removeFromTop(rowHeight);
    channelsEconomyModeMPECheckbox->setBounds(chEconMPERow.withTrimmedLeft(labelWidth));
}

int SettingsPanel::getRequiredHeight() const {
    return padding + // top padding
           headerRowHeight + padding + 4 * (rowHeight + padding) +
           sectionSpacing + // Basic Settings
           headerRowHeight + padding + 4 * (rowHeight + padding) +
           sectionSpacing +                                       // Visual Settings
           headerRowHeight + padding + 3 * (rowHeight + padding); // MPE Tuning Mode Settings
}

void SettingsPanel::paint(juce::Graphics &g) { g.fillAll(params->theme.darker); }
} // namespace audio_plugin