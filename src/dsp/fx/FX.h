#pragma once
#include "JuceHeader.h"
#include "../../Globals.h"

class RipplerAudioProcessor;

class FX
{
public:
	enum FXType
	{
		Vibrato,
		Stereoizer,
		Compressor,
		Distortion,
		Tremolo,
		Chorus,
		Flanger,
		Phaser,
		Delay,
		BucketDelay,
		Reverb,
		Particle,
		Magnetic,
		EQ,
		Limiter,
		kFXs
	};

	static constexpr std::array<std::string_view, kFXs> FXPrefix = {
		"fx_vib_",
		"fx_stereo_",
		"fx_comp_",
		"fx_dist_",
		"fx_trem_",
		"fx_chorus_",
		"fx_flanger_",
		"fx_phaser_",
		"fx_delay_",
		"fx_bucket_",
		"fx_reverb_",
		"fx_particle_",
		"fx_magnetic_",
		"fx_eq_",
		"fx_limiter_"
	};

	static constexpr std::array<std::string_view, kFXs> FXName = {
		"Vibrato",
		"Stereoizer",
		"Compressor",
		"Distortion",
		"Tremolo",
		"Chorus",
		"Flanger",
		"Phaser",
		"Delay",
		"Bucket Delay",
		"Reverb",
		"Particle",
		"Magnetic",
		"EQ",
		"Limiter"
	};

	static juce::Colour getColor(FXType type) {
		switch(type) {
			case(FX::Vibrato): return COLOR_VIBRATO();
			case(FX::Stereoizer): return COLOR_STEREOIZER();
			case(FX::Compressor): return COLOR_COMPRESSOR();
			case(FX::Distortion): return COLOR_DISTORTION();
			case(FX::Tremolo): return COLOR_TREMOLO();
			case(FX::Chorus): return COLOR_CHORUS();
			case(FX::Flanger): return COLOR_CHORUS();
			case(FX::Phaser): return COLOR_CHORUS();
			case(FX::Delay): return COLOR_DELAY();
			case(FX::BucketDelay): return COLOR_DELAY();
			case(FX::Reverb): return COLOR_REVERB();
			case(FX::Particle): return COLOR_REVERB();
			case(FX::Magnetic): return COLOR_TREMOLO();
			case(FX::EQ): return COLOR_EQ();
			case(FX::Limiter): return COLOR_EQ();
            case FX::kFXs:
                jassertfalse;
                break;
		}
		return COLOR_VIBRATO();
	}

	FX(RipplerAudioProcessor& p, FXType type, int layer) : audioProcessor(p), type(type), layer(layer)
	{
		prefix = juce::String(FXPrefix[type].data());
	}

	virtual ~FX() = default;

	virtual void processBlock(float* left, float* right, int nsamples, int blockoffset, bool audioRate) = 0;
	virtual void prepare(float srate) = 0;
	virtual void clear() = 0;

	juce::String prefix = "";
	RipplerAudioProcessor& audioProcessor;
	FXType type;
	std::atomic<bool> active = false;
	std::atomic<bool> bypass = false;
	int layer;
};
