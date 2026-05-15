#pragma once
#include "JuceHeader.h"
#include "../../Globals.h"

using namespace globals;
class TetraOPAudioProcessor;

class FX
{
public:
	enum FXType
	{
		Compressor,
		Distortion,
		Chorus,
		Phaser,
		Delay,
		Reverb,
		EQ,
		kFXs
	};

	static constexpr std::array<std::string_view, kFXs> FXPrefix = {
		"fx_comp_",
		"fx_dist_",
		"fx_chorus_",
		"fx_phaser_",
		"fx_delay_",
		"fx_reverb_",
		"fx_eq_",
	};

	static constexpr std::array<std::string_view, kFXs> FXName = {
		"Compressor",
		"Distortion",
		"Chorus",
		"Phaser",
		"Delay",
		"Reverb",
		"EQ",
	};

	static juce::Colour getColor(FXType type) {
		switch(type) {
			case(FX::Compressor): return COLOR_COMPRESSOR();
			case(FX::Distortion): return COLOR_DISTORTION();
			case(FX::Chorus): return COLOR_CHORUS();
			case(FX::Phaser): return COLOR_CHORUS();
			case(FX::Delay): return COLOR_DELAY();
			case(FX::Reverb): return COLOR_REVERB();
			case(FX::EQ): return COLOR_EQ();
            case FX::kFXs:
                jassertfalse;
                break;
		}
		return COLOR_VIBRATO();
	}

	FX(TetraOPAudioProcessor& p, FXType type) : audioProcessor(p), type(type)
	{
		prefix = juce::String(FXPrefix[type].data());
	}

	virtual ~FX() = default;

	virtual void processBlock(float* left, float* right, int nsamples, int blockoffset, bool audioRate) = 0;
	virtual void prepare(float srate) = 0;
	virtual void clear() = 0;

	juce::String prefix = "";
	TetraOPAudioProcessor& audioProcessor;
	FXType type;
	std::atomic<bool> active = false;
	std::atomic<bool> bypass = false;
};
