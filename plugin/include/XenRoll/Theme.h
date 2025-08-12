#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class Theme {
  public:
    enum ThemeType { Purple = 1, Green = 2, Gray = 3, Red = 4 };

    // Theme dependent
    static juce::Colour darkest;
    static juce::Colour darker;
    static juce::Colour dark;
    static juce::Colour bright;
    static juce::Colour brighter;
    static juce::Colour brightest;
    static juce::Colour activated;

    // Theme independent colors
    static juce::Colour maxHarmony;
    static juce::Colour midHarmony;
    static juce::Colour minHarmony;

    // For lines
    static constexpr float wider = 4.0f;
    static constexpr float wide = 3.0f;
    static constexpr float narrow = 2.0f;
    static constexpr float narrower = 1.0f;

    // For text
    static constexpr float big = 24.0f;
    static constexpr float medium = 20.0f;
    static constexpr float small_ = 16.0f;

    static void setTheme(ThemeType theme) {
        switch (theme) {
        case Purple:
            setPurpleTheme();
            break;
        case Green:
            setGreenTheme();
            break;
        case Gray:
            setGrayTheme();
            break;
        case Red:
            setRedTheme();
            break;
        }
    }

    static const juce::Array<juce::String> getThemeNames() {
        return {"Purple", "Green", "Gray", "Red"};
    }

  private:
    static void setPurpleTheme();
    static void setGreenTheme();
    static void setGrayTheme();
    static void setRedTheme();
};
} // namespace audio_plugin