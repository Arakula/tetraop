#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Voice.h"

using namespace globals;

class TetraOPAudioProcessor;

class Synth : public gin::Synthesiser
{
public:
    bool lastEventWasNoteOff = false;

    Synth(TetraOPAudioProcessor& p);
    ~Synth() override;

    void handleMidiEvent (const juce::MidiMessage& m) override;

private:
	TetraOPAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Synth)
};