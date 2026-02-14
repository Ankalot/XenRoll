#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class Theme {
  public:
    enum ThemeType {
        Purple = 1,   ///< Purple theme
        Green = 2,    ///< Green theme
        Gray = 3,     ///< Gray theme
        Red = 4,      ///< Red theme
        Blue = 5,     ///< Blue theme
        Orange = 6,   ///< Orange theme
        Cyan = 7,     ///< Cyan theme
        LightBlue = 8 ///< Light blue theme
    };

    // Theme dependent colors
    static juce::Colour darkest;   ///< Darkest theme color
    static juce::Colour darker;    ///< Darker theme color
    static juce::Colour dark;      ///< Dark theme color
    static juce::Colour bright;    ///< Bright theme color
    static juce::Colour brighter;  ///< Brighter theme color
    static juce::Colour brightest; ///< Brightest theme color
    static juce::Colour activated; ///< Activated element color

    // Theme independent colors
    static juce::Colour maxHarmony; ///< Color for maximum harmonicity
    static juce::Colour midHarmony; ///< Color for mid harmonicity
    static juce::Colour minHarmony; ///< Color for minimum harmonicity

    // Line widths
    static constexpr float wider = 4.0f;    ///< Widest line width
    static constexpr float wide = 3.0f;     ///< Wide line width
    static constexpr float narrow = 2.0f;   ///< Narrow line width
    static constexpr float narrower = 1.0f; ///< Narrowest line width

    // Text sizes
    static constexpr float big = 24.0f;    ///< Big text size
    static constexpr float medium = 20.0f; ///< Medium text size
    static constexpr float small_ = 16.0f; ///< Small text size

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
                "On the seabed",     "Desert",           "Exotic",   "Tender UwU"};
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