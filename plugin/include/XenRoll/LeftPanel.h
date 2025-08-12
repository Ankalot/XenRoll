#pragma once

#include "Note.h"
#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

class LeftPanel : public juce::Component {
  public:
    LeftPanel(int octave_height_px, int leftPanel_width_px, AudioPluginAudioProcessorEditor *editor,
              Parameters *params);

    void changeOctaveHeightPx(int new_octave_height_px) {
        octave_height_px = new_octave_height_px;
        this->setSize(leftPanel_width_px, params->num_octaves * octave_height_px);
        repaint();
    }

    void paint(juce::Graphics &g) override {
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
                    g.fillRoundedRectangle(
                        juce::Rectangle<float>(0.0f, yPos - 10, float(leftPanel_width_px), 20.0f),
                        4.0f);
                }
            }
        }

        // octaves
        g.setColour(Theme::darkest);
        for (int i = 0; i <= params->num_octaves; ++i) {
            float yPos = float(i * octave_height_px);
            g.drawLine(0, yPos, (float)leftPanel_width_px, yPos, Theme::wide);
        }

        // keys
        g.setFont(Theme::medium);
        for (int i = 0; i < params->num_octaves; ++i) {
            int j = 0;
            for (const int &key : keys) {
                float yPos = (i + 1.0f - float(key) / 1200) * octave_height_px;
                g.setColour(Theme::darkest);
                g.drawLine(leftPanel_width_px - 20.0f, yPos, float(leftPanel_width_px), yPos,
                           Theme::narrow);
                if (params->showKeysHarmonicity) {
                    const int totalCents = key + 1200*(params->num_octaves - i - 1);
                    if (keysHarmonicity.contains(totalCents)) {
                        const float keyHarm = keysHarmonicity[totalCents];
                        if (keyHarm > 0) {
                            g.setColour(Theme::midHarmony.interpolatedWith(Theme::maxHarmony, keyHarm));
                        } else {
                            g.setColour(Theme::midHarmony.interpolatedWith(Theme::minHarmony, -keyHarm));
                        }
                    }
                }
                g.drawText(juce::String(key),
                           juce::Rectangle<int>(leftPanel_width_px - 106 + 42 * ((j + 1) % 2),
                                                (int)yPos - 10, 80, 20),
                           juce::Justification::left, false);
                j++;
            }
        }

        // octaves labels
        g.setFont(Theme::big);
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

    void updateKeys(const std::set<int> &new_keys) {
        keys = new_keys;
        repaint();
    }

    void setAllCurrPlayedNotesTotalCents(const std::set<int> &newCurrPlayedNotesTotalCents) {
        if (currPlayedNotesTotalCents != newCurrPlayedNotesTotalCents) {
            currPlayedNotesTotalCents = newCurrPlayedNotesTotalCents;
            repaint();
        }
    }

    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseMove(const juce::MouseEvent &event) override;

    void updateKeysHarmonicity(const std::map<int, float> newKeysHarmonicity) {
        keysHarmonicity = newKeysHarmonicity;
        repaint();
    }

  private:
    const int leftPanel_width_px;
    int octave_height_px;
    std::set<int> keys;

    std::set<int> currPlayedNotesTotalCents;
    std::set<int> manuallyPlayedNoteTotalCents;

    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    std::map<int, float> keysHarmonicity = {};

    std::tuple<int, int> centsToKeysCents(int octave, int cents);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftPanel)
};
} // namespace audio_plugin