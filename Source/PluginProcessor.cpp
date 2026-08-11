#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    inline float dbToLin (float db) noexcept { return juce::Decibels::decibelsToGain (db); }

    // Map 0..1 to something musically useful
    inline float mapDriveDb (float norm01, bool danger) noexcept
    {
        // Safe: 0..24 dB, Danger: 0..36 dB
        const float maxDb = danger ? 36.0f : 24.0f;
        return norm01 * maxDb;
    }

    inline float mapOutputDb (float norm01) noexcept
    {
        // -24 .. +6 dB
        return juce::jmap (norm01, -24.0f, 6.0f);
    }

    inline float mapToneHz (float norm01, double sr) noexcept
    {
        // 40 Hz .. 0.45*sr
        const float lo = 40.0f;
        const float hi = (float) (0.45 * sr);
        // Exponential-ish mapping
        const float t = norm01;
        const float hz = lo * std::pow (hi / lo, t);
        return juce::jlimit (lo, hi, hz);
    }

    inline float mapResonance (float norm01, bool danger) noexcept
    {
        // SVF resonance: JUCE expects ~0.1..~5+ (depends); keep bounded.
        // Safe: 0.2..1.8, Danger: 0.2..3.2
        const float hi = danger ? 3.2f : 1.8f;
        return juce::jmap (norm01, 0.2f, hi);
    }

    inline float mapFeedback (float norm01, bool danger) noexcept
    {
        // Absolutely never allow >= 1.0
        const float maxFb = danger ? 0.995f : 0.97f;
        return juce::jlimit (0.0f, maxFb, norm01 * maxFb);
    }

    inline float mapNoiseMix (float norm01) noexcept
    {
        return juce::jlimit (0.0f, 1.0f, norm01);
    }
}

BerzmowAudioProcessor::BerzmowAudioProcessor()
: AudioProcessor (BusesProperties()
                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
  apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

const juce::String BerzmowAudioProcessor::getName() const { return "Berzmow"; }

bool BerzmowAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* BerzmowAudioProcessor::createEditor() { return new BerzmowAudioProcessorEditor (*this); }

juce::AudioProcessorValueTreeState::ParameterLayout BerzmowAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto addFloat = [&] (const char* id, const char* name, float minV, float maxV, float defV)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (id, name,
            juce::NormalisableRange<float> (minV, maxV, 0.0f, 1.0f), defV));
    };

    auto addBool = [&] (const char* id, const char* name, bool defV)
    {
        p.push_back (std::make_unique<juce::AudioParameterBool> (id, name, defV));
    };

    // 0..1 knobs (we map them ourselves)
    addFloat ("drive",     "Drive",     0.0f, 1.0f, 0.35f);
    addFloat ("feedback",  "Feedback",  0.0f, 1.0f, 0.25f);
    addFloat ("tone",      "Tone",      0.0f, 1.0f, 0.60f);
    addFloat ("reso",      "Resonance", 0.0f, 1.0f, 0.40f);
    addFloat ("noiseMix",  "Noise Mix", 0.0f, 1.0f, 1.00f);
    addFloat ("output",    "Output",    0.0f, 1.0f, 0.35f);

    addBool  ("danger",        "Danger",         false);
    addBool  ("limiterBypass", "Limiter Bypass", false);

    return { p.begin(), p.end() };
}

bool BerzmowAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getChannelSet (true,  0);
    const auto& out = layouts.getChannelSet (false, 0);

    // Stereo only for MVP; expand if you want.
    return (in == juce::AudioChannelSet::stereo() && out == juce::AudioChannelSet::stereo());
}

void BerzmowAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    limiterScratch.setSize (2, samplesPerBlock, false, false, true);

    // Prepare limiter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 2;

    limiter.prepare (spec);
    limiter.reset();

    // Configure default filter types
    for (int ch = 0; ch < 2; ++ch)
    {
        preEmphasis[ch].reset();
        highPass[ch].reset();
        fbHighPass[ch].reset();

        svf[ch].reset();
        juce::dsp::ProcessSpec svfSpec = spec;
        svfSpec.numChannels = 1;
        svf[ch].prepare (svfSpec);
        svf[ch].setType (juce::dsp::StateVariableTPTFilterType::bandpass);
    }

    resetDspState();
}

void BerzmowAudioProcessor::releaseResources()
{
}

void BerzmowAudioProcessor::resetDspState() noexcept
{
    for (int ch = 0; ch < 2; ++ch)
        fbState[ch] = 0.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        preEmphasis[ch].reset();
        highPass[ch].reset();
        fbHighPass[ch].reset();
        svf[ch].reset();
    }

    limiter.reset();
}

void BerzmowAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    jassert (buffer.getNumChannels() == 2);

    // ---- Read parameters (once per block) ----
    const bool danger         = apvts.getRawParameterValue ("danger")->load() > 0.5f;
    const bool limiterBypass  = apvts.getRawParameterValue ("limiterBypass")->load() > 0.5f;

    const float driveN    = apvts.getRawParameterValue ("drive")->load();
    const float fbN       = apvts.getRawParameterValue ("feedback")->load();
    const float toneN     = apvts.getRawParameterValue ("tone")->load();
    const float resoN     = apvts.getRawParameterValue ("reso")->load();
    const float noiseMixN = apvts.getRawParameterValue ("noiseMix")->load();
    const float outN      = apvts.getRawParameterValue ("output")->load();

    const float driveDb   = mapDriveDb (driveN, danger);
    const float driveGain = dbToLin (driveDb);

    const float fbAmt     = mapFeedback (fbN, danger);
    const float toneHz    = mapToneHz (toneN, currentSampleRate);
    const float reso      = mapResonance (resoN, danger);
    const float noiseMix  = mapNoiseMix (noiseMixN);

    // Output trim: if limiter is bypassed, auto-trim for safety
    const float outDb     = mapOutputDb (outN) + (limiterBypass ? -12.0f : 0.0f);
    const float outGain   = dbToLin (outDb);

    // ---- Update coefficients (block-rate is fine for MVP) ----
    // Pre-emphasis: a gentle peak around 3kHz to increase perceived loudness
    const float preEmphDb = danger ? 9.0f : 6.0f; // Danger boosts more
    for (int ch = 0; ch < 2; ++ch)
    {
        *preEmphasis[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 3000.0f, 0.7f, dbToLin (preEmphDb));

        // Output HP: keep rumble/DC down (helps headroom + stability)
        *highPass[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (
            currentSampleRate, 30.0);

        // FB HP: slightly higher cut to prevent DC runaway inside loop
        *fbHighPass[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (
            currentSampleRate, 40.0);

        svf[ch].setCutoffFrequency (toneHz);
        svf[ch].setResonance (reso);
    }

    // ---- Main synthesis ----
    bool badDetected = false;

    auto* l = buffer.getWritePointer (0);
    auto* r = buffer.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        // Input (if host provides) + noise
        const float inL = l[i];
        const float inR = r[i];

        const float n0 = (rng.nextFloat() * 2.0f - 1.0f);
        const float n1 = (rng.nextFloat() * 2.0f - 1.0f);

        float xL = inL * (1.0f - noiseMix) + n0 * noiseMix;
        float xR = inR * (1.0f - noiseMix) + n1 * noiseMix;

        // Feedback injection (pick previous fbState)
        xL = sanitizeSample (xL + fbState[0] * fbAmt);
        xR = sanitizeSample (xR + fbState[1] * fbAmt);

        // Drive into saturation
        xL = sanitizeSample (xL * driveGain);
        xR = sanitizeSample (xR * driveGain);

        // Two-stage nonlinearity: soft sat then hard clip for density
        xL = softSat (xL);
        xR = softSat (xR);

        // Hard clip threshold: Danger lowers threshold (more flattening => higher perceived loudness)
        const float clipThr = danger ? 0.55f : 0.75f;
        xL = hardClip (xL * 1.6f, clipThr);
        xR = hardClip (xR * 1.6f, clipThr);

        // Pre-emphasis + HP
        xL = preEmphasis[0].processSample (xL);
        xR = preEmphasis[1].processSample (xR);
        xL = highPass[0].processSample (xL);
        xR = highPass[1].processSample (xR);

        xL = sanitizeSample (xL);
        xR = sanitizeSample (xR);

        // SVF bandpass tone shaping
        xL = svf[0].processSample (0, xL);
        xR = svf[1].processSample (0, xR);

        xL = sanitizeSample (xL);
        xR = sanitizeSample (xR);

        // Feedback capture (through FB HP + mild saturation for stability)
        float fbL = fbHighPass[0].processSample (xL);
        float fbR = fbHighPass[1].processSample (xR);

        fbL = softSat (fbL * (danger ? 1.25f : 1.0f));
        fbR = softSat (fbR * (danger ? 1.25f : 1.0f));

        fbL = sanitizeSample (fbL);
        fbR = sanitizeSample (fbR);

        fbState[0] = fbL;
        fbState[1] = fbR;

        // Output gain (limiter applied later as a block)
        l[i] = sanitizeSample (xL * outGain);
        r[i] = sanitizeSample (xR * outGain);

        // Detect any non-finite at runtime (paranoid guard)
        if (!isFinite (l[i]) || !isFinite (r[i]) || !isFinite (fbState[0]) || !isFinite (fbState[1]))
            badDetected = true;
    }

    // Final safety pass: eliminate any NaN/Inf that slipped through
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* p = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            if (!isFinite (p[i])) { p[i] = 0.0f; badDetected = true; }
            else p[i] = safetyClamp (p[i]);
        }
    }

    // If we detected bad state, reset DSP to prevent persistent corruption
    if (badDetected)
        resetDspState();

    // Output limiter (optional bypass)
    if (! limiterBypass)
    {
        // Limiter works in-place using dsp::AudioBlock
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        limiter.process (ctx);

        // Post-limiter sanitize (belt and suspenders)
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* p = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                p[i] = sanitizeSample (p[i]);
        }
    }
}

void BerzmowAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void BerzmowAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// This factory function is required by the JUCE plugin client.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BerzmowAudioProcessor();
}
