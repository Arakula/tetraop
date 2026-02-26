#include "Synth.h"

Synth::Synth(TetraOPAudioProcessor& p) : audioProcessor(p)
{
	enableLegacyMode();
    setVoiceStealingEnabled (true);

	for (int i = 0; i < MAX_POLYPHONY; i++)
    {
        auto voice = new Voice (audioProcessor, i);
        addVoice (voice);
    }
}

Synth::~Synth()
{
}

void Synth::handleMidiEvent (const juce::MidiMessage& m)
{
    MPESynthesiser::handleMidiEvent (m);
}