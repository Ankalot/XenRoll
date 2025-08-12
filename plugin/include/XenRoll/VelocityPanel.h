#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class VelocityPanel : public juce::Component {
  public:
    VelocityPanel(std::function<void()> onValueChange) {
        setAlwaysOnTop(true);

        velocitySlider = std::make_unique<juce::Slider>();
        velocitySlider->setRange(0.0, 127.0, 1.0);
        velocitySlider->setValue(100.0);
        velocitySlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        velocitySlider->setSliderStyle(juce::Slider::LinearHorizontal);
        velocitySlider->onValueChange = onValueChange;
        addAndMakeVisible(velocitySlider.get());
    }

    void resized() override { velocitySlider->setBounds(getLocalBounds().reduced(2)); }

    void setVelocity(float vel) { velocitySlider->setValue(vel * 127.0); }

    float getVelocity() { return float(velocitySlider->getValue() / 127.0); }

    void paint(juce::Graphics &g) override {
        g.setColour(Theme::bright);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
    }

  private:
    std::unique_ptr<juce::Slider> velocitySlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityPanel)
};
} // namespace audio_plugin