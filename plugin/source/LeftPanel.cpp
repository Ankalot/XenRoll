#include "XenRoll/PluginEditor.h"
#include "XenRoll/LeftPanel.h"

namespace audio_plugin {
LeftPanel::LeftPanel(int octave_height_px, int leftPanel_width_px,
                     AudioPluginAudioProcessorEditor *editor, Parameters *params)
    : octave_height_px(octave_height_px), leftPanel_width_px(leftPanel_width_px), editor(editor),
      params(params) {
    this->setSize(leftPanel_width_px, params->num_octaves * octave_height_px);
    setWantsKeyboardFocus(false);
}

void LeftPanel::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isLeftButtonDown()) {
        if (keys.empty()) {
            return;
        }
        juce::Point<int> point = event.getPosition();
        int octave = params->num_octaves - 1 - point.getY() / octave_height_px;
        int cents =
            static_cast<int>(round(
                (1.0f - (point.getY() % octave_height_px) * 1.0f / octave_height_px) * 1200)) %
            1200;
        if (cents == 0) {
            octave += 1;
            if (octave == params->num_octaves) {
                return;
            }
        }
        std::tie(octave, cents) = centsToKeysCents(octave, cents);
        manuallyPlayedNoteTotalCents.insert(octave * 1200 + cents);
        editor->setManuallyPlayedNotesTotalCents(manuallyPlayedNoteTotalCents);
    }
    editor->bringBackKeyboardFocus();
}

void LeftPanel::mouseUp(const juce::MouseEvent &event) {
    if (event.mods.isLeftButtonDown()) {
        manuallyPlayedNoteTotalCents.clear();
        editor->setManuallyPlayedNotesTotalCents(manuallyPlayedNoteTotalCents);
    }
}

void LeftPanel::mouseDrag(const juce::MouseEvent &event) {
    if (event.mods.isLeftButtonDown()) {
        if (keys.empty()) {
            return;
        }
        juce::Point<int> point = event.getPosition();
        int octave = params->num_octaves - 1 - point.getY() / octave_height_px;
        int cents =
            static_cast<int>(round(
                (1.0f - (point.getY() % octave_height_px) * 1.0f / octave_height_px) * 1200)) %
            1200;
        if (cents == 0) {
            octave += 1;
            if (octave == params->num_octaves) {
                return;
            }
        }
        std::tie(octave, cents) = centsToKeysCents(octave, cents);
        int totalCents = octave * 1200 + cents;
        if (!manuallyPlayedNoteTotalCents.contains(totalCents)) {
            manuallyPlayedNoteTotalCents.clear();
            manuallyPlayedNoteTotalCents.insert(totalCents);
            editor->setManuallyPlayedNotesTotalCents(manuallyPlayedNoteTotalCents);
        }
    }
}

void LeftPanel::mouseMove(const juce::MouseEvent &event) {
    juce::ignoreUnused(event);
    if (keys.empty()) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    } else {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
}

std::tuple<int, int> LeftPanel::centsToKeysCents(int octave, int cents) {
    int nearest = 0;
    int minDist = 10000;
    for (const int &key : keys) {
        if (abs(key - cents) < minDist) {
            minDist = abs(key - cents);
            nearest = key;
        }
    }
    if ((octave != 0) && (abs(1200 + cents - *keys.rbegin()) < minDist)) {
        octave--;
        nearest = *keys.rbegin();
    }
    if ((octave != params->num_octaves + 1) && (abs(1200 + *keys.begin() - cents) < minDist)) {
        octave++;
        nearest = *keys.begin();
    }
    return {octave, nearest};
}
} // namespace audio_plugin