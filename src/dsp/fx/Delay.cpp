#include "./Delay.h"
#include "../../PluginProcessor.h"

Delay::Delay(TetraOPAudioProcessor& p)
	: FX(p, FX::Delay)
{
	modeParam = audioProcessor.params.getRawParameterValue(prefix + "mode");
	linkParam = audioProcessor.params.getRawParameterValue(prefix + "link");
	syncLParam = audioProcessor.params.getRawParameterValue(prefix + "sync_l");
	syncRParam = audioProcessor.params.getRawParameterValue(prefix + "sync_r");
	highcutParam = audioProcessor.params.getRawParameterValue(prefix + "highcut");
	lowcutParam = audioProcessor.params.getRawParameterValue(prefix + "lowcut");

	rateLParam = audioProcessor.modulation->getParamHandle(prefix + "rate_l");
	rateRParam = audioProcessor.modulation->getParamHandle(prefix + "rate_r");
	rateSyncLParam = audioProcessor.modulation->getParamHandle(prefix + "rate_sync_l");
	rateSyncRParam = audioProcessor.modulation->getParamHandle(prefix + "rate_sync_r");
	feedbackParam = audioProcessor.modulation->getParamHandle(prefix + "feedback");
	pipoWidthParam = audioProcessor.modulation->getParamHandle(prefix + "pipo_width");
	mixParam = audioProcessor.modulation->getParamHandle(prefix + "mix");
	highcutModParam = audioProcessor.modulation->getParamHandle(prefix + "highcut");
	lowcutModParam = audioProcessor.modulation->getParamHandle(prefix + "lowcut");

    audioProcessor.params.addParameterListener(prefix + "on", this);
    audioProcessor.params.addParameterListener(prefix + "bypass", this);
	audioProcessor.params.addParameterListener(prefix + "link", this);
	audioProcessor.params.addParameterListener(prefix + "sync_l", this);
	audioProcessor.params.addParameterListener(prefix + "sync_r", this);
	audioProcessor.params.addParameterListener(prefix + "rate_l", this);
	audioProcessor.params.addParameterListener(prefix + "rate_sync_l", this);
	audioProcessor.params.addParameterListener(prefix + "rate_r", this);
	audioProcessor.params.addParameterListener(prefix + "rate_sync_r", this);
}

Delay::~Delay()
{
    audioProcessor.params.removeParameterListener(prefix + "on", this);
    audioProcessor.params.removeParameterListener(prefix + "bypass", this);
	audioProcessor.params.removeParameterListener(prefix + "link", this);
    audioProcessor.params.removeParameterListener(prefix + "sync_l", this);
    audioProcessor.params.removeParameterListener(prefix + "sync_r", this);
	audioProcessor.params.removeParameterListener(prefix + "rate_l", this);
	audioProcessor.params.removeParameterListener(prefix + "rate_sync_l", this);
	audioProcessor.params.removeParameterListener(prefix + "rate_r", this);
	audioProcessor.params.removeParameterListener(prefix + "rate_sync_r", this);
}

void Delay::prepare(float _srate)
{
    srate = _srate;
    auto lpfreq_raw = highcutParam->load();
    lpfreq = std::tan(juce::MathConstants<float>::pi * lpfreq_raw / srate);
    auto hpfreq_raw = lowcutParam->load();
    hpfreq = std::tan(juce::MathConstants<float>::pi * hpfreq_raw / srate);
    auto time = getTimeSamples(false, 0);
    timeL.setup(0.25f, srate);
    timeR.setup(0.25f, srate);
    timeL.reset((float)time[0]);
    timeR.reset((float)time[1]);
    delayL.resize((int)(srate * 11));
    delayR.resize((int)(srate * 11));
    delayL.clear();
    delayR.clear();
}

std::array<int, 2> Delay::getTimeSamples(bool forceSync, int blockoffset)
{
    auto syncL = forceSync ? 1 : (int)syncLParam->load();
    auto syncR = forceSync ? 1 : (int)syncRParam->load();

    auto getSamplesSync = [this](int rate, int sync)
        {
            auto secondsPerBeat = audioProcessor.secondsPerBeat;
            if (secondsPerBeat == 0.f)
                secondsPerBeat = 0.25f;

            float qn = 1.f;
            if (rate == 0) qn = 1.f * 8.f; // 2 bar
            if (rate == 1) qn = 1.f * 4.f; // bar
            if (rate == 2) qn = 1.f * 2.f; // 1/2 bar
            if (rate == 3) qn = 1.f / 1.f; // 1/4 bar
            if (rate == 4) qn = 1.f / 2.f; // 1/8 bar
            if (rate == 5) qn = 1.f / 4.f; // 1/16 bar
            if (rate == 6) qn = 1.f / 8.f; // 1/32 bar
            if (rate == 7) qn = 1.f / 16.f; // 1/64 bar
            if (rate == 8) qn = 1.f / 32.f; // 1/128 bar
            if (sync == 2) qn *= 2 / 3.f;
            if (sync == 3) qn *= 1.5f;

            return (int)std::ceil(qn * secondsPerBeat * srate);
        };

    auto tl = syncL == 0
        ? (int)std::ceil(audioProcessor.modulation->getValue(rateLParam, false, blockoffset, srate) * srate)
        : getSamplesSync((int)audioProcessor.modulation->getValue(rateSyncLParam, false, blockoffset, srate), syncL);

    auto tr = syncR == 0
        ? (int)std::ceil(audioProcessor.modulation->getValue(rateRParam, false, blockoffset, srate) * srate)
        : getSamplesSync((int)audioProcessor.modulation->getValue(rateSyncRParam, false, blockoffset, srate), syncR);

    return { tl, tr };
}

void Delay::processBlock(float* left, float* right, int nsamps, int /*blockoffset*/, bool /*audioRate*/)
{
    auto mode = (int)modeParam->load();
    auto time = getTimeSamples(false, nsamps);

    auto feedback = audioProcessor.modulation->getValue(feedbackParam, false, nsamps, srate);
    auto pipoWidth = audioProcessor.modulation->getValue(pipoWidthParam, false, nsamps, srate);
    float lfactor = pipoWidth > 0.f ? 1.f - pipoWidth : 1.f;
    float rfactor = pipoWidth < 0.f ? 1.f + pipoWidth : 1.f;
    float feedbackL, feedbackR;

    // balance feedback between left and right delays
    float e = (float)time[0] / (float)time[1];
    if (time[0] < time[1]) {
        feedbackR = feedback;
        feedbackL = std::pow(feedback, e);
    }
    else {
        e = 1.f / e;
        feedbackL = feedback;
        feedbackR = std::pow(feedback, e);
    }

    auto mix = audioProcessor.modulation->getValue(mixParam, false, nsamps, srate);

    auto lpfreq_raw = audioProcessor.modulation->getValue(highcutModParam, false, nsamps, srate);
    if (lpfreq_raw >= 20000.f) {
        lpstate_l = 0.f;
        lpstate_r = 0.f;
    }
    auto lpfreq_targ = std::tan(juce::MathConstants<float>::pi * lpfreq_raw / srate);
    auto lpfreq_step = (lpfreq_targ - lpfreq) / nsamps;

    auto hpfreq_raw = audioProcessor.modulation->getValue(lowcutModParam, false, nsamps, srate);
    if (hpfreq_raw <= 20.f) {
        hpstate_l = 0.f;
        hpstate_r = 0.f;
    }
    auto hpfreq_targ = std::tan(juce::MathConstants<float>::pi * hpfreq_raw / srate);
    auto hpfreq_step = (hpfreq_targ - hpfreq) / nsamps;

    if (mode == 2) { // tap delay, first time is the tap length (predelay), second is the delay time
        if (predelayL.size < (int)time[0]) {
            predelayL.resize((int)std::ceil(time[0]));
            predelayR.resize((int)std::ceil(time[0]));
        }
        if (delayL.size < (int)time[1]) {
            delayL.resize((int)std::ceil(time[1]));
        }
        if (delayR.size < (int)time[1]) {
            delayR.resize((int)std::ceil(time[1]));
        }
    }
    else {
        if (delayL.size < (int)time[0]) {
            delayL.resize((int)std::ceil(time[0]));
        }
        if (delayR.size < (int)time[1]) {
            delayR.resize((int)std::ceil(time[1]));
        }
    }

    auto drymix = Utils::cosHalfPi()(mix);
    auto wetmix = Utils::sinHalfPi()(mix);

    for (int i = 0; i < nsamps; ++i) {
        auto timeLeft = timeL.process((float)time[0]);
        auto timeRight = timeR.process((float)time[1]);

        auto v0 = delayL.read(mode == 2 ? timeRight : timeLeft);
        auto v1 = delayR.read(timeRight);

        auto v0filter = v0;
        auto v1filter = v1;

        if (lpfreq_raw < 20000.f) {
            float gL = lpfreq;
            float G = gL / (1.0f + gL);

            float vL = (v0filter - lpstate_l) * G;
            float yL = vL + lpstate_l;
            lpstate_l = yL + vL;

            float vR = (v1filter - lpstate_r) * G;
            float yR = vR + lpstate_r;
            lpstate_r = yR + vR;

            v0filter = yL;
            v1filter = yR;
        }

        if (hpfreq_raw > 20.f) {
            float gH = hpfreq;
            float G = gH / (1.0f + gH);

            // Left
            float vL = (v0filter - hpstate_l) * G;
            float yLP_l = vL + hpstate_l;
            hpstate_l = yLP_l + vL;
            float hpOutL = v0filter - yLP_l;

            // Right
            float vR = (v1filter - hpstate_r) * G;
            float yLP_r = vR + hpstate_r;
            hpstate_r = yLP_r + vR;
            float hpOutR = v1filter - yLP_r;

            v0filter = hpOutL;
            v1filter = hpOutR;
        }

        if (mode == 0) { // normal
            delayL.write(left[i] + v0filter * feedbackL);
            delayR.write(right[i] + v1filter * feedbackR);
        }
        else if (mode == 1) { // ping-pong
            delayL.write(left[i] * lfactor + v1filter * feedbackL);
            delayR.write(right[i] * rfactor + v0filter * feedbackR);
        }
        else if (mode == 2) { // tap delay
            float preL = predelayL.read(timeLeft);
            float preR = predelayR.read(timeLeft);
            predelayL.write(left[i]);
            predelayR.write(right[i]);
            delayL.write(preL + v0filter * feedback);
            delayR.write(preR + v1filter * feedback);
        }

        left[i] = left[i] * drymix + v0filter * wetmix;
        right[i] = right[i] * drymix + v1filter * wetmix;

        // interpolation
        lpfreq += lpfreq_step;
        hpfreq += hpfreq_step;
    }
    lpfreq = lpfreq_targ;
    hpfreq = hpfreq_targ;
}

void Delay::parameterChanged(const juce::String& paramId, float value)
{
    if (audioProcessor.isLoadingPreset)
        return;

    if (paramId == prefix + "on" || paramId == prefix + "bypass") {
        clear();
    }

    // keep linked delay Left and Right rates in sync
    auto link = (bool)linkParam->load();

    if (paramId == prefix + "link" && (bool)value) {
        auto syncL = audioProcessor.params.getParameter(prefix + "rate_sync_l");
        auto syncR = audioProcessor.params.getParameter(prefix + "rate_sync_r");
        syncR->setValueNotifyingHost(syncL->getValue());
        auto rateL = audioProcessor.params.getParameter(prefix + "rate_l");
        auto rateR = audioProcessor.params.getParameter(prefix + "rate_r");
        rateR->setValueNotifyingHost(rateL->getValue());
        auto modeL = audioProcessor.params.getParameter(prefix + "sync_l");
        auto modeR = audioProcessor.params.getParameter(prefix + "sync_r");
        modeR->setValueNotifyingHost(modeL->getValue());
    }
    else if (paramId == prefix + "rate_sync_l") {
        if (link) {
            auto rateL = audioProcessor.params.getParameter(prefix + "rate_sync_l");
            auto rateR = audioProcessor.params.getParameter(prefix + "rate_sync_r");
            if (std::fabs(rateR->getValue() - rateL->getValue()) > 1e-5) {
                rateR->setValueNotifyingHost(rateL->getValue());
            }
        }
    }
    else if (paramId == prefix + "sync_l") {
        // if sync is turned off set rate from rate_sync
        // makes it easier for the user to offset times from project tempo
        if (value < 1.f) {
            auto time = getTimeSamples(true, 0);
            auto param = audioProcessor.params.getParameter(prefix + "rate_l");
            param->setValueNotifyingHost(param->convertTo0to1(time[0] / srate));
            if (link) {
                param = audioProcessor.params.getParameter(prefix + "rate_r");
                param->setValueNotifyingHost(param->convertTo0to1(time[0] / srate));
            }
        }
        if (link) {
            auto syncR = (int)syncRParam->load();
            if (syncR != (int)value) {
                auto param = audioProcessor.params.getParameter(prefix + "sync_r");
                param->setValueNotifyingHost(param->convertTo0to1(value));
            }
        }
    }
    else if (paramId == prefix + "sync_r") {
        // if sync is turned off set rate from rate_sync
        // makes it easier for the user to offset times from project tempo
        if (value < 1.f) {
            auto time = getTimeSamples(true, 0);
            auto param = audioProcessor.params.getParameter(prefix + "rate_r");
            param->setValueNotifyingHost(param->convertTo0to1(time[1] / srate));
            if (link) {
                param = audioProcessor.params.getParameter(prefix + "rate_l");
                param->setValueNotifyingHost(param->convertTo0to1(time[1] / srate));
            }
        }
        if (link) {
            auto syncL = (int)syncLParam->load();
            if (syncL != (int)value) {
                auto param = audioProcessor.params.getParameter(prefix + "sync_l");
                param->setValueNotifyingHost(param->convertTo0to1(value));
            }
        }
    }
    else if (paramId == prefix + "rate_sync_l") {
        if (link) {
            auto rateL = audioProcessor.params.getParameter(prefix + "rate_sync_l");
            auto rateR = audioProcessor.params.getParameter(prefix + "rate_sync_r");
            if (std::fabs(rateR->getValue() - rateL->getValue()) > 1e-5) {
                rateR->setValueNotifyingHost(rateL->getValue());
            }
        }
    }
    else if (paramId == prefix + "rate_l") {
        if (link) {
            auto rateL = audioProcessor.params.getParameter(prefix + "rate_l");
            auto rateR = audioProcessor.params.getParameter(prefix + "rate_r");
            if (std::fabs(rateR->getValue() - rateL->getValue()) > 1e-5) {
                rateR->setValueNotifyingHost(rateL->getValue());
            }
        }
    }
    else if (paramId == prefix + "rate_sync_r") {
        if (link) {
            auto rateL = audioProcessor.params.getParameter(prefix + "rate_sync_l");
            auto rateR = audioProcessor.params.getParameter(prefix + "rate_sync_r");
            if (std::fabs(rateR->getValue() - rateL->getValue()) > 1e-5) {
                rateL->setValueNotifyingHost(rateR->getValue());
            }
        }
    }
    else if (paramId == prefix + "rate_r") {
        if (link) {
            auto rateL = audioProcessor.params.getParameter(prefix + "rate_l");
            auto rateR = audioProcessor.params.getParameter(prefix + "rate_r");
            if (std::fabs(rateR->getValue() - rateL->getValue()) > 1e-5) {
                rateL->setValueNotifyingHost(rateR->getValue());
            }
        }
    }
}
