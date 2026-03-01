#include "Synth.h"

Synth::Synth(TetraOPAudioProcessor& p) : audioProcessor(p)
{
    setVoiceStealingEnabled (true);

	for (int i = 0; i < MAX_POLYPHONY; i++)
    {
        auto voice = new Voice (audioProcessor, i);
        addVoice (voice);
    }
    fm = std::make_unique<FmMatrix>(audioProcessor);
    unison = std::make_unique<Unison>(audioProcessor);
}

Synth::~Synth()
{
}

void Synth::prepare()
{
    auto srate = (float)getSampleRate();
    fm->prepare(srate);
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
    unison->tick();

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

    static constexpr size_t W = 4;
    size_t numActive = activeVoices.size();

    for (auto& voice : activeVoices)
    {
        voice->startBlock(startSample, numSamples);
    }

    // process voices in batches
    for (size_t i = 0; i < numActive; i += W)
    {
        size_t batchSize = std::min(W, numActive - i);

        SIMDVox vox;

        // fetch data into arrays
        Voice::VoiceVec voiceVec{};
        OSC::OSCVec oscVec[MAX_OSCILLATORS] = {};
        for (int lane = 0; lane < batchSize; ++lane)
        {
            activeVoices[i + lane]->stateToVec(voiceVec, lane);
            for (int o = 0; o < MAX_OSCILLATORS; ++o)
            {
                activeVoices[i + lane]->osc[o].stateToVec(oscVec[o], lane, o, *unison);
            }
        }
        // convert arrays to SIMD
        vox.voice = Voice::vecToSIMD(voiceVec);
        for (int v = 0; v < 4; ++v)
        {
            vox.osc[v] = OSC::vecToSIMD(oscVec[v]);
        }

        fm->processBlock(vox, numSamples);

        for (int s = 0; s < numSamples; ++s)
        {
            left[startSample + s] += (fm->outL[s] * vox.voice.env * vox.voice.vel_mult).sum();
            right[startSample + s] += (fm->outR[s] * vox.voice.env * vox.voice.vel_mult).sum();
            vox.voice.env += vox.voice.env_step;
        }

        // update voices state
        Voice::VoiceVec voiceVecOut{};
        OSC::OSCVec oscVecOut[4] = {};
        voiceVecOut = Voice::SIMDToVec(vox.voice);
        for (int o = 0; o < MAX_OSCILLATORS; ++o)
            oscVecOut[o] = OSC::SIMDToVec(vox.osc[o]);
        for (int lane = 0; lane < batchSize; ++lane)
        {
            activeVoices[i + lane]->vecToState(voiceVecOut, lane);
            for (int o = 0; o < MAX_OSCILLATORS; ++o)
                activeVoices[i + lane]->osc[o].vecToState(oscVecOut[o], lane);
        }
    }

    for (auto& voice : activeVoices)
    {
        voice->endBlock(startSample, numSamples);
    }
}