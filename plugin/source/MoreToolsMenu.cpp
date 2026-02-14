#include "XenRoll/MoreToolsMenu.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
MoreToolsMenu::MoreToolsMenu(AudioPluginAudioProcessorEditor *editor) : editor(editor) {
    setAlwaysOnTop(true);
    setWantsKeyboardFocus(false);
    setVisible(false);

    quantizeSelNotesButton = std::make_unique<juce::TextButton>("Quantize selected notes");
    quantizeSelNotesButton->setLookAndFeel(&editor->smallLF);
    quantizeSelNotesButton->setClickingTogglesState(false);
    quantizeSelNotesButton->onClick = [this, editor]() { editor->quantizeSelectedNotes(); };
    addAndMakeVisible(quantizeSelNotesButton.get());

    randSelNotesTimeButton = std::make_unique<juce::TextButton>("Randomize selected notes' timing");
    randSelNotesTimeButton->setLookAndFeel(&editor->smallLF);
    randSelNotesTimeButton->setClickingTogglesState(false);
    randSelNotesTimeButton->onClick = [this, editor]() { editor->randomizeSelectedNotesTiming(); };
    addAndMakeVisible(randSelNotesTimeButton.get());

    randSelNotesVelButton =
        std::make_unique<juce::TextButton>("Randomize selected notes' velocity");
    randSelNotesVelButton->setLookAndFeel(&editor->smallLF);
    randSelNotesVelButton->setClickingTogglesState(false);
    randSelNotesVelButton->onClick = [this, editor]() { editor->randomizeSelectedNotesVelocity(); };
    addAndMakeVisible(randSelNotesVelButton.get());

    // Position elements
    int y = vertPadding;
    quantizeSelNotesButton->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    randSelNotesTimeButton->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    randSelNotesVelButton->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    const int totalHeight = y + vertPadding;
    setSize(width, totalHeight);
}
} // namespace audio_plugin