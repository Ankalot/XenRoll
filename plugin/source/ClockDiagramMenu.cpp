#include "XenRoll/ClockDiagramMenu.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
ClockDiagramMenu::ClockDiagramMenu(Parameters *params, AudioPluginAudioProcessorEditor *editor)
    : params(params), editor(editor) {
    setAlwaysOnTop(true);
    setWantsKeyboardFocus(false);
    setVisible(false);

    maxChordDtimeClockDiagramLabel = std::make_unique<juce::Label>();
    juce::Font currentFont = maxChordDtimeClockDiagramLabel->getFont();
    currentFont.setHeight(Theme::small_);
    maxChordDtimeClockDiagramLabel->setText(
        "Max diff between the beginnings\nof notes in a chord (in bars)",
        juce::dontSendNotification);
    maxChordDtimeClockDiagramLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(maxChordDtimeClockDiagramLabel.get());

    maxChordDtimeClockDiagramCombo = std::make_unique<juce::ComboBox>();
    for (int i = 2; i <= 256; i *= 2) {
        maxChordDtimeClockDiagramCombo->addItem("1/" + juce::String(i), i);
    }
    maxChordDtimeClockDiagramCombo->setSelectedId(
        juce::roundToInt(1.0f / params->maxChordDtimeClockDiagram));
    maxChordDtimeClockDiagramCombo->setLookAndFeel(editor->smallLF.get());
    maxChordDtimeClockDiagramCombo->onChange = [this, params, editor]() {
        params->maxChordDtimeClockDiagram = 1.0f / maxChordDtimeClockDiagramCombo->getSelectedId();
        if (params->showClockDiagram) {
            editor->refreshClockDiagramPanel();
        }
    };
    addAndMakeVisible(maxChordDtimeClockDiagramCombo.get());

    int y = vertPadding;

    maxChordDtimeClockDiagramLabel->setBounds(horPadding, y, width - 2 * horPadding, 2 * rowHeight);
    y += 2 * rowHeight;
    maxChordDtimeClockDiagramCombo->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;

    const int totalHeight = y + vertPadding;
    setSize(width, totalHeight);
}
} // namespace audio_plugin