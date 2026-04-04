#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
/**
 * @brief A very small window for changing the velocity of the selected notes(s)
 */
class VelocityPanel : public juce::Component {
  public:
    VelocityPanel(std::function<void()> onValueChange, std::function<void()> onDragEnd) {
        setAlwaysOnTop(true);

        velocitySlider = std::make_unique<juce::Slider>();
        velocitySlider->setRange(0.0, 1.0, 1.0 / 127);
        velocitySlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        velocitySlider->setSliderStyle(juce::Slider::LinearHorizontal);
        velocitySlider->onValueChange = onValueChange;
        velocitySlider->onDragEnd = onDragEnd;
        addAndMakeVisible(velocitySlider.get());
    }

    void resized() override { velocitySlider->setBounds(getLocalBounds().reduced(2)); }

    /**
     * @brief Set the velocity value
     * @param vel Velocity value (0-1)
     */
    void setVelocity(float vel) { velocitySlider->setValue(vel, juce::dontSendNotification); }

    /**
     * @brief Get the current velocity value
     * @return Velocity value (0-1)
     */
    float getVelocity() { return float(velocitySlider->getValue()); }

    void paint(juce::Graphics &g) override {
        g.setColour(Theme::bright);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
    }

  private:
    std::unique_ptr<juce::Slider> velocitySlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityPanel)
};
} // namespace audio_plugin