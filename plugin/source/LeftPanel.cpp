#include "XenRoll/LeftPanel.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
LeftPanel::LeftPanel(int octave_height_px, int leftPanel_width_px,
                     AudioPluginAudioProcessorEditor *editor, Parameters *params)
    : octave_height_px(octave_height_px), leftPanel_width_px(leftPanel_width_px), editor(editor),
      params(params), init_octave_height_px(octave_height_px) {
    this->setSize(leftPanel_width_px, params->num_octaves * octave_height_px);
    setWantsKeyboardFocus(false);
}

void LeftPanel::paint(juce::Graphics &g) {
    g.fillAll(Theme::bright);

    // rectangles for keys that are played
    g.setColour(Theme::brighter);
    for (int i = 0; i < params->num_octaves; ++i) {
        for (const int &key : keys) {
            float yPos = (i + 1.0f - float(key) / 1200) * octave_height_px;
            bool keyIsPlaying = false;
            for (int totalCents : currPlayedNotesTotalCents) {
                if (1200 * (params->num_octaves - i - 1) + key == totalCents) {
                    keyIsPlaying = true;
                    break;
                }
            }
            if (keyIsPlaying) {
                g.fillRoundedRectangle(juce::Rectangle<float>(0.0f, yPos - adaptKeyHeight(10.0f),
                                                              float(leftPanel_width_px),
                                                              adaptKeyHeight(20.0f)),
                                       4.0f);
            }
        }
    }

    // octaves
    g.setColour(Theme::darkest);
    for (int i = 0; i <= params->num_octaves; ++i) {
        float yPos = float(i * octave_height_px);
        g.drawLine(0, yPos, 40, yPos, adaptSize(Theme::wide));
        g.drawLine((float)leftPanel_width_px - 20, yPos, (float)leftPanel_width_px, yPos,
                   adaptSize(Theme::wide));
    }

    // keys
    g.setFont(adaptFont(Theme::medium));
    juce::String keyText = juce::String::fromUTF8("⤬⤬⤬");
    for (int i = 0; i < params->num_octaves; ++i) {
        int j = 0;
        for (const int &key : keys) {
            float yPos = (i + 1.0f - float(key) / 1200) * octave_height_px;
            g.setColour(Theme::darkest);
            g.drawLine(leftPanel_width_px - 20.0f, yPos, float(leftPanel_width_px), yPos,
                       adaptSize(Theme::narrow));
            if (params->showKeysHarmonicity) {
                const int totalCents = key + 1200 * (params->num_octaves - i - 1);
                if (keysHarmonicity.contains(totalCents)) {
                    const float keyHarm = keysHarmonicity[totalCents];
                    if (keyHarm > 0) {
                        g.setColour(Theme::midHarmony.interpolatedWith(Theme::maxHarmony, keyHarm));
                    } else {
                        g.setColour(
                            Theme::midHarmony.interpolatedWith(Theme::minHarmony, -keyHarm));
                    }
                }
            }
            if (!params->hideCents) {
                keyText = juce::String(key);
            }
            g.drawText(keyText,
                       juce::Rectangle<int>(leftPanel_width_px - 145 + 42 * ((j + 1) % 2),
                                            (int)yPos - 9, 80, 20),
                       juce::Justification::right, false);
            j++;
        }
    }

    // octaves labels
    g.setFont(adaptFont(Theme::big));
    for (int i = 0; i < params->num_octaves; ++i) {
        float yPos = float(i * octave_height_px);
        juce::Graphics::ScopedSaveState state(g);
        g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, 15,
                                                       yPos + octave_height_px / 2));
        g.drawText("OCTAVE " + juce::String(params->num_octaves - i - 1),
                   juce::Rectangle<int>(-100, (int)yPos, 230, octave_height_px),
                   juce::Justification::centred, false);
    }
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