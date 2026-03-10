#include "Synth.h"
#include "../PluginEditor.h"

Synth::Synth(TetraOPAudioProcessor& p) : audioProcessor(p)
{
    setVoiceStealingEnabled (true);

	for (int i = 0; i < MAX_POLYPHONY; ++i)
    {
        auto voice = new Voice (audioProcessor, i);
        addVoice (voice);
    }

    createFilters(0);
    createFilters(1);

    fm = std::make_unique<FmMatrix>(audioProcessor);
}

Synth::~Synth()
{
}

void Synth::prepare()
{
    auto srate = (float)getSampleRate();
    fm->prepare(srate);
    dcBlockerL.setSampleRate(srate);
    dcBlockerR.setSampleRate(srate);

    for (auto& filter : f1L)
        filter->prepare(srate);
    for (auto& filter : f1R)
        filter->prepare(srate);
}

void Synth::clear()
{
    clearVoices();
    dcBlockerL.reset();
    dcBlockerR.reset();
}

static std::unique_ptr<Filter> makeFilter(Filter::Type type)
{
    switch (type)
    {
    case Filter::kDigital12: return std::make_unique<Digital>(Filter::k12p);
        default: return std::make_unique<Digital>(Filter::k12p);
    }
}

void Synth::createFilters(int f)
{
    String prefix = f == 0 ? "f1_" : "f2_";
    auto type = (Filter::Type)audioProcessor.params.getRawParameterValue(prefix + "type")->load();
    auto mode = (Filter::Mode)audioProcessor.params.getRawParameterValue(prefix + "mode")->load();

    for (int i = 0; i < MAX_POLYPHONY / SIMDSZ; ++i)
    {
        f1L[i] = makeFilter(type);
        f1R[i] = makeFilter(type);
        f1L[i]->setMode(mode);
        f1R[i]->setMode(mode);
    }
}

void Synth::initFilters(int voiceId, float cutoff, float resonance)
{
    auto group = voiceId / SIMDSZ;
    auto lane = voiceId % SIMDSZ;

    auto mask = Utils::laneToMask(lane);
    f1L[group]->init(cutoff, resonance, true, mask);
    f1R[group]->init(cutoff, resonance, true, mask);
}

void Synth::updateFilters(int voiceId, float cutoff, float resonance)
{
    auto group = voiceId / SIMDSZ;
    auto lane = voiceId % SIMDSZ;

    auto mask = Utils::laneToMask(lane);
    Utils::setMasked(f1L[group]->cut_targ, cutoff, mask);
    Utils::setMasked(f1R[group]->cut_targ, cutoff, mask);
    Utils::setMasked(f1L[group]->res_targ, resonance, mask);
    Utils::setMasked(f1R[group]->res_targ, resonance, mask);
}

void Synth::handleMidiEvent (const juce::MidiMessage& m)
{
    if (m.isNoteOff())
        lastEventWasNoteOff = true;
    else if (m.isNoteOn())
        lastEventWasNoteOff = false;

    MPESynthesiser::handleMidiEvent (m);
}

void Synth::renderNextSubBlock(AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const ScopedLock sl(voicesLock);
    auto left = buffer.getWritePointer(0);
    auto right = buffer.getWritePointer(1);

     // gather active voices
    std::vector<Voice*> activeVoices;
    for (auto* voice : voices)
        if (voice->isActive())
            activeVoices.push_back((Voice*)voice);

    if (activeVoices.empty())
        return;

    for (auto& voice : activeVoices) 
    {
        voice->startBlock(startSample, numSamples);
        for (auto& osc : voice->osc)
            osc.startBlock(startSample, numSamples);
    }

    int maxVoice = 0;
    for (auto& voice : activeVoices)
        if (voice->id > maxVoice)
            maxVoice = voice->id;

    for (int batch = 0; batch <= maxVoice / SIMDSZ; ++batch)
    {
        bool maskarr[4] = { false, false, false, false };
        for (int v = 0; v < SIMDSZ; ++v)
        {
            auto idx = batch * SIMDSZ + v;
            auto voice = (Voice*)voices[idx];
            if (voice->isActive())
            {
                maskarr[v] = true;
            }
        }
        SIMDM mask = SIMDM(maskarr);

        if (mask.testz())
            continue; // all voices inactive

        int activeVoice = -1; // used to retrieve oscillator outputs for visualization
        for (int lane = 0; lane < SIMDSZ; ++lane)
            if ((batch * 4 + lane) == audioProcessor.modulation->lastUsedVoice)
                activeVoice = batch * 4 + lane;

        auto& v = vox[batch];
        fm->processBlock(v, numSamples, activeVoice, Utils::maskToFloat(mask));

        bool f1On = true;
        bool filterSeries = false;
        std::fill(filterInL.begin(), filterInL.begin() + numSamples, SIMDF(0.0f));
        std::fill(filterInR.begin(), filterInR.begin() + numSamples, SIMDF(0.0f));
        std::fill(filterOutL.begin(), filterOutL.begin() + numSamples, SIMDF(0.f));
        std::fill(filterOutR.begin(), filterOutR.begin() + numSamples, SIMDF(0.f));

        // process filter 1
        SIMDF routef1[MAX_OSCILLATORS] = { 1.f, 1.f, 1.f, 1.f };
        if (f1On)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                for (int o = 0; o < MAX_OSCILLATORS; ++o)
                {
                    filterInL[i] += fm->outL[o][i] * routef1[o];
                    filterInR[i] += fm->outR[o][i] * routef1[o];
                }
            }

            auto& fl = f1L[batch];
            auto& fr = f1R[batch];

            fl->processBlock(filterInL, startSample, numSamples, audioProcessor.currBlockSize, Utils::maskToFloat(mask));
            fr->processBlock(filterInR, startSample, numSamples, audioProcessor.currBlockSize, Utils::maskToFloat(mask));

            if (!filterSeries)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    filterOutL[i] += fl->out[i];
                    filterOutR[i] += fr->out[i];
                }
            }
        }

        bool f2On = false;
        SIMDF routef2[MAX_OSCILLATORS] = { 1.f, 1.f, 1.f, 1.f };

        // direct out are oscillators not routed to any filter
        SIMDF routeDirOut[MAX_OSCILLATORS];
        for (int i = 0; i < MAX_OSCILLATORS; ++i)
            routeDirOut[i] = SIMDF(1.f) - ((routef1[i] * f1On) + routef2[i] * f2On).min(1.f);

        // mix output
        std::fill(outL.begin(), outL.begin() + numSamples, 0.f);
        std::fill(outR.begin(), outR.begin() + numSamples, 0.f);

        for (int i = 0; i < numSamples; ++i)
        {
            for (int o = 0; o < MAX_OSCILLATORS; ++o)
            {
                outL[i] += (fm->outL[o][i] * routeDirOut[o]);
                outR[i] += (fm->outR[o][i] * routeDirOut[o]);
            }
        }

        if (f1On)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                outL[i] += filterOutL[i];
                outR[i] += filterOutR[i];
            }
        }

        for (int s = 0; s < numSamples; ++s)
        {
            left[startSample + s] += (outL[s] * v.voice.env * v.voice.vel_mult).sum();
            right[startSample + s] += (outR[s] * v.voice.env * v.voice.vel_mult).sum();
            v.voice.env += v.voice.env_step;
        }
    }

    // final dc blocking
    for (int i = 0; i < numSamples; ++i)
    {
        left[startSample + i] = dcBlockerL.process(left[startSample + i]);
        right[startSample + i] = dcBlockerR.process(right[startSample + i]);
    }

    for (auto& voice : activeVoices) {
        voice->endBlock(startSample, numSamples);
    }
}