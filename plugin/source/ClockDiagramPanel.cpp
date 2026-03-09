#include "XenRoll/ClockDiagramPanel.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
ClockDiagramPanel::ClockDiagramPanel(Parameters *params, AudioPluginAudioProcessorEditor *editor,
                                     const std::vector<Note> &notes)
    : params(params), editor(editor), notes(notes) {
    setAlwaysOnTop(true);
    setInterceptsMouseClicks(false, false);
    setWantsKeyboardFocus(false);
    setMouseClickGrabsKeyboardFocus(false);

    setSize(2 * (radius + padding), 2 * (radius + padding));
}

void ClockDiagramPanel::refresh() {
    chordsAndNotes.clear();
    allCents.clear();
    std::vector<std::pair<float, int>> actualNotesTimeAndCents;

    for (const Note &note : notes) {
        if ((note.time <= time) && (time < note.time + note.duration)) {
            float cents =
                (note.cents + juce::roundToInt(note.bend * (time - note.time) / note.duration)) %
                1200;
            actualNotesTimeAndCents.push_back(std::make_pair(note.time, cents));
            allCents.insert(cents);
        }
    }
    std::sort(actualNotesTimeAndCents.begin(), actualNotesTimeAndCents.end(),
              [](const std::pair<float, int> &a, const std::pair<float, int> &b) {
                  return a.first < b.first;
              });

    if (actualNotesTimeAndCents.size() != 0) {
        float prevTime = -1e4;
        int numGroups = 0;
        for (const auto &(timeAndCents) : actualNotesTimeAndCents) {
            if (timeAndCents.first <= prevTime + params->maxChordDtimeClockDiagram) {
                chordsAndNotes[numGroups - 1].insert(timeAndCents.second);
            } else {
                chordsAndNotes.push_back({timeAndCents.second});
                numGroups++;
            }
            prevTime = timeAndCents.first;
        }
    }

    repaint();
}

void ClockDiagramPanel::paint(juce::Graphics &g) {
    // Background
    g.setColour(Theme::bright.withAlpha(opacity));
    g.fillEllipse(getLocalBounds().toFloat());

    // 12-EDO ticks
    g.setColour(Theme::darker);
    int centreX = padding + radius;
    int centreY = padding + radius;
    for (int i = 0; i < 12; ++i) {
        float angle = i * juce::MathConstants<float>::twoPi / 12.0f;
        float startX = centreX + std::cos(angle) * (radius - tickLength);
        float startY = centreY - std::sin(angle) * (radius - tickLength);
        float endX = centreX + std::cos(angle) * radius;
        float endY = centreY - std::sin(angle) * radius;
        g.drawLine(startX, startY, endX, endY, Theme::wide);
    }

    // Circle
    g.setColour(Theme::darkest);
    g.drawEllipse(padding, padding, 2 * radius, 2 * radius, Theme::wide);

    // Lines
    g.setColour(Theme::activated);
    for (const auto &chordOrNote : chordsAndNotes) {
        if (chordOrNote.size() == 1) {
            // Single note
            const int cents = *(chordOrNote.begin());
            float angle = juce::MathConstants<float>::twoPi * (1 - cents / 1200.0f) +
                          juce::MathConstants<float>::pi / 2;

            float startX = centreX + std::cos(angle) * (radius - tickLength);
            float startY = centreY - std::sin(angle) * (radius - tickLength);
            float endX = centreX + std::cos(angle) * radius;
            float endY = centreY - std::sin(angle) * radius;

            g.drawLine(startX, startY, endX, endY, Theme::wider);
        } else if (chordOrNote.size() == 2) {
            // Interval
            auto it = chordOrNote.begin();
            int cents1 = *it++;
            int cents2 = *it;
            float angle1 = juce::MathConstants<float>::twoPi * (1 - cents1 / 1200.0f) +
                           juce::MathConstants<float>::pi / 2;
            float angle2 = juce::MathConstants<float>::twoPi * (1 - cents2 / 1200.0f) +
                           juce::MathConstants<float>::pi / 2;

            float x1 = centreX + std::cos(angle1) * radius;
            float y1 = centreY - std::sin(angle1) * radius;
            float x2 = centreX + std::cos(angle2) * radius;
            float y2 = centreY - std::sin(angle2) * radius;

            g.drawLine(x1, y1, x2, y2, Theme::wider);
        } else {
            // Chord
            juce::Path polygon;

            auto it = chordOrNote.begin();
            bool firstPoint = true;

            for (; it != chordOrNote.end(); ++it) {
                int cents = *it;
                float angle = juce::MathConstants<float>::twoPi * (1 - cents / 1200.0f) +
                              juce::MathConstants<float>::pi / 2;

                float x = centreX + std::cos(angle) * radius;
                float y = centreY - std::sin(angle) * radius;

                if (firstPoint) {
                    polygon.startNewSubPath(x, y);
                    firstPoint = false;
                } else {
                    polygon.lineTo(x, y);
                }
            }
            polygon.closeSubPath();

            g.strokePath(polygon, juce::PathStrokeType(Theme::wider));
        }
    }

    // Labels
    g.setColour(Theme::darkest);
    g.setFont(Theme::small_);
    for (const auto &cents : allCents) {
        float angle = juce::MathConstants<float>::twoPi * (1 - cents / 1200.0f) +
                      juce::MathConstants<float>::pi / 2;

        int labelDistance = radius + labelIndent;
        float x = centreX + std::cos(angle) * labelDistance;
        float y = centreY - std::sin(angle) * labelDistance;

        juce::Rectangle<float> textRect(x - 20.0f, y - 10.0f, 40.0f, 20.0f);

        g.drawText(juce::String(cents), textRect, juce::Justification::centred);
    }
}
} // namespace audio_plugin