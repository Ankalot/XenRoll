#pragma once

#include "XenRoll/data/Parameters.h"
#include "XenRoll/data/Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_plugin {

class HelpViewport : public juce::Viewport {
  public:
    HelpViewport(Theme *theme) : theme(theme) {
        setScrollBarsShown(true, false);
        getVerticalScrollBar().setColour(juce::ScrollBar::backgroundColourId, theme->dark);
        getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, theme->bright);
        setViewportIgnoreDragFlag(true);
        setAlwaysOnTop(true);
    }

    /**
     * @brief Update scrollbar colors to match current theme
     */
    void updateColors() {
        getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, theme->bright);
    }

    void paint(juce::Graphics &g) override { g.fillAll(theme->darker); }

  private:
    Theme *theme;
};

class HelpPanel : public juce::Component {
  public:
    HelpPanel(Theme *theme) : theme(theme) {
        setVisible(false);
        setAlwaysOnTop(true);
    }

    void paint(juce::Graphics &g) override {
        g.fillAll(theme->darker);

        auto bounds = getLocalBounds().reduced(10);
        int columnWidth = (bounds.getWidth() - columnGap) / 2;

        auto leftColumn = bounds.removeFromLeft(columnWidth);
        auto rightColumn = bounds.removeFromRight(columnWidth);

        // ==================== MOUSE ACTIONS (LEFT COLUMN) ====================
        drawMainHeader(g, leftColumn, "Mouse Actions");

        drawSubHeader(g, leftColumn, "Main Panel (canvas)");
        drawTable(g, leftColumn,
                  {{"LClick + empty space", "Create a note / deselect all"},
                   {"LClick + note", "Select note"},
                   {"Shift + LClick + note", "Add note to the selection"},
                   {"Ctrl + LClick + note", "Select all notes of the same pitch (+ Shift to add)"},
                   {"LDrag + note", "Drag selected notes"},
                   {"Shift + LDrag + note", "Drag slowly (fine pitch adjustment)"},
                   {"LDrag + note's right edge", "Resize notes (change duration)"},
                   {"Alt + LDrag + note's right edge", "Time-stretch selected notes"},
                   {"RClick + note", "Delete note"},
                   {"Alt + RClick/RDrag", "Audition notes under the cursor"},
                   {"RDrag + empty space", "Rectangle-select notes"},
                   {"MDrag + empty space", "Pan the view"},
                   {"MClick + note", "Change the velocity of selected notes"},
                   {"Scroll", "Zoom in/out (time)"},
                   {"Ctrl + Scroll", "Zoom in/out (pitch)"},
                   {"Alt + Scroll", "Bend selected notes"},
                   {"Alt + Shift + Scroll", "Bend selected notes faster"},
                   {"Alt + Ctrl + Scroll", "Snap-bend selected notes to nearest keys"}});

        drawSubHeader(g, leftColumn, "Top Panel (with time)");
        drawTable(g, leftColumn,
                  {{"Alt + LClick", "Turn zone on"},
                   {"Alt + RClick", "Turn zone off"},
                   {"Shift + LClick", "Create zone point"},
                   {"Shift + RClick", "Delete zone point"},
                   {"Click", "Set playhead time (uses OSC, port " +
                                 juce::String(Parameters::oscPort) + ")"}});

        drawSubHeader(g, leftColumn, "Left Panel (with keys)");
        drawTable(g, leftColumn, {{"LClick", "Play key (hold to sustain)"}});

        // ==================== KEYBOARD ACTIONS (RIGHT COLUMN) ====================
        drawMainHeader(g, rightColumn, "Keyboard Actions");

        drawSubHeader(g, rightColumn, "Basic");
        drawTable(g, rightColumn,
                  {{"Z/X/C...A/S/D...Q/W/E...", "Play keys"},
                   {"Del", "Delete selected notes"},
                   {"Esc", "Deselect all notes"},
                   {"Ctrl/Cmd + A", "Select all notes"},
                   {"Ctrl/Cmd + C", "Copy selected notes"},
                   {"Ctrl/Cmd + V", "Paste copied notes"},
                   {"Ctrl/Cmd + Z", "Undo"},
                   {"Ctrl/Cmd + Y", "Redo"}});

        drawSubHeader(g, rightColumn, "With Cents / Ratio");
        drawTable(g, rightColumn,
                  {{"0-9, /, Backspace", "Enter cents or ratio"},
                   {"Enter", "Set pitch of selected notes (using cents / ratio)"},
                   {"Shift + Enter", "Increase pitch (add cents / ratio)"},
                   {"Ctrl + Enter", "Decrease pitch (subtract cents / ratio)"}});

        drawSubHeader(g, rightColumn, "Moving Notes");
        drawTable(g, rightColumn,
                  {{"Up/Down", "Move by 1 cent (or 1 key in snap mode)"},
                   {"Shift + Up/Down", "Move by 1 octave"},
                   {"Left/Right", "Move by 1 subdivision"},
                   {"Shift + Left/Right", "Move by 1 bar"}});

        drawSubHeader(g, rightColumn, "Hotkeys");
        drawTable(
            g, rightColumn,
            {{"Alt + " + keyToString(Parameters::hotkeys::timeSnap_withAlt),
              "Toggle time snapping"},
             {"Alt + " + keyToString(Parameters::hotkeys::keySnap_withAlt), "Toggle key snapping"},
             {"Alt + " + keyToString(Parameters::hotkeys::editRatiosMarks_withAlt),
              "Toggle ratios marks editing"},
             {"Alt + " + keyToString(Parameters::hotkeys::pitchMemory_withAlt),
              "Toggle harmonicity display"}});

        drawSubHeader(g, rightColumn, "Extra");
        drawTable(g, rightColumn, {{"`", "Show/hide debug overlay (FPS)"}});
    }

    int getRequiredHeight() const {
        const int pad = 20; // 10 top + 10 bottom from reduced(10)
        const int mainH = mainHeaderHeight + 4;
        const int subH = sectionHeaderHeight + 2;

        // Left column: Main Panel (18 rows), Top Panel (5 rows), Left Panel (1 row)
        int left = mainH + subH + 18 * rowHeight + sectionSpacing + subH + 5 * rowHeight +
                   sectionSpacing + subH + 1 * rowHeight;

        // Right column: Basic (8), With Cents (4), Moving Notes (4), Hotkeys (4), Extra (1)
        int right = mainH + subH + 8 * rowHeight + sectionSpacing + subH + 4 * rowHeight +
                    sectionSpacing + subH + 4 * rowHeight + sectionSpacing + subH + 4 * rowHeight +
                    sectionSpacing + subH + rowHeight;

        return pad + juce::jmax(left, right);
    }

  private:
    Theme *theme;
    const int firstColumnWidth = 280;
    const int rowHeight = 22;
    const int sectionHeaderHeight = 22;
    const int mainHeaderHeight = 28;
    const int sectionSpacing = 12;
    const int columnGap = 20;

    static juce::String keyToString(char key) {
        return juce::String::charToString(static_cast<char>(std::toupper(key)));
    }

    void drawMainHeader(juce::Graphics &g, juce::Rectangle<int> &bounds, const juce::String &text) {
        g.setColour(theme->brightest);
        g.setFont(Theme::medium);
        auto headerBounds = bounds.removeFromTop(mainHeaderHeight);
        g.drawText(text, headerBounds, juce::Justification::centredLeft, false);
        bounds.removeFromTop(4);
    }

    void drawSubHeader(juce::Graphics &g, juce::Rectangle<int> &bounds, const juce::String &text) {
        g.setColour(theme->brighter);
        g.setFont(Theme::small_);
        auto headerBounds = bounds.removeFromTop(sectionHeaderHeight);
        g.drawText(text, headerBounds, juce::Justification::centredLeft, false);
        bounds.removeFromTop(2);
    }

    void drawTable(juce::Graphics &g, juce::Rectangle<int> &bounds,
                   const std::vector<std::pair<juce::String, juce::String>> &rows) {
        int tableHeight = static_cast<int>(rows.size()) * rowHeight;
        auto tableBounds = bounds.removeFromTop(tableHeight);

        g.setColour(theme->brighter.withAlpha(0.2f));
        g.fillRect(tableBounds);

        g.setFont(Theme::small_);

        for (size_t i = 0; i < rows.size(); ++i) {
            auto rowBounds = tableBounds.removeFromTop(rowHeight);

            if (i % 2 == 0) {
                g.setColour(theme->brighter.withAlpha(0.05f));
                g.fillRect(rowBounds);
            }

            g.setColour(theme->brightest);

            auto keyBounds = rowBounds.removeFromLeft(firstColumnWidth).reduced(5, 2);
            g.drawText(rows[i].first, keyBounds, juce::Justification::centredLeft, false);

            auto descBounds = rowBounds.reduced(5, 2);
            g.drawText(rows[i].second, descBounds, juce::Justification::centredLeft, false);
        }

        bounds.removeFromTop(sectionSpacing);
    }
};
} // namespace audio_plugin