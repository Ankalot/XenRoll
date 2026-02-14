#pragma once

#include "Note.h"
#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief The panel is to the left of the main one. There are keys and octaves drawn on it.
 */
class LeftPanel : public juce::Component {
  public:
    LeftPanel(int octave_height_px, int leftPanel_width_px, AudioPluginAudioProcessorEditor *editor,
              Parameters *params);

    void changeOctaveHeightPx(int new_octave_height_px) {
        octave_height_px = new_octave_height_px;
        this->setSize(leftPanel_width_px, params->num_octaves * octave_height_px);
        repaint();
    }

    /**
     * @brief Adapt line thickness based on zoom level
     * @param inputThickness Input thickness
     * @return Adapted thickness
     */
    float adaptSize(float inputThickness) {
        return inputThickness * std::min(1.0f, octave_height_px * 1.0f / init_octave_height_px);
    }

    /**
     * @brief Adapt font size based on zoom level
     * @param inputThickness Input font size
     * @return Adapted font size
     */
    float adaptFont(float inputThickness) {
        return inputThickness * std::min(1.0f, (octave_height_px + init_octave_height_px) * 0.5f /
                                                   init_octave_height_px);
    }

    /**
     * @brief Adapt key height based on zoom level
     * @param inputThickness Input height
     * @return Adapted height
     */
    float adaptKeyHeight(float inputThickness) {
        return inputThickness * std::min(1.0f, (octave_height_px + 0.5f * init_octave_height_px) *
                                                   (2.0f / 3) / init_octave_height_px);
    }

    void paint(juce::Graphics &g) override;

    /**
     * @brief Update the keys in this panel
     * @param new_keys Set of new keys
     * @note Repaints panel
     */
    void updateKeys(const std::set<int> &new_keys) {
        keys = new_keys;
        repaint();
    }

    /**
     * @brief Set the currently played notes
     * @param newCurrPlayedNotesTotalCents Set of currently played notes (in total cents)
     * @note Repaints panel (if these currently played notes are new)
     */
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

    /**
     * @brief Update keys harmonicity
     * @param newKeysHarmonicity Map of total cents to harmonicity values
     * @note Repaints panel
     */
    void updateKeysHarmonicity(const std::map<int, float> newKeysHarmonicity) {
        keysHarmonicity = newKeysHarmonicity;
        repaint();
    }

  private:
    const int leftPanel_width_px;
    int init_octave_height_px;
    int octave_height_px;
    std::set<int> keys; ///< 0-1199 cents

    std::set<int> currPlayedNotesTotalCents;
    std::set<int> manuallyPlayedNoteTotalCents;

    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    std::map<int, float> keysHarmonicity = {}; ///< total cents -> harmonicity

    /**
     * @brief Convert octave and cents to octave and nearest key cents
     * @param octave Octave number
     * @param cents Cents within octave
     * @return Tuple of (octave, cents of nearest key)
     */
    std::tuple<int, int> centsToKeysCents(int octave, int cents);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftPanel)
};
} // namespace audio_plugin