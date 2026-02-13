#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class Theme {
  public:
    enum ThemeType {
        Purple = 1,
        Green = 2,
        Gray = 3,
        Red = 4,
        Blue = 5,
        Orange = 6,
        Cyan = 7,
        LightBlue = 8
    };

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
        case Blue:
            setBlueTheme();
            break;
        case Orange:
            setOrangeTheme();
            break;
        case Cyan:
            setCyanTheme();
            break;
        case LightBlue:
            setLightBlueTheme();
            break;
        }
    }

    static const juce::Array<juce::String> getThemeNames() {
        return {"Purple", "Green", "Gray", "Red", "Blue", "Orange", "Cyan", "LightBlue"};
    }

    static const juce::Array<juce::String> getThemeDescriptions() {
        return {"SEREGA PIRAT mode", "Nature & flowers", "Standart", "Welcome to hell",
                "On the seabed", "Desert", "Exotic", "Tender UwU"};
    }

  private:
    static void setPurpleTheme();
    static void setGreenTheme();
    static void setGrayTheme();
    static void setRedTheme();
    static void setBlueTheme();
    static void setOrangeTheme();
    static void setCyanTheme();
    static void setLightBlueTheme();
};
} // namespace audio_plugin