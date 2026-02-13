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
    darkest   = juce::Colour(75, 50, 80);
    darker    = juce::Colour(95, 65, 100);
    dark      = juce::Colour(115, 80, 120);
    bright    = juce::Colour(175, 130, 180);
    brighter  = juce::Colour(200, 150, 215);
    brightest = juce::Colour(255, 255, 255);
    activated = juce::Colour(207, 103, 181);
}

void Theme::setGreenTheme() {
    darkest   = juce::Colour(15, 45, 15);
    darker    = juce::Colour(30, 70, 30);
    dark      = juce::Colour(60, 110, 60);
    bright    = juce::Colour(100, 160, 100);
    brighter  = juce::Colour(160, 210, 160);
    brightest = juce::Colour(230, 250, 230);
    activated = juce::Colour(255, 150, 190);
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
    darkest   = juce::Colour(60, 15, 15);
    darker    = juce::Colour(90, 25, 25);
    dark      = juce::Colour(130, 40, 40);
    bright    = juce::Colour(190, 70, 70);
    brighter  = juce::Colour(230, 140, 140);
    brightest = juce::Colour(250, 240, 240);
    activated = juce::Colour(255, 200, 80);
}

void Theme::setBlueTheme() {
    darkest   = juce::Colour(10, 25, 60);
    darker    = juce::Colour(25, 50, 90);
    dark      = juce::Colour(50, 80, 130);
    bright    = juce::Colour(100, 130, 180);
    brighter  = juce::Colour(160, 190, 230);
    brightest = juce::Colour(240, 245, 255);
    activated = juce::Colour(255, 200, 50);
}

void Theme::setOrangeTheme() {
    darkest   = juce::Colour(70, 40, 15);
    darker    = juce::Colour(110, 60, 25);
    dark      = juce::Colour(160, 90, 40);
    bright    = juce::Colour(215, 140, 70);
    brighter  = juce::Colour(245, 185, 120);
    brightest = juce::Colour(255, 250, 245);
    activated = juce::Colour(200, 240, 140);
}

void Theme::setCyanTheme() {
    darkest   = juce::Colour(5, 50, 50);
    darker    = juce::Colour(10, 80, 80);
    dark      = juce::Colour(20, 120, 120);
    bright    = juce::Colour(60, 170, 170);
    brighter  = juce::Colour(130, 210, 210);
    brightest = juce::Colour(235, 250, 250);
    activated = juce::Colour(255, 150, 100);
}

void Theme::setLightBlueTheme() {
    darkest   = juce::Colour(35, 55, 85);
    darker    = juce::Colour(65, 85, 115);
    dark      = juce::Colour(95, 125, 155);
    bright    = juce::Colour(135, 165, 195);
    brighter  = juce::Colour(175, 205, 230);
    brightest = juce::Colour(225, 240, 250);
    activated = juce::Colour(255, 150, 180);
}
} // namespace audio_plugin