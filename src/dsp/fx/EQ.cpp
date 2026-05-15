#include "./EQ.h"
#include "../../PluginProcessor.h"

EQ::EQ(RipplerAudioProcessor& p, int _layer) : FX(p, FX::EQ, _layer)
{
	juce::String prelayer = layer == 0 ? "m_" : layer == 1 ? "l1_" : "l2_";
	for (int i = 0; i < globals::EQ_BANDS; i++) {
		auto pre = prelayer + "fx_eq_band" + juce::String(i + 1);
		freqParams[i] = audioProcessor.params.getRawParameterValue(pre + "_freq");
		gainParams[i] = audioProcessor.params.getRawParameterValue(pre + "_gain");
		qParams[i] = audioProcessor.params.getRawParameterValue(pre + "_q");
		if (i == 0 || i == globals::EQ_BANDS - 1)
			modeParams[i] = audioProcessor.params.getRawParameterValue(pre + "_mode");
		freqModParams[i] = audioProcessor.modulation->getParamHandle(pre + "_freq");
		gainModParams[i] = audioProcessor.modulation->getParamHandle(pre + "_gain");
		qModParams[i] = audioProcessor.modulation->getParamHandle(pre + "_q");
	}

	audioProcessor.params.addParameterListener(prefix + "on", this);
	audioProcessor.params.addParameterListener(prefix + "bypass", this);
	audioProcessor.params.addParameterListener(prefix + "band1_mode", this);
	audioProcessor.params.addParameterListener(prefix + "band" + juce::String(globals::EQ_BANDS) + "_mode", this);
}

EQ::~EQ()
{
	audioProcessor.params.removeParameterListener(prefix + "on", this);
	audioProcessor.params.removeParameterListener(prefix + "bypass", this);
	audioProcessor.params.removeParameterListener(prefix + "band1_mode", this);
	audioProcessor.params.removeParameterListener(prefix + "band" + juce::String(globals::EQ_BANDS) + "_mode", this);
}

void EQ::clear()
{
	for (int i = 0; i < globals::EQ_BANDS; ++i) {
		bandsL[i].clear(0.f);
		bandsR[i].clear(0.f);
	}
}

void EQ::parameterChanged(const juce::String& paramId, float value)
{
	(void)paramId;
	(void)value;
	initFilters();
}

void EQ::prepare(float _srate)
{
	srate = _srate;
	initFilters();
}

void EQ::initFilters() 
{
	for (int i = 0; i < globals::EQ_BANDS; i++) {
		auto& bandL = bandsL[i];
		auto& bandR = bandsR[i];
		bandL.clear(0.f);
		bandR.clear(0.f);
		auto freq = freqParams[i]->load();
		auto gain = gainParams[i]->load();
		gain = exp(gain * globals::DB2LOG);
		auto q = qParams[i]->load();
		if (i == 0) {
			auto mode = modeParams[i]->load();
			if (mode == 0) {
				bandL.hp(srate, freq, q);
				bandR.hp(srate, freq, q);
			}
			else {
				bandL.ls(srate, freq, q, gain);
				bandR.ls(srate, freq, q, gain);
			}
		}
		else if (i == globals::EQ_BANDS - 1) {
			auto mode = modeParams[i]->load();
			if (mode == 0) {
				bandL.lp(srate, freq, q);
				bandR.lp(srate, freq, q);
			}
			else {
				bandL.hs(srate, freq, q, gain);
				bandR.hs(srate, freq, q, gain);
			}
		}
		else {
			bandL.pk(srate, freq, q, gain);
			bandR.pk(srate, freq, q, gain);
		}
	}
}

void EQ::processBlock(float* left, float* right, int nsamps, int /*blockoffset*/, bool /*audioRate*/)
{
	for (int i = 0; i < globals::EQ_BANDS; i++) {
		auto& bandL = bandsL[i];
		auto& bandR = bandsR[i];
		auto freq = audioProcessor.modulation->getValue(freqModParams[i], false, nsamps, srate);
		auto gain = audioProcessor.modulation->getValue(gainModParams[i], false, nsamps, srate);
		gain = exp(gain * globals::DB2LOG);
		auto q = audioProcessor.modulation->getValue(qModParams[i], false, nsamps, srate);
		bandL.processBlock(left, nsamps, 0, nsamps, freq, q, gain);
		bandR.processBlock(right, nsamps, 0, nsamps, freq, q, gain);
	}
}
