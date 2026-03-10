#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Voice.h"
#include "FmMatrix.h"
#include "../dsp/filter/Filter.h"
#include "../dsp/filter/Digital.h"

using namespace globals;

class TetraOPAudioProcessor;

class Synth : public gin::Synthesiser
{
public:
    std::unique_ptr<FmMatrix> fm;
    bool lastEventWasNoteOff = false;

    Synth(TetraOPAudioProcessor& p);
    ~Synth() override;

    void prepare();
    void clear();
    void initFilters(int f);
    void prepareFilters(int voiceId, float cutoff, float resonance);
    void renderNextSubBlock(AudioBuffer<float>& outputAudio, int startSample, int numSamples) override;
    void handleMidiEvent(const juce::MidiMessage& m) override;

private:
    std::array<std::unique_ptr<Filter>, MAX_POLYPHONY / SIMDSZ> filterL;
    std::array<std::unique_ptr<Filter>, MAX_POLYPHONY / SIMDSZ> filterR;

    DCBlocker dcBlockerL{};
    DCBlocker dcBlockerR{};
    Voice::VoiceVec voiceVec{};
    OSC::OSCVec oscVec[MAX_OSCILLATORS]{};
    Voice::VoiceVec voiceVecOutTemp{};
    OSC::OSCVec oscVecOutTemp[MAX_OSCILLATORS]{};
    SIMDVox vox{};

	TetraOPAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Synth)
};