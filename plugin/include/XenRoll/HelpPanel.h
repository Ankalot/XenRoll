#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class HelpViewport : public juce::Viewport {
  public:
    HelpViewport() {
        setScrollBarsShown(true, false);
        getVerticalScrollBar().setColour(juce::ScrollBar::backgroundColourId, Theme::dark);
        getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, Theme::bright);
        setViewportIgnoreDragFlag(true);
        setAlwaysOnTop(true);
    }

    void paint(juce::Graphics &g) override { g.fillAll(Theme::darker); }
};

class HelpPanel : public juce::Component {
  public:
    HelpPanel() {
        setVisible(false);
        setAlwaysOnTop(true);
    }

    void paint(juce::Graphics &g) override {
        g.fillAll(Theme::darker);

        auto bounds = getLocalBounds().reduced(10);

        // Draw Mouse Actions section
        g.setColour(Theme::brightest);
        g.setFont(Theme::medium);
        g.drawText("Mouse Actions", bounds.removeFromTop(25), juce::Justification::centredLeft,
                   false);
        bounds.removeFromTop(5);

        drawTable(
            g, bounds.removeFromTop(480),
            {{"LCLICK + EMPTY SPACE", "Create a note under cursor or deselect all notes"},
             {"LCLICK + LEFT PANEL WITH KEYS", "Play a key. You also can hold down it; move it"},
             {"LCLICK + DRAG RIGHT SIDE OF NOTE", "Resize note (change its duration)"},
             {"LCLICK + DRAG NOTE", "Drag selected note(-s)"},
             {"LCLICK + NOTE", "Select a note"},
             {"LCLICK + SHIFT + NOTE", "Select a note without deselecting others"},
             {"LCLICK + TOP TIME PANEL", "Turn on zone"},
             {"LCLICK + SHIFT + TOP TIME PANEL", "Create zone point"},
             {"RCLICK + NOTE", "Delete a note"},
             {"RCLICK + DRAG EMPTY SPACE", "Select notes in rectangular area"},
             {"RCLICK + TOP TIME PANEL", "Turn off zone"},
             {"RCLICK + SHIFT + TOP TIME PANEL", "Delete zone point"},
             {"MCLICK + DRAG EMPTY SPACE", "Drag view"},
             {"MCLICK + NOTE", "Change velocity of selected note(-s)"},
             {"SCROLL", "Time zoom in/out"},
             {"SCROLL + CTRL", "Pitch zoom in/out"},
             {"SCROLL + ALT", "Bend selected notes"},
             {"SCROLL + ALT + SHIFT", "Bend selected notes faster"}});

        bounds.removeFromTop(20);

        // Draw Keyboard Actions section
        g.setFont(Theme::medium);
        g.setColour(Theme::brightest);
        g.drawText("Keyboard Actions", bounds.removeFromTop(25), juce::Justification::centredLeft,
                   false);
        bounds.removeFromTop(5);

        drawTable(g, bounds,
                  {{"DEL", "Delete selected notes"},
                   {"ESC", "Deselect all notes"},
                   {"CTRL + A", "Select all notes"},
                   {"CTRL + C", "Copy selected notes"},
                   {"CTRL + V", "Paste copied notes"},
                   {"CTRL + Z", "Discard last change"},
                   {"Z/X/C/.../A/S/D/.../Q/W/E/...", "Play a key"},
                   {"0-9, /, BACKSPACE", "Enter number of cents or ratio"},
                   {"ENTER + CENTS/RATIO", "Set pitch of selected notes"},
                   {"ENTER + SHIFT + CENTS/RATIO", "Increase pitch of selected notes"},
                   {"ENTER + CTRL + CENTS/RATIO", "Decrease pitch of selected notes"},
                   {"UP", "Raise selected notes by 1 cent (or by 1 key in snap mode)"},
                   {"DOWN", "Lower selected notes by 1 cent (or by 1 key in snap mode)"},
                   {"SHIFT + UP", "Raise selected notes up an octave"},
                   {"SHIFT + DOWN", "Lower selected notes by an octave"}});
    }

  private:
    int firstColumnWidth = 300;

    void drawTable(juce::Graphics &g, juce::Rectangle<int> bounds,
                   const std::vector<std::pair<juce::String, juce::String>> &rows) {
        g.setColour(Theme::brighter.withAlpha(0.2f));
        g.fillRect(bounds);

        g.setColour(Theme::brightest);
        g.setFont(Theme::small_);

        int rowHeight = bounds.getHeight() / juce::jmax(1, (int)rows.size());

        for (size_t i = 0; i < rows.size(); ++i) {
            auto rowBounds = bounds.removeFromTop(rowHeight);

            // Draw row background for better readability
            if (i % 2 == 0) {
                g.setColour(Theme::brighter.withAlpha(0.05f));
                g.fillRect(rowBounds);
            }

            g.setColour(Theme::brightest);

            // Draw key combination (narrower column)
            auto keyBounds = rowBounds.removeFromLeft(firstColumnWidth).reduced(5, 2);
            g.drawText(rows[i].first, keyBounds, juce::Justification::centredLeft, false);

            // Draw description (wider column)
            auto descBounds = rowBounds.reduced(5, 2);
            g.drawText(rows[i].second, descBounds, juce::Justification::centredLeft, false);
        }
    }
};
} // namespace audio_plugin