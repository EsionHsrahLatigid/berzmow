#pragma once

#include <JuceHeader.h>

class BerzmowAudioProcessor final : public juce::AudioProcessor
{
public:
    BerzmowAudioProcessor();
    ~BerzmowAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Public so Editor can attach
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    //==============================================================================
    // ---- Safety helpers (ALWAYS ON) ----
    static inline bool isFinite (float x) noexcept { return std::isfinite (x); }

    static inline float safetyClamp (float x) noexcept
    {
        // Numerical safety clamp (NOT a limiter). Keep always-on to prevent Inf.
        return juce::jlimit (-1.0e6f, 1.0e6f, x);
    }

    static inline float sanitizeSample (float x) noexcept
    {
        if (! isFinite (x)) return 0.0f;
        return safetyClamp (x);
    }

    static inline float hardClip (float x, float thr) noexcept
    {
        return juce::jlimit (-thr, thr, x);
    }

    static inline float softSat (float x) noexcept
    {
        // Simple, stable saturator
        return std::tanh (x);
    }

    void resetDspState() noexcept;
    void updateFixedFilterCoefficients();
    void updateAutomatedFilterState (float preEmphasisDb, float toneHz, float resonance);

    //==============================================================================
    // ---- DSP ----
    juce::Random rng;

    // Filters: pre-emphasis peak + HP (DC/rumble control)
    juce::dsp::IIR::Filter<float> preEmphasis[2];
    juce::dsp::IIR::Filter<float> highPass[2];

    // Feedback path DC-cut (separate so you can tune independently)
    juce::dsp::IIR::Filter<float> fbHighPass[2];

    // SVF tone shaping (bandpass by default)
    juce::dsp::StateVariableTPTFilter<float> svf[2];

    // Output limiter (optional bypass)
    juce::dsp::Limiter<float> limiter;
    juce::AudioBuffer<float> limiterScratch;

    // Feedback state (per channel)
    float fbState[2] { 0.0f, 0.0f };

    double currentSampleRate = 44100.0;
    float cachedPreEmphasisDb = -1.0f;
    float cachedToneHz = -1.0f;
    float cachedResonance = -1.0f;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BerzmowAudioProcessor)
};
