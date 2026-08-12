#pragma once

#include <JuceHeader.h>
#include <ehl/juce_design/EhlDesign.h>
#include "PluginProcessor.h"

class BerzmowAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit BerzmowAudioProcessorEditor (BerzmowAudioProcessor&);
    ~BerzmowAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateDisplay();

    BerzmowAudioProcessor& audioProcessor;

    ehl::juce_design::LookAndFeel lookAndFeel;
    ehl::juce_design::ParameterDisplay display{ehl::juce_design::DisplayKind::distortion};

    juce::Slider drive, feedback, tone, reso, noiseMix, output;
    juce::ToggleButton danger, limiterBypass;
    juce::Label driveLabel, feedbackLabel, toneLabel, resoLabel, noiseMixLabel, outputLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> aDrive, aFb, aTone, aReso, aNoise, aOut;
    std::unique_ptr<ButtonAttachment> aDanger, aLimiterBypass;

    void initKnob (juce::Slider& s, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BerzmowAudioProcessorEditor)
};
