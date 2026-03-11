// Copyright 2026 tilr
// Based off Vital synth one_pole
#pragma once
#include <JuceHeader.h>
#include "Filter.h"
#include "../../engine/Utils.h"

class OnePole {
public:
    SIMDF coeff = 0.0;
    SIMDF state = 0.0;
    SIMDF curr = 0.0;

    OnePole() {}

    void init(SIMDF freq, float srate) {
        static constexpr float kMaxRads = 0.499f * juce::MathConstants<float>::pi;
        auto x = freq.min(srate * 0.48f) * (MathConstants<float>::pi / srate);
        coeff = x.min(kMaxRads).tan();
    }

    SIMDF eval(SIMDF sample) {
        SIMDF delta = coeff * (sample - state);
        state += delta;
        curr = state;
        state += delta;
        return curr;
    }

    void reset(SIMDF sample, SIMDM mask) {
        Utils::setMasked(state, sample, mask);
        Utils::setMasked(curr, sample, mask);
    }
};