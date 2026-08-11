#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class BerzmowAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BerzmowAudioProcessorEditor (BerzmowAudioProcessor&);
    ~BerzmowAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BerzmowAudioProcessor& audioProcessor;

    juce::Slider drive, feedback, tone, reso, noiseMix, output;
    juce::ToggleButton danger, limiterBypass;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> aDrive, aFb, aTone, aReso, aNoise, aOut;
    std::unique_ptr<ButtonAttachment> aDanger, aLimiterBypass;

    void initKnob (juce::Slider& s, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BerzmowAudioProcessorEditor)
};
