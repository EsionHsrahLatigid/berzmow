#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <juce_events/juce_events.h>
#include <cmath>
#include <functional>
#include <iostream>

namespace
{
bool check(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "[FAIL] " << message << '\n';
    return condition;
}

void setParameter(juce::AudioProcessorValueTreeState& state, const char* id, float value)
{
    if (auto* parameter = state.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void setParameter(juce::AudioProcessorValueTreeState& state, const char* id, bool value)
{
    if (auto* parameter = state.getParameter(id))
        parameter->setValueNotifyingHost(value ? 1.0f : 0.0f);
}

bool hasForbiddenRedPixel(juce::Component& component)
{
    juce::Image image(juce::Image::RGB, component.getWidth(), component.getHeight(), true);
    juce::Graphics graphics(image);
    component.paintEntireComponent(graphics, true);

    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto colour = image.getPixelAt(x, y);
            if (colour.getRed() > 180 && colour.getGreen() < 96 && colour.getBlue() < 96)
                return true;
        }

    return false;
}

bool componentTreeHasUsableControls(juce::Component& root)
{
    bool passed = true;
    int interactiveCount = 0;
    std::function<void(juce::Component&)> visit = [&](juce::Component& component)
    {
        if (dynamic_cast<juce::Slider*>(&component) != nullptr
            || dynamic_cast<juce::ToggleButton*>(&component) != nullptr)
        {
            ++interactiveCount;
            passed &= check(component.getWidth() >= 24 && component.getHeight() >= 24,
                            "editor controls should keep at least 24px hit areas");
        }

        for (int index = 0; index < component.getNumChildComponents(); ++index)
            visit(*component.getChildComponent(index));
    };
    visit(root);
    passed &= check(interactiveCount == 8, "editor should expose all six sliders and two toggles");
    return passed;
}

bool verifyEditorContract(BerzmowAudioProcessor& processor)
{
    BerzmowAudioProcessorEditor editor(processor);
    editor.resized();

    bool passed = true;
    passed &= check(editor.getWidth() == ehl::juce_design::Metrics::defaultWidth
                       && editor.getHeight() == ehl::juce_design::Metrics::defaultHeight,
                   "editor should use the shared EHL default size");
    passed &= check(editor.isResizable(), "editor should be resizable within shared EHL limits");
    passed &= check(componentTreeHasUsableControls(editor), "editor control tree should remain usable");
    passed &= check(!hasForbiddenRedPixel(editor), "editor should not retain red/neon accent pixels");
    return passed;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    BerzmowAudioProcessor processor;
    bool passed = true;

    passed &= check(processor.getName() == "Berzmow", "product name should be Berzmow");
    passed &= verifyEditorContract(processor);
    passed &= check(!processor.acceptsMidi(), "processor should not accept MIDI");
    passed &= check(!processor.isMidiEffect(), "processor should be an audio effect");

    juce::AudioProcessor::BusesLayout stereo;
    stereo.inputBuses.add(juce::AudioChannelSet::stereo());
    stereo.outputBuses.add(juce::AudioChannelSet::stereo());
    passed &= check(processor.isBusesLayoutSupported(stereo), "stereo input/output should be supported");

    auto* drive = processor.apvts.getParameter("drive");
    passed &= check(drive != nullptr, "Drive parameter should exist");
    if (drive != nullptr)
    {
        drive->setValueNotifyingHost(drive->convertTo0to1(0.20f));
        juce::MemoryBlock state;
        processor.getStateInformation(state);
        drive->setValueNotifyingHost(drive->convertTo0to1(0.90f));
        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        passed &= check(std::abs(processor.apvts.getRawParameterValue("drive")->load() - 0.20f) < 0.001f,
                        "APVTS state should round-trip");
    }

    processor.prepareToPlay(44100.0, 256);

    for (int block = 0; block < 8; ++block)
    {
        juce::AudioBuffer<float> audio(2, 256);
        for (int sample = 0; sample < audio.getNumSamples(); ++sample)
        {
            const auto value = static_cast<float>(0.1 * std::sin(2.0 * juce::MathConstants<double>::pi
                                                                 * 110.0 * sample / 44100.0));
            audio.setSample(0, sample, value);
            audio.setSample(1, sample, value);
        }

        juce::MidiBuffer midi;
        processor.processBlock(audio, midi);

        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                passed &= check(std::isfinite(audio.getSample(channel, sample)), "processed audio should remain finite");
    }

    const int variableBlockSizes[] { 1, 7, 32, 64, 127, 256 };
    for (int pass = 0; pass < 4; ++pass)
    {
        setParameter(processor.apvts, "drive", pass % 2 == 0 ? 0.0f : 1.0f);
        setParameter(processor.apvts, "feedback", pass % 2 == 0 ? 0.05f : 1.0f);
        setParameter(processor.apvts, "tone", pass % 2 == 0 ? 0.0f : 1.0f);
        setParameter(processor.apvts, "reso", pass % 2 == 0 ? 0.0f : 1.0f);
        setParameter(processor.apvts, "noiseMix", pass % 2 == 0 ? 0.0f : 1.0f);
        setParameter(processor.apvts, "output", pass % 2 == 0 ? 0.0f : 1.0f);
        setParameter(processor.apvts, "danger", pass % 2 != 0);
        setParameter(processor.apvts, "limiterBypass", pass == 3);

        for (const auto blockSize : variableBlockSizes)
        {
            juce::AudioBuffer<float> audio(2, blockSize);
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
            {
                const auto value = static_cast<float>(0.05 * std::sin(2.0 * juce::MathConstants<double>::pi
                                                                      * (55.0 + pass * 37.0) * sample / 44100.0));
                audio.setSample(0, sample, value);
                audio.setSample(1, sample, -value);
            }

            juce::MidiBuffer midi;
            processor.processBlock(audio, midi);

            for (int channel = 0; channel < audio.getNumChannels(); ++channel)
                for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                    passed &= check(std::isfinite(audio.getSample(channel, sample)),
                                    "automated variable-block output should remain finite");
        }
    }

    if (passed)
        std::cout << "Berzmow plug-in integration checks passed\n";
    return passed ? 0 : 1;
}
