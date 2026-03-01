#include "Unison.h"

Unison::Unison(TetraOPAudioProcessor& p)
    : audioProcessor(p)
{
    for (int i = 0; i < MAX_OSCILLATORS; ++i)
    {
        recalcUnison(i);
    }
}

Unison::~Unison()
{
}

void Unison::recalcUnison(int oscIdx)
{
    auto& o = osc[oscIdx];

    o.phaseRand = 0.f;
    o.phaseStart = 0.f;
    o.voices = oscIdx == 0 ? 2 : 1;

    for (int i = 0; i < MAX_UNISON; ++i)
    {
        o.mask[i] = 0.f;
    }

    if (o.voices == 1)
        return;

    auto pans = generateVoicesPan(oscIdx);
    auto ratios = generateDetuneRatios(oscIdx);

    for (int j = 0; j < o.voices; ++j)
    {
        auto pan = pans[j];
        auto pansqr = pan * pan;
        o.gain_l[j] = 1 - pansqr;
        o.gain_r[j] = -pansqr + pan*2.f;
        o.ratio[j] = ratios[j];
    }
}

FloatArr16x Unison::generatePhases(int oscIdx)
{

    FloatArr16x out;
    auto& o = osc[oscIdx];

    for (int i = 0; i < MAX_UNISON; ++i)
    {
        out[i] = (rand() / (float)RAND_MAX) * o.phaseRand + o.phaseStart;
        if (out[i] > 1.f) out[i] -= 1.f;
    }

    return out;
}

FloatArr16x Unison::generateDetuneRatios(int oscIdx)
{
    FloatArr16x out;
    std::fill(out.begin(), out.end(), 0.f);

    auto& o = osc[oscIdx];
    int voices = o.voices;
    if (voices < 2) return out;

    bool oddVoices = (voices % 2 != 0);
    int centerIndex = voices / 2;
    int maxPos = oddVoices ? centerIndex : (voices / 2 - 0.5);
    float imaxPos = 1.f / maxPos;

    for (int i = 0; i < voices; ++i)
    {
        float pos = oddVoices ? (float)i - centerIndex : i - (voices - 1) / 2.f;
        float detuneCents = pos * imaxPos * MAX_DETUNE_CENTS;
        out[i] = Utils::centsToRatio(detuneCents);
    }

    return out;
}

FloatArr16x Unison::generateVoicesPan(int oscIdx)
{
    FloatArr16x out;
    std::fill(out.begin(), out.end(), 0.f);

    auto& o = osc[oscIdx];
    int voices = o.voices;
    if (voices < 2) return out;

    bool oddVoices = (voices % 2 != 0);
    int centerIndex = voices / 2;
    float maxPos = oddVoices ? float(centerIndex) : (voices - 1) * 0.5f;
    float imaxPos = 1.f / maxPos;

    for (int i = 0; i < voices; ++i)
    {
        float pos = oddVoices
            ? float(i - centerIndex)
            : float(i) - (voices - 1) * 0.5f;

        float pan = pos * imaxPos;
        out[i] = (pan + 1.f) * 0.5;
    }

    return out;
}