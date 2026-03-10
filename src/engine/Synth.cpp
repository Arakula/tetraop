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

    initFilters(0);
    initFilters(1);

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

    for (auto& filter : filterL)
        filter->prepare(srate);
    for (auto& filter : filterR)
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

void Synth::initFilters(int f)
{
    String prefix = f == 0 ? "f1_" : "f2_";
    auto type = (Filter::Type)audioProcessor.params.getRawParameterValue(prefix + "type")->load();
    auto mode = (Filter::Mode)audioProcessor.params.getRawParameterValue(prefix + "mode")->load();

    for (int i = 0; i < MAX_POLYPHONY / SIMDSZ; ++i)
    {
        filterL[i] = makeFilter(type);
        filterR[i] = makeFilter(type);
        filterL[i]->setMode(mode);
        filterR[i]->setMode(mode);
    }
}

void Synth::prepareFilters(int voiceId, float cutoff, float resonance)
{
    auto group = voiceId / SIMDSZ;
    auto lane = voiceId % SIMDSZ;

    auto mask = Utils::laneToMask(lane);
    filterL[group]->init(cutoff, resonance, true, mask);
    filterR[group]->init(cutoff, resonance, true, mask);
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
        for (auto& o : voice->osc)
            o.startBlock(startSample, numSamples);
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
        fm->processBlock(v, numSamples, activeVoice, mask);

        for (int s = 0; s < numSamples; ++s)
        {
            left[startSample + s] += (fm->outL[s] * v.voice.env * v.voice.vel_mult).sum();
            right[startSample + s] += (fm->outR[s] * v.voice.env * v.voice.vel_mult).sum();
            v.voice.env += v.voice.env_step;
        }
    }

    // process active voices in batches
    // voices are serialized into SIMD registers and scattered after
    // its not a great approach but works well enough
    /*
    for (size_t i = 0; i < numActive; i += W)
    {
        size_t batchSize = std::min(W, numActive - i);

        // clear memory used to batch data into SIMD lanes
        std::memset(&voiceVec, 0, sizeof(voiceVec));
        for (int o = 0; o < MAX_OSCILLATORS; ++o)
            std::memset(&oscVec[o], 0, sizeof(oscVec[o]));

        int activeVoice = -1; // used to retrieve oscillator outputs for visualization

        // fetch data into arrays
        for (int lane = 0; lane < batchSize; ++lane)
        {
            if (activeVoices[i + lane]->id == audioProcessor.modulation->lastUsedVoice)
            {
                activeVoice = lane;
            }

            activeVoices[i + lane]->stateToVec(voiceVec, lane);
            for (int o = 0; o < MAX_OSCILLATORS; ++o)
            {
                activeVoices[i + lane]->osc[o].prepareBlock(startSample, numSamples);
                activeVoices[i + lane]->osc[o].stateToVec(oscVec[o], lane, fm->isOut[o], numSamples);
            }
        }
        // convert arrays to SIMD
        vox.voice = Voice::vecToSIMD(voiceVec);
        for (int v = 0; v < 4; ++v)
        {
            vox.osc[v] = OSC::vecToSIMD(oscVec[v]);
        }

        // process oscillators
        fm->processBlock(vox, numSamples, activeVoice);

        for (int s = 0; s < numSamples; ++s)
        {
            left[startSample + s] += (fm->outL[s] * vox.voice.env * vox.voice.vel_mult).sum();
            right[startSample + s] += (fm->outR[s] * vox.voice.env * vox.voice.vel_mult).sum();
            vox.voice.env += vox.voice.env_step;
        }

        // update voices state
        voiceVecOutTemp = Voice::SIMDToVec(vox.voice);
        for (int o = 0; o < MAX_OSCILLATORS; ++o)
            oscVecOutTemp[o] = OSC::SIMDToVec(vox.osc[o]);
        for (int lane = 0; lane < batchSize; ++lane)
        {
            activeVoices[i + lane]->vecToState(voiceVecOutTemp, lane);
            for (int o = 0; o < MAX_OSCILLATORS; ++o)
                activeVoices[i + lane]->osc[o].vecToState(oscVecOutTemp[o], lane);
        }
    }*/

    

    // process filters
    // filters are ordered by voice number in SIMD registers
    // instead of serialized and scattered like FM processing
    // process the registers up to highest active voice
    //for (int f = 0; f <= maxVoice / SIMDSZ; ++f)
    //{
    //    auto& fl = filterL[f];
    //    auto& fr = filterR[f];
    //
    //    bool mask[4] = { false, false, false, false };
    //    float cut[4] = { 0.f, 0.f, 0.f, 0.f };
    //    float res[4] = { 0.f, 0.f, 0.f, 0.f };
    //
    //    for (int v = 0; v < SIMDSZ; ++v)
    //    {
    //        auto idx = f * SIMDSZ + v;
    //        auto voice = (Voice*)voices[idx];
    //        if (voice->isActive())
    //        {
    //            mask[v] = true;
    //            cut[v] = voice->f1_cut;
    //            res[v] = voice->f1_res;
    //        }
    //    }
    //
    //    SIMDM msk = mask;
    //    if (msk.testz())
    //        continue; // all voices inactive
    //
    //    fl->setTargets(cut, res, msk);
    //    fr->setTargets(cut, res, msk);
    //    fl->processBlock(left, startSample, numSamples, audioProcessor.currBlockSize, msk);
    //    fr->processBlock(right, startSample, numSamples, audioProcessor.currBlockSize, msk);
    //}

    //for (int s = 0; s < numSamples; ++s)
    //{
    //    auto idx = startSample + s;
    //    left[idx] = 0.f;
    //    right[idx] = 0.f;
    //}

    // mix filters into output
    //for (int f = 0; f <= maxVoice / SIMDSZ; ++f)
    //{
    //    auto& fl = filterL[f];
    //    auto& fr = filterR[f];
    //
    //    for (int s = 0; s < numSamples; ++s)
    //    {
    //        auto idx = startSample + s;
    //        left[idx] += fl->out[s];
    //        right[idx] += fr->out[s];
    //    }
    //}

    // final dc blocking
    for (int i = 0; i < numSamples; ++i)
    {
        left[startSample + i] = dcBlockerL.process(left[startSample + i]);
        right[startSample + i] = dcBlockerR.process(right[startSample + i]);
    }

    for (auto& voice : activeVoices) {
        voice->endBlock(startSample, numSamples);
        for (int o = 0; o < MAX_OSCILLATORS; ++o)
            voice->osc[o].endBlock(numSamples);
    }
}