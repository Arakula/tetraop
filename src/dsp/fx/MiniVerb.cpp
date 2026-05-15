#include "./MiniVerb.h"
#include "../../PluginProcessor.h"

MiniVerb::MiniVerb(TetraOPAudioProcessor& p)
	: FX(p, FX::Reverb)
{
	audioProcessor.params.addParameterListener(prefix + "on", this);
	audioProcessor.params.addParameterListener(prefix + "bypass", this);

    predelParam = audioProcessor.modulation->getParamHandle(prefix + "predel");
	sizeParam = audioProcessor.modulation->getParamHandle(prefix + "revsize");
	decayParam = audioProcessor.modulation->getParamHandle(prefix + "decay");
	modrateParam = audioProcessor.modulation->getParamHandle(prefix + "modrate");
	moddepthParam = audioProcessor.modulation->getParamHandle(prefix + "moddepth");
	mixParam = audioProcessor.modulation->getParamHandle(prefix + "mix");
	lowcutParam = audioProcessor.modulation->getParamHandle(prefix + "lowcut");
	highcutParam = audioProcessor.modulation->getParamHandle(prefix + "highcut");
}

MiniVerb::~MiniVerb()
{
	audioProcessor.params.removeParameterListener(prefix + "on", this);
	audioProcessor.params.removeParameterListener(prefix + "bypass", this);
}

void MiniVerb::clear()
{
	for (int i = 0; i < NUM_ALLPASS; i++) {
		allpassL[i].clear();
		allpassR[i].clear();
		echoL[i].clear();
		echoR[i].clear();
	}
}

void MiniVerb::parameterChanged(const juce::String& paramId, float value)
{
	(void)paramId;
	(void)value;
	for (int i = 0; i < NUM_ALLPASS; i++) {
		allpassL[i].clear();
		allpassR[i].clear();
		echoL[i].clear();
		echoR[i].clear();
        predelL.clear();
        predelR.clear();
	}
}

void MiniVerb::prepare(float _srate)
{
	srate = _srate;
	mps = srate / 343;
	distance = mps * 3.75f;

    predelL.resize((int)std::ceil(srate) + 1);
    predelR.resize((int)std::ceil(srate) + 1);

    std::array<float, NUM_ALLPASS> apLCoeffs = {8.51f, 6.21f, 3.17f, 2.21f};
    std::array<float, NUM_ALLPASS> apRCoeffs = {8.49f, 6.23f, 3.71f, 2.12f};
    std::array<float, NUM_ALLPASS> ecLCoeffs = { 47.64f, 17.62f, 23.31f, 35.31f};
    std::array<float, NUM_ALLPASS> ecRCoeffs = { 49.12f, 15.86f, 27.31f, 33.21f};

	for (int i = 0; i < NUM_ALLPASS; i++) {
		allpassL[i].init(srate, apLCoeffs[i], distance);
		allpassR[i].init(srate, apRCoeffs[i], distance);
		echoL[i].init(srate, ecLCoeffs[i], distance);
		echoR[i].init(srate, ecRCoeffs[i], distance);
	}
}

void MiniVerb::processBlock(float* left, float* right, int nsamps, int, bool)
{
    float predel = audioProcessor.modulation->getValue(predelParam, false, nsamps, srate);
    int predelSamps = (int)std::ceil(predel * srate);

	float size = audioProcessor.modulation->getValue(sizeParam, false, nsamps, srate);
	float stereo = 0.6667f - 1.6667f * size; // map size 0.1...1 to 0.5...-1 stereo
	float echomix = size * 0.3f;
	size = (0.9f - 0.9f * size);
	float decay = audioProcessor.modulation->getValue(decayParam, false, nsamps, srate);
	float modrate = audioProcessor.modulation->getValue(modrateParam, false, nsamps, srate);
	float moddepth = audioProcessor.modulation->getValue(moddepthParam, false, nsamps, srate);
	float mix = audioProcessor.modulation->getValue(mixParam, false, nsamps, srate);
	float lowcut = audioProcessor.modulation->getValue(lowcutParam, false, nsamps, srate);
	float highcut = audioProcessor.modulation->getValue(highcutParam, false, nsamps, srate);

	auto expSize = sqrt(size);
	for (int j = 0; j < NUM_ALLPASS; j++) {
		allpassL[j].setSizeOffsets(size);
		allpassR[j].setSizeOffsets(size);
		allpassL[j].initFilters(lowcut, highcut);
		allpassR[j].initFilters(lowcut, highcut);
		allpassL[j].prepareBlock(nsamps);
		allpassR[j].prepareBlock(nsamps);
		echoL[j].setSizeEcho(expSize);
		echoR[j].setSizeEcho(expSize);
		echoL[j].initFilters(lowcut, highcut);
		echoR[j].initFilters(lowcut, highcut);
		echoL[j].prepareBlock(nsamps);
		echoR[j].prepareBlock(nsamps);
	}

	for (int i = 0; i < nsamps; ++i) {
        predelL.write(left[i]);
        predelR.write(right[i]);

        auto spl0 = predelL.read(predelSamps + 1);
        auto spl1 = predelR.read(predelSamps + 1);

        spl0 = allpassL[0].allPass(spl0, moddepth * 150, modrate * 0.51f, smear * 0.75f);
        spl1 = allpassR[0].allPass(spl1, moddepth * 150, modrate * 0.53f, smear * 0.75f);
        spl0 = echoL[0].echo(spl0, moddepth * 70, modrate * 0.27f, decay, echomix);
        spl1 = echoR[0].echo(spl1, moddepth * 72, modrate * 0.25f, decay, echomix);

        auto tmp = spl0;
        spl0 = allpassL[1].allPass(spl1, moddepth * 140, modrate * 0.71f, smear * 0.72f);
        spl1 = allpassR[1].allPass(tmp, moddepth * 140, modrate * 0.61f, smear * 0.72f);
        spl0 = echoL[1].echo(spl0, moddepth * 27, modrate * 0.21f, decay, echomix);
        spl1 = echoR[1].echo(spl1, moddepth * 20, modrate * 0.21f, decay, echomix);

        tmp = spl0;
        spl0 = allpassL[2].allPass(spl1, moddepth * 130, modrate * 0.85f, smear * 0.69f);
        spl1 = allpassR[2].allPass(tmp, moddepth * 130, modrate * 0.89f, smear * 0.69f);
        tmp = spl0;
        spl0 = echoL[2].echo(spl1, moddepth * 25, modrate * 0.31f, decay, echomix);
        spl1 = echoR[2].echo(tmp, moddepth * 29, modrate * 0.33f, decay, echomix);

        tmp = spl0;
        spl0 = allpassL[3].allPass(spl1, moddepth * 120, modrate * 0.99f, smear * 0.65f);
        spl1 = allpassR[3].allPass(tmp, moddepth * 120, modrate * 1.02f, smear * 0.65f);
        auto mono = (spl0 + spl1) * 0.5f;
        spl0 = echoL[3].echo(spl0 + (mono - spl0) * stereo, moddepth * 125, modrate * 0.31f, decay, echomix);
        spl1 = echoR[3].echo(spl1 + (mono - spl1) * stereo, moddepth * 95, modrate * 0.32f, decay, echomix);

        auto drymix = Utils::cosHalfPi()(mix);
        auto wetmix = Utils::sinHalfPi()(mix);
        left[i] = left[i] * drymix + spl0 * wetmix;
        right[i] = right[i] * drymix + spl1 * wetmix;
	}
}
