#include "PluginEditor.h"

namespace
{
void initLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    ehl::juce_design::styleLabel (label);
    label.setJustificationType (juce::Justification::centred);
}
} // namespace

BerzmowAudioProcessorEditor::BerzmowAudioProcessorEditor (BerzmowAudioProcessor& p)
: AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);

    initKnob (drive,    "Drive");
    initKnob (feedback, "FB");
    initKnob (tone,     "Tone");
    initKnob (reso,     "Reso");
    initKnob (noiseMix, "Noise");
    initKnob (output,   "Out");

    danger.setButtonText ("Danger");
    limiterBypass.setButtonText ("Limiter Bypass");
    ehl::juce_design::styleToggle (danger);
    ehl::juce_design::styleToggle (limiterBypass);

    initLabel (driveLabel, "DRIVE");
    initLabel (feedbackLabel, "FB");
    initLabel (toneLabel, "TONE");
    initLabel (resoLabel, "RESO");
    initLabel (noiseMixLabel, "NOISE");
    initLabel (outputLabel, "OUT");

    addAndMakeVisible (danger);
    addAndMakeVisible (limiterBypass);
    addAndMakeVisible (display);
    addAndMakeVisible (driveLabel);
    addAndMakeVisible (feedbackLabel);
    addAndMakeVisible (toneLabel);
    addAndMakeVisible (resoLabel);
    addAndMakeVisible (noiseMixLabel);
    addAndMakeVisible (outputLabel);

    // Attachments
    aDrive = std::make_unique<SliderAttachment> (audioProcessor.apvts, "drive", drive);
    aFb    = std::make_unique<SliderAttachment> (audioProcessor.apvts, "feedback", feedback);
    aTone  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "tone", tone);
    aReso  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "reso", reso);
    aNoise = std::make_unique<SliderAttachment> (audioProcessor.apvts, "noiseMix", noiseMix);
    aOut   = std::make_unique<SliderAttachment> (audioProcessor.apvts, "output", output);

    aDanger        = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "danger", danger);
    aLimiterBypass = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "limiterBypass", limiterBypass);

    setResizable (true, true);
    setResizeLimits (ehl::juce_design::Metrics::minimumWidth,
                     ehl::juce_design::Metrics::minimumHeight,
                     ehl::juce_design::Metrics::maximumWidth,
                     ehl::juce_design::Metrics::maximumHeight);
    setSize (ehl::juce_design::Metrics::defaultWidth,
             ehl::juce_design::Metrics::defaultHeight);
    updateDisplay();
    startTimerHz (15);
}

BerzmowAudioProcessorEditor::~BerzmowAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BerzmowAudioProcessorEditor::initKnob (juce::Slider& s, const juce::String& name)
{
    s.setName (name);
    ehl::juce_design::styleSlider (s);
    addAndMakeVisible (s);
}

void BerzmowAudioProcessorEditor::paint (juce::Graphics& g)
{
    ehl::juce_design::paintEditorChrome (g, getLocalBounds(),
                                         "Berzmow",
                                         "feedback noise distortion");
}

void BerzmowAudioProcessorEditor::resized()
{
    display.setBounds (ehl::juce_design::parameterDisplayArea (getLocalBounds()));
    ehl::juce_design::layoutLabelledControl (
        driveLabel, drive, ehl::juce_design::controlCell (getLocalBounds(), 0));
    ehl::juce_design::layoutLabelledControl (
        feedbackLabel, feedback, ehl::juce_design::controlCell (getLocalBounds(), 1));
    ehl::juce_design::layoutLabelledControl (
        toneLabel, tone, ehl::juce_design::controlCell (getLocalBounds(), 2));
    ehl::juce_design::layoutLabelledControl (
        resoLabel, reso, ehl::juce_design::controlCell (getLocalBounds(), 3));
    ehl::juce_design::layoutLabelledControl (
        noiseMixLabel, noiseMix, ehl::juce_design::controlCell (getLocalBounds(), 4));
    ehl::juce_design::layoutLabelledControl (
        outputLabel, output, ehl::juce_design::controlCell (getLocalBounds(), 5));
    danger.setBounds (ehl::juce_design::controlCell (getLocalBounds(), 6).reduced (8, 24));
    limiterBypass.setBounds (ehl::juce_design::controlCell (getLocalBounds(), 7).reduced (8, 24));
}

void BerzmowAudioProcessorEditor::timerCallback() { updateDisplay(); }

void BerzmowAudioProcessorEditor::updateDisplay()
{
    const auto normalized = [this](const char* id)
    {
        if (auto* parameter = audioProcessor.apvts.getParameter (id))
            return parameter->getValue();
        return 0.0f;
    };
    display.setValues ({ normalized ("drive"), normalized ("feedback"),
                         normalized ("tone"), normalized ("reso") });
}
