#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Voice.h"
#include "FmMatrix.h"
#include "../dsp/filter/Filter.h"
#include "../dsp/filter/Digital.h"

using namespace globals;

class TetraOPAudioProcessor;

class Synth : public gin::Synthesiser, public juce::AudioProcessorValueTreeState::Listener
{
public:
    struct FilterState
    {
        Filter::Type type;
        Filter::Mode mode;
        bool on;
        bool ain; // a osc input
        bool bin;
        bool cin;
        bool din;
        bool fin; // other filter input
        bool dirty;
    };

    std::unique_ptr<FmMatrix> fm;
    bool lastEventWasNoteOff = false;

    Synth(TetraOPAudioProcessor& p);
    ~Synth() override;
    void parameterChanged(const juce::String& paramId, float value) override;

    void onFilterChange(int f);
    void prepare();
    void clear();
    void createFilters(int f);
    void initFilters(int voiceId, float cutoff, float resonance);
    void updateFilters(int voiceId, float cutoff, float resonance);
    void renderNextSubBlock(AudioBuffer<float>& outputAudio, int startSample, int numSamples) override;
    void handleMidiEvent(const juce::MidiMessage& m) override;
    std::array<SIMDVox, MAX_POLYPHONY / SIMDSZ> vox{};

private:
    // filters
    FilterState f1;
    FilterState f2;
    bool filterSeries = false;
    std::array<std::unique_ptr<Filter>, MAX_POLYPHONY / SIMDSZ> f1L;
    std::array<std::unique_ptr<Filter>, MAX_POLYPHONY / SIMDSZ> f1R;
    std::array<std::unique_ptr<Filter>, MAX_POLYPHONY / SIMDSZ> f2L;
    std::array<std::unique_ptr<Filter>, MAX_POLYPHONY / SIMDSZ> f2R;
    std::array<SIMDF, MAX_BLOCKSIZE> filterInL;
    std::array<SIMDF, MAX_BLOCKSIZE> filterInR;
    std::array<SIMDF, MAX_BLOCKSIZE> filterOutL;
    std::array<SIMDF, MAX_BLOCKSIZE> filterOutR;

    std::array<SIMDF, MAX_BLOCKSIZE> outL;
    std::array<SIMDF, MAX_BLOCKSIZE> outR;

    DCBlocker dcBlockerL{};
    DCBlocker dcBlockerR{};

	TetraOPAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Synth)
};