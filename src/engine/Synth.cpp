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
        voice->startBlock(startSample, numSamples);

    // process voices in batches
    for (size_t i = 0; i < numActive; i += W)
    {
        size_t batchSize = std::min(W, numActive - i);

        // clear memory used to batch data into SIMD lanes
        std::memset(&voiceVec, 0, sizeof(voiceVec));
        for (int o = 0; o < MAX_OSCILLATORS; ++o)
            std::memset(&oscVec[o], 0, sizeof(oscVec[o]));

        // fetch data into arrays
        for (int lane = 0; lane < batchSize; ++lane)
        {
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

        fm->processBlock(vox, numSamples);

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
    }

    for (auto& voice : activeVoices)
        voice->endBlock(startSample, numSamples);
}