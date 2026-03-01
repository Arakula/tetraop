/*
    Copyright © tilr 2026
*/
#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Utils.h"

using namespace globals;
using Arr16f = std::array<float, MAX_UNISON>;

class Unison
{
public:
    static Arr16f generatePhases(float phaseStart, float phaseRand)
    {
        alignas(sizeof(SIMDF)) Arr16f out{};

        for (int i = 0; i < MAX_UNISON; ++i)
        {
            out[i] = (rand() / (float)RAND_MAX) * phaseRand + phaseStart;
            if (out[i] > 1.f) out[i] -= 1.f;
        }

        return out;
    }

    static Arr16f generateDetuneRatios(int nvoices, float detune)
    {
        alignas(sizeof(SIMDF)) Arr16f out{};
        std::fill(out.begin(), out.end(), 0.f);

        if (nvoices < 2) return out;

        bool oddVoices = (nvoices % 2 != 0);
        int centerIndex = nvoices / 2;
        float maxPos = oddVoices ? centerIndex : (nvoices * 0.5f - 0.5f);
        float imaxPos = 1.f / maxPos;
        detune *= MAX_DETUNE_CENTS;

        for (int i = 0; i < nvoices; ++i)
        {
            float pos = oddVoices ? (float)i - centerIndex : i - (nvoices - 1) / 2.f;
            float detuneCents = pos * imaxPos * detune;
            out[i] = Utils::centsToRatio(detuneCents);
        }

        return out;
    }

    static Arr16f generateVoicesPan(int nvoices, float stereo)
    {
        alignas(sizeof(SIMDF)) Arr16f out{};
        std::fill(out.begin(), out.end(), 0.f);

        if (nvoices < 2) return out;

        bool oddVoices = (nvoices % 2 != 0);
        int centerIndex = nvoices / 2;
        float maxPos = oddVoices ? float(centerIndex) : (nvoices - 1) * 0.5f;
        float imaxPos = (1.f / maxPos) * stereo;

        for (int i = 0; i < nvoices; ++i)
        {
            float pos = oddVoices
                ? float(i - centerIndex)
                : float(i) - (nvoices - 1) * 0.5f;

            float pan = pos * imaxPos;
            out[i] = (pan + 1.f) * 0.5f;
        }

        return out;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Unison)
};