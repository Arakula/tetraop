#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Voice.h"
#include "FmMatrix.h"
#include "Unison.h"

using namespace globals;

class TetraOPAudioProcessor;

class Synth : public gin::Synthesiser
{
public:
    std::unique_ptr<FmMatrix> fm;
    std::unique_ptr<Unison> unison;
    bool lastEventWasNoteOff = false;

    Synth(TetraOPAudioProcessor& p);
    ~Synth() override;

    void prepare();
    void renderNextSubBlock(AudioBuffer<float>& outputAudio, int startSample, int numSamples) override;
    void handleMidiEvent (const juce::MidiMessage& m) override;

private:
	TetraOPAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Synth)
};