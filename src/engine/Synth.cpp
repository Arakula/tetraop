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

    for (int i = 0; i < 2; ++i)
    {
        String prefix = i == 0 ? "f1_" : "f2_";
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

void Synth::onFilterChange(int f1orf2)
{
    String prefix = f1orf2 == 0 ? "f1_" : "f2_";
    auto& filter = f1orf2 == 0 ? f1 : f2;
    filter.on = (bool)audioProcessor.params.getRawParameterValue(prefix + "on")->load();
    filter.type = (Filter::Type)audioProcessor.params.getRawParameterValue(prefix + "type")->load();
    filter.mode = (Filter::Mode)audioProcessor.params.getRawParameterValue(prefix + "mode")->load();
    filter.ain = (bool)audioProcessor.params.getRawParameterValue(prefix + "inA")->load();
    filter.bin = (bool)audioProcessor.params.getRawParameterValue(prefix + "inB")->load();
    filter.cin = (bool)audioProcessor.params.getRawParameterValue(prefix + "inC")->load();
    filter.din = (bool)audioProcessor.params.getRawParameterValue(prefix + "inD")->load();

    auto& filtersL = f1orf2 == 0 ? f1L : f2L;
    auto& filtersR = f1orf2 == 0 ? f1R : f2R;

    if (filter.type != filtersL[0]->type)
    {
        createFilters(f1orf2);
    }
    else if (filter.mode != filtersL[0]->filterMode)
    {
        SIMDM mask = { true, true, true ,true };
        for (auto& ff : filtersL)
        {
            ff->setMode(filter.mode);
            ff->init(ff->cut_targ, ff->res_targ, true, mask);
        }
        for (auto& ff : filtersR)
        {
            ff->setMode(filter.mode);
            ff->init(ff->cut_targ, ff->res_targ, true, mask);
        }
    }

    if (f1orf2 == 1)
        filter.fin = audioProcessor.params.getRawParameterValue("f2_inF1")->load();

    filterSeries = f1.on && f2.on && f2.fin;
}

void Synth::prepare()
{
    auto srate = (float)getSampleRate();
    fm->prepare(srate);
    dcBlockerL.setSampleRate(srate);
    dcBlockerR.setSampleRate(srate);

    if (f1L[0] == nullptr)
    {
        createFilters(0);
        createFilters(1);
    }
    
    for (auto& filter : f1L)
        filter->prepare(srate, true);
    for (auto& filter : f1R)
        filter->prepare(srate, true);
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
        case Filter::kAnalog12: return std::make_unique<Analog>(Filter::k12p);
        case Filter::kAnalog24: return std::make_unique<Analog>(Filter::k24p);
        case Filter::kLadder12: return std::make_unique<Ladder>(Filter::k12p);
        case Filter::kLadder24: return std::make_unique<Ladder>(Filter::k24p);
        case Filter::kTB303: return std::make_unique<TB303>();
        case Filter::kPhaserPos: return std::make_unique<Phaser>(true);
        case Filter::kPhaserNeg: return std::make_unique<Phaser>(false);
        default: return std::make_unique<Digital>(Filter::k12p);
    }
}

void Synth::createFilters(int f) // f: f1 or f2
{
    String prefix = f == 0 ? "f1_" : "f2_";
    auto type = (Filter::Type)audioProcessor.params.getRawParameterValue(prefix + "type")->load();
    auto mode = (Filter::Mode)audioProcessor.params.getRawParameterValue(prefix + "mode")->load();
    auto srate = (float)getSampleRate();

    for (int i = 0; i < MAX_POLYPHONY / SIMDSZ; ++i)
    {
        auto& fl = f == 0 ? f1L[i] : f2L[i];
        auto& fr = f == 0 ? f1R[i] : f2R[i];
        SIMDF cutl, cutr, resl, resr, drivel, driver, mixl, mixr;
        bool filterReplace = false;
        if (fl != nullptr)
        {
            filterReplace = true;
            cutl = fl->cut_targ;
            cutr = fr->cut_targ;
            resl = fl->res_targ;
            resr = fr->res_targ;
            drivel = fl->drivenorm;
            driver = fr->drivenorm;
            mixl = fl->mix;
            mixr = fr->mix;
        }
        fl = makeFilter(type);
        fr = makeFilter(type);
        fl->setMode(mode);
        fr->setMode(mode);
        fl->prepare(srate, false);
        fr->prepare(srate, false);
        if (filterReplace)
        {
            SIMDM mask = { true, true, true, true };
            fl->init(cutl, resl, true, mask);
            fr->init(cutr, resr, true, mask);
            fl->setDrive(drivel, mask);
            fr->setDrive(driver, mask);
            fl->setMix(mixl, mask);
            fr->setMix(mixr, mask);
        }
    }
}

void Synth::initFilters(int voiceId, int fid, float cutoff, float resonance, float drive, float mix)
{
    auto group = voiceId / SIMDSZ;
    auto lane = voiceId % SIMDSZ;

    auto mask = Utils::laneToMask(lane);
    auto& fl = fid == 0 ? f1L[group] : f2L[group];
    auto& fr = fid == 0 ? f1R[group] : f2R[group];

    fl->init(cutoff, resonance, true, mask);
    fr->init(cutoff, resonance, true, mask);
    fl->setDrive(drive, mask);
    fr->setDrive(drive, mask);
    fl->setMix(mix, mask);
    fr->setMix(mix, mask);
}

void Synth::updateFilters(int voiceId, int fid, float cutoff, float resonance, float drive, float mix)
{
    auto group = voiceId / SIMDSZ;
    auto lane = voiceId % SIMDSZ;

    auto mask = Utils::laneToMask(lane);
    auto& fl = fid == 0 ? f1L[group] : f2L[group];
    auto& fr = fid == 0 ? f1R[group] : f2R[group];

    Utils::setMasked(fl->cut_targ, cutoff, mask);
    Utils::setMasked(fr->cut_targ, cutoff, mask);
    Utils::setMasked(fl->res_targ, resonance, mask);
    Utils::setMasked(fr->res_targ, resonance, mask);
    fl->setDrive(drive, mask);
    fr->setDrive(drive, mask);
    fl->setMix(mix, mask);
    fr->setMix(mix, mask);
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

    int maxVoice = -1;
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

            fl->processBlock(filterInL, startSample, numSamples, Utils::maskToFloat(mask));
            fr->processBlock(filterInR, startSample, numSamples, Utils::maskToFloat(mask));

            for (int i = 0; i < numSamples; ++i)
            {
                filterOutL[i] += fl->out[i];
                filterOutR[i] += fr->out[i];
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

            fl->processBlock(filterInL, startSample, numSamples, Utils::maskToFloat(mask));
            fr->processBlock(filterInR, startSample, numSamples, Utils::maskToFloat(mask));

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