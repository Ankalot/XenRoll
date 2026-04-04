#include "XenRoll/MoreToolsMenu.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
MoreToolsMenu::MoreToolsMenu(AudioPluginAudioProcessorEditor *editor, Theme *theme)
    : editor(editor), theme(theme) {
    setAlwaysOnTop(true);
    setWantsKeyboardFocus(false);
    setVisible(false);

    quantizeSelNotesButton = std::make_unique<juce::TextButton>("Quantize selected notes");
    quantizeSelNotesButton->setLookAndFeel(editor->smallLF.get());
    quantizeSelNotesButton->setClickingTogglesState(false);
    quantizeSelNotesButton->onClick = [this, editor]() { editor->quantizeSelectedNotes(); };
    addAndMakeVisible(quantizeSelNotesButton.get());

    randSelNotesTimeButton = std::make_unique<juce::TextButton>("Randomize selected notes' timing");
    randSelNotesTimeButton->setLookAndFeel(editor->smallLF.get());
    randSelNotesTimeButton->setClickingTogglesState(false);
    randSelNotesTimeButton->onClick = [this, editor]() { editor->randomizeSelectedNotesTiming(); };
    addAndMakeVisible(randSelNotesTimeButton.get());

    randSelNotesVelButton =
        std::make_unique<juce::TextButton>("Randomize selected notes' velocity");
    randSelNotesVelButton->setLookAndFeel(editor->smallLF.get());
    randSelNotesVelButton->setClickingTogglesState(false);
    randSelNotesVelButton->onClick = [this, editor]() { editor->randomizeSelectedNotesVelocity(); };
    addAndMakeVisible(randSelNotesVelButton.get());

    deleteAllRatiosMarksButton = std::make_unique<juce::TextButton>("Delete all ratios marks");
    deleteAllRatiosMarksButton->setLookAndFeel(editor->smallLF.get());
    deleteAllRatiosMarksButton->setClickingTogglesState(false);
    deleteAllRatiosMarksButton->onClick = [this, editor]() { editor->deleteAllRatiosMarks(); };
    addAndMakeVisible(deleteAllRatiosMarksButton.get());

    // Position elements
    int y = vertPadding;
    quantizeSelNotesButton->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    randSelNotesTimeButton->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    randSelNotesVelButton->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    deleteAllRatiosMarksButton->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    const int totalHeight = y + vertPadding;
    setSize(width, totalHeight);
}
} // namespace audio_plugin