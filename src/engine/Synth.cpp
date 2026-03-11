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

    fm = std::make_unique<FmMatrix>(audioProcessor);

    createFilters(0);
    createFilters(1);

    for (int i = 0; i < 2; ++i)
    {
        String prefix = i == 0 ? "f1_" : "f2";
        audioProcessor.params.addParameterListener(prefix + "on", this);
        audioProcessor.params.addParameterListener(prefix + "type", this);
        audioProcessor.params.addParameterListener(prefix + "mode", this);
        audioProcessor.params.addParameterListener(prefix + "inA", this);
        audioProcessor.params.addParameterListener(prefix + "inB", this);
        audioProcessor.params.addParameterListener(prefix + "inC", this);
        audioProcessor.params.addParameterListener(prefix + "inD", this);
    }
}

Synth::~Synth()
{
}

void Synth::parameterChanged(const juce::String& paramId, float)
{
    if (paramId.startsWith("f1_"))
        f1.dirty = true;
    else if (paramId.startsWith("f2_"))
        f2.dirty = true;
}

void Synth::onFilterChange(int f)
{
    String prefix = f == 0 ? "f1_" : "f2_";
    auto& filter = f == 0 ? f1 : f2;
    filter.on = (bool)audioProcessor.params.getRawParameterValue(prefix + "on")->load();
    filter.type = (Filter::Type)audioProcessor.params.getRawParameterValue(prefix + "type")->load();
    filter.mode = (Filter::Mode)audioProcessor.params.getRawParameterValue(prefix + "mode")->load();
    filter.ain = (bool)audioProcessor.params.getRawParameterValue(prefix + "inA")->load();
    filter.bin = (bool)audioProcessor.params.getRawParameterValue(prefix + "inB")->load();
    filter.cin = (bool)audioProcessor.params.getRawParameterValue(prefix + "inC")->load();
    filter.din = (bool)audioProcessor.params.getRawParameterValue(prefix + "inD")->load();

    auto& filtersL = f == 0 ? f1L : f2L;
    auto& filtersR = f == 0 ? f1R : f2R;
    
    if (filter.type != filtersL[0]->type)
    {
        createFilters(f);
    }
    else if (filter.mode != filtersL[0]->filterMode)
    {
        for (auto& ff : filtersL)
            ff->setMode(filter.mode);
        for (auto& ff : filtersR)
            ff->setMode(filter.mode);
    }
    
    if (f == 1) 
        filter.fin = audioProcessor.params.getRawParameterValue("f2_inF1")->load();

    filterSeries = f1.on && f2.on && f2.fin;
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
    for (auto& filter : f1L) filter->clear(0.f, { true, true, true, true });
    for (auto& filter : f1R) filter->clear(0.f, { true, true, true, true });
    for (auto& filter : f2L) filter->clear(0.f, { true, true, true, true });
    for (auto& filter : f2R) filter->clear(0.f, { true, true, true, true });
}

static std::unique_ptr<Filter> makeFilter(Filter::Type type)
{
    switch (type)
    {
        case Filter::kDigital12: return std::make_unique<Digital>(Filter::k12p);
        case Filter::kDigital24: return std::make_unique<Digital>(Filter::k24p);
        default: return std::make_unique<Digital>(Filter::k12p);
    }
}

void Synth::createFilters(int f)
{
    String prefix = f == 0 ? "f1_" : "f2_";
    auto type = (Filter::Type)audioProcessor.params.getRawParameterValue(prefix + "type")->load();
    auto mode = (Filter::Mode)audioProcessor.params.getRawParameterValue(prefix + "mode")->load();
    auto srate = (float)getSampleRate();

    for (int i = 0; i < MAX_POLYPHONY / SIMDSZ; ++i)
    {
        auto& fl = f == 0 ? f1L : f2L;
        auto& fr = f == 0 ? f1R : f2R;
        fl[i] = makeFilter(type);
        fr[i] = makeFilter(type);
        fl[i]->setMode(mode);
        fr[i]->setMode(mode);
        fl[i]->prepare(srate);
        fr[i]->prepare(srate);
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
    if (f1.dirty)
    {
        onFilterChange(0);
        f1.dirty = false;
    }

    if (f2.dirty)
    {
        onFilterChange(1);
        f2.dirty = false;
    }

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


        // process filter 1
        std::fill(filterOutL.begin(), filterOutL.begin() + numSamples, SIMDF(0.f));
        std::fill(filterOutR.begin(), filterOutR.begin() + numSamples, SIMDF(0.f));
        bool routef1[MAX_OSCILLATORS] = { f1.ain, f1.bin, f1.cin, f1.din };
        if (f1.on)
        {
            std::fill(filterInL.begin(), filterInL.begin() + numSamples, SIMDF(0.0f));
            std::fill(filterInR.begin(), filterInR.begin() + numSamples, SIMDF(0.0f));
            for (int o = 0; o < MAX_OSCILLATORS; ++o)
            {
                if (!routef1[o] || !v.osc[o].isOn)
                    continue;

                for (int i = 0; i < numSamples; ++i)
                {
                    filterInL[i] += fm->outL[o][i];
                    filterInR[i] += fm->outR[o][i];
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

        // process filter 2
        bool routef2[MAX_OSCILLATORS] = { f2.ain, f2.bin, f2.cin, f2.din };
        if (f2.on)
        {
            std::fill(filterInL.begin(), filterInL.begin() + numSamples, SIMDF(0.0f));
            std::fill(filterInR.begin(), filterInR.begin() + numSamples, SIMDF(0.0f));
            for (int o = 0; o < MAX_OSCILLATORS; ++o)
            {
                if (!routef2[o] || !v.osc[o].isOn)
                    continue;

                for (int i = 0; i < numSamples; ++i)
                {
                    filterInL[i] += fm->outL[o][i];
                    filterInR[i] += fm->outR[o][i];
                }
            }

            if (filterSeries)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    filterInL[i] += filterOutL[i];
                    filterInR[i] += filterOutR[i];
                }
                std::fill(filterOutL.begin(), filterOutL.begin() + numSamples, SIMDF(0.f));
                std::fill(filterOutR.begin(), filterOutR.begin() + numSamples, SIMDF(0.f));
            }

            auto& fl = f2L[batch];
            auto& fr = f2R[batch];

            fl->processBlock(filterInL, startSample, numSamples, audioProcessor.currBlockSize, Utils::maskToFloat(mask));
            fr->processBlock(filterInR, startSample, numSamples, audioProcessor.currBlockSize, Utils::maskToFloat(mask));

            for (int i = 0; i < numSamples; ++i)
            {
                filterOutL[i] += fl->out[i];
                filterOutR[i] += fr->out[i];
            }
        }

        // direct out are oscillators not routed to any filter
        bool routeDirOut[MAX_OSCILLATORS]{};
        for (int i = 0; i < MAX_OSCILLATORS; ++i)
            routeDirOut[i] = !((routef1[i] && f1.on) || (routef2[i] && f2.on));

        std::fill(outL.begin(), outL.begin() + numSamples, 0.f);
        std::fill(outR.begin(), outR.begin() + numSamples, 0.f);

        // mix direct output
        for (int o = 0; o < MAX_OSCILLATORS; ++o)
        {
            if (!routeDirOut[o] || !v.osc[o].isOn)
                continue; 

            for (int i = 0; i < numSamples; ++i)
            {
                outL[i] += (fm->outL[o][i] * routeDirOut[o]);
                outR[i] += (fm->outR[o][i] * routeDirOut[o]);
            }
        }

        // mix filter output
        if (f1.on || f2.on)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                outL[i] += filterOutL[i];
                outR[i] += filterOutR[i];
            }
        }

        // final mix
        for (int s = 0; s < numSamples; ++s)
        {
            left[startSample + s] += (outL[s] * v.voice.env * v.voice.vel_mult).sum();
            right[startSample + s] += (outR[s] * v.voice.env * v.voice.vel_mult).sum();
            v.voice.env += v.voice.env_step;
        }
    }

    // dc blocking
    for (int i = 0; i < numSamples; ++i)
    {
        left[startSample + i] = dcBlockerL.process(left[startSample + i]);
        right[startSample + i] = dcBlockerR.process(right[startSample + i]);
    }

    for (auto& voice : activeVoices) {
        voice->endBlock(startSample, numSamples);
    }
}