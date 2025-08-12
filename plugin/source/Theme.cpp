#include "XenRoll/Theme.h"

namespace audio_plugin {
juce::Colour Theme::darkest;
juce::Colour Theme::darker;
juce::Colour Theme::dark;
juce::Colour Theme::bright;
juce::Colour Theme::brighter;
juce::Colour Theme::brightest;
juce::Colour Theme::activated;

juce::Colour Theme::maxHarmony = juce::Colour(0, 255, 0);
juce::Colour Theme::midHarmony = juce::Colour(255, 255, 0);
juce::Colour Theme::minHarmony = juce::Colour(255, 0, 0);

void Theme::setPurpleTheme() {
    darkest   = juce::Colour(82, 54, 84);
    darker    = juce::Colour(99, 66, 102);
    dark      = juce::Colour(118, 77, 121);
    bright    = juce::Colour(169, 124, 171);
    brighter  = juce::Colour(194, 140, 213);
    brightest = juce::Colour(255, 255, 255);
    activated = juce::Colour(207, 103, 181);
}

void Theme::setGreenTheme() {
    darkest   = juce::Colour(20, 54, 1);
    darker    = juce::Colour(36, 85, 1);
    dark      = juce::Colour(75, 131, 31);
    bright    = juce::Colour(115, 169, 66);
    brighter  = juce::Colour(170, 213, 118);
    brightest = juce::Colour(228, 242, 213);
    activated = juce::Colour(255, 173, 199);
}

void Theme::setGrayTheme() {
    darkest   = juce::Colour(38, 38, 38);
    darker    = juce::Colour(61, 61, 61);
    dark      = juce::Colour(75, 75, 75);
    bright    = juce::Colour(145, 145, 145);
    brighter  = juce::Colour(185, 185, 185);
    brightest = juce::Colour(228, 228, 228);
    activated = juce::Colour(19, 189, 153);
}

void Theme::setRedTheme() {
    darkest   = juce::Colour(44, 6, 5);
    darker    = juce::Colour(72, 13, 13);
    dark      = juce::Colour(136, 27, 21);
    bright    = juce::Colour(225, 70, 62);
    brighter  = juce::Colour(243, 156, 156);
    brightest = juce::Colour(253, 251, 251);
    activated = juce::Colour(19, 189, 153);
}
} // namespace audio_plugin