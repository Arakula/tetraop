/*
    Copyright © tilr

    Because Unison params are global to all voices, they are stored and computed in this class
    Only Unison phases and phase increments are kept in the oscillator instances
*/
#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Utils.h"

using namespace globals;
using FloatArr16x = std::array<float, MAX_UNISON>;

class TetraOPAudioProcessor;

class Unison
{
public:
    struct UnisonOSC
    {
        alignas(sizeof(SIMDF)) FloatArr16x mask;
        alignas(sizeof(SIMDF)) FloatArr16x gain_l;
        alignas(sizeof(SIMDF)) FloatArr16x gain_r;
        alignas(sizeof(SIMDF)) FloatArr16x ratio;
        int voices;
        float phaseRand;
        float phaseStart;
    };

    UnisonOSC osc[MAX_OSCILLATORS];

    Unison(TetraOPAudioProcessor& p);
    ~Unison();

    void recalcUnison(int oscIdx);
    FloatArr16x generatePhases(int oscIdx);
    FloatArr16x generateDetuneRatios(int oscIdx);
    FloatArr16x generateVoicesPan(int oscIdx);

private:
	TetraOPAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Unison)
};