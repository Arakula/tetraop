#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Voice.h"
#include "FmMatrix.h"

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
    void renderNextSubBlock(AudioBuffer<float>& outputAudio, int startSample, int numSamples) override;
    void handleMidiEvent (const juce::MidiMessage& m) override;

private:
    Voice::VoiceVec voiceVec{};
    OSC::OSCVec oscVec[MAX_OSCILLATORS]{};
    Voice::VoiceVec voiceVecOutTemp{};
    OSC::OSCVec oscVecOutTemp[MAX_OSCILLATORS]{};
    SIMDVox vox{};

	TetraOPAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Synth)
};