#include "PluginEditor.h"

BerzmowAudioProcessorEditor::BerzmowAudioProcessorEditor (BerzmowAudioProcessor& p)
: AudioProcessorEditor (&p), audioProcessor (p)
{
    initKnob (drive,    "Drive");
    initKnob (feedback, "FB");
    initKnob (tone,     "Tone");
    initKnob (reso,     "Reso");
    initKnob (noiseMix, "Noise");
    initKnob (output,   "Out");

    danger.setButtonText ("Danger");
    limiterBypass.setButtonText ("Limiter Bypass");

    addAndMakeVisible (danger);
    addAndMakeVisible (limiterBypass);

    // Attachments
    aDrive = std::make_unique<SliderAttachment> (audioProcessor.apvts, "drive", drive);
    aFb    = std::make_unique<SliderAttachment> (audioProcessor.apvts, "feedback", feedback);
    aTone  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "tone", tone);
    aReso  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "reso", reso);
    aNoise = std::make_unique<SliderAttachment> (audioProcessor.apvts, "noiseMix", noiseMix);
    aOut   = std::make_unique<SliderAttachment> (audioProcessor.apvts, "output", output);

    aDanger        = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "danger", danger);
    aLimiterBypass = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "limiterBypass", limiterBypass);

    setSize (520, 220);
}

void BerzmowAudioProcessorEditor::initKnob (juce::Slider& s, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    s.setName (name);
    addAndMakeVisible (s);
}

void BerzmowAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawText ("Berzmow", 10, 10, getWidth() - 20, 20, juce::Justification::left);

    g.setFont (12.0f);
    auto drawLabel = [&] (juce::Slider& s)
    {
        g.drawText (s.getName(), s.getX(), s.getY() - 16, s.getWidth(), 14, juce::Justification::centred);
    };

    drawLabel (drive);
    drawLabel (feedback);
    drawLabel (tone);
    drawLabel (reso);
    drawLabel (noiseMix);
    drawLabel (output);
}

void BerzmowAudioProcessorEditor::resized()
{
    const int pad = 10;
    const int knobW = 80;
    const int knobH = 90;
    const int y0 = 45;

    drive.setBounds    (pad + 0*(knobW+pad), y0, knobW, knobH);
    feedback.setBounds (pad + 1*(knobW+pad), y0, knobW, knobH);
    tone.setBounds     (pad + 2*(knobW+pad), y0, knobW, knobH);
    reso.setBounds     (pad + 3*(knobW+pad), y0, knobW, knobH);
    noiseMix.setBounds (pad + 4*(knobW+pad), y0, knobW, knobH);
    output.setBounds   (pad + 5*(knobW+pad), y0, knobW, knobH);

    danger.setBounds        (pad, y0 + knobH + 20, 140, 24);
    limiterBypass.setBounds (pad + 150, y0 + knobH + 20, 160, 24);
}
