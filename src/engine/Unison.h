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
    enum Mode
    {
        kUnison,
        kGaussian,
        kAlternate,
        kFifths,
        kSub
    };

    static Arr16f generatePhases(float phaseRand)
    {
        alignas(sizeof(SIMDF)) Arr16f out{};

        for (int i = 0; i < MAX_UNISON; ++i)
            out[i] = (rand() / (float)RAND_MAX) * phaseRand;

        return out;
    }

    static Arr16f generateDetuneRatios(int nvoices, float detune, float spread)
    {
        alignas(sizeof(SIMDF)) Arr16f out{};
        if (nvoices < 2) return out;

        bool oddVoices = (nvoices % 2 != 0);
        int centerIndex = nvoices / 2;
        float maxPos = oddVoices ? centerIndex : (nvoices * 0.5f - 0.5f);
        float imaxPos = 1.f / maxPos;
        detune *= MAX_DETUNE_CENTS;

        float exponent = spread > 0.f
            ? 1.f + spread * 3.f
            : 1.f / (1.f - spread * 3.f);

        for (int i = 0; i < nvoices; ++i)
        {
            float pos = oddVoices ? (float)i - centerIndex : i - (nvoices - 1) / 2.f;
            float normPos = pos * imaxPos;
            float warpedPos = std::copysign(std::pow(std::abs(normPos), exponent), normPos);
            float detuneCents = warpedPos * detune;
            out[i] = Utils::centsToRatio(detuneCents);
        }

        return out;
    }

    static Arr16f generateVoicesPan(int nvoices, float stereo)
    {
        alignas(sizeof(SIMDF)) Arr16f out{};
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

    static Arr16f generateVoicesGain(int nvoices, float blend, bool normalizeRMS = true)
    {
        alignas(sizeof(SIMDF)) Arr16f out {};
        if (nvoices <= 1) {
            out[0] = 1.f;
            return out;
        }
        if (nvoices == 2) {
            out[0] = normalizeRMS ? 1.f / std::sqrt(2.f) : 1.f;
            out[1] = normalizeRMS ? 1.f / std::sqrt(2.f) : 1.f;
            return out;
        }

        bool oddVoices = (nvoices % 2 != 0);
        int centerIndex = nvoices / 2;
        int n_center = oddVoices ? 1 : 2;
        int n_side = nvoices - n_center;
        int side_pairs = n_side / 2;

        float center_amp = 1.0f;
        float side_amp = blend;

        float sum_sq = n_center * center_amp * center_amp + side_pairs * side_amp * side_amp;
        float adjustment = 1.0f / std::sqrt(sum_sq);

        if (!normalizeRMS)
            adjustment = 1.f;

        for (int i = 0; i < nvoices; ++i)
        {
            bool is_center = false;

            if (oddVoices)
                is_center = (i == centerIndex);
            else
                is_center = (i == centerIndex || i == centerIndex - 1);

            out[i] = (is_center ? center_amp : side_amp) * adjustment;
        }

        return out;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Unison)
};