
#pragma once

#include "FX.h"
#include "../../engine/Modulation.h"
#include "../../engine/Utils.h"
#include "./VitalPhaser.h"

// Simple triangle LFO
class TriLFO
{
public:
    float phase = 0.0f;
    float rate = 0.5f;
    float srate = 44100.0f;
    int direction = 1;
	float offset = 0.f;

	void prepare(float _srate)
	{
		srate = _srate;
	}
	void setRate(float r)
	{
		rate = r;
	}
	void clear()
	{
		phase = 0.f;
	}

	float tick(float _offset = 0.f)
	{
		offset = _offset;
		phase += rate / srate;
		if (phase >= 1.f)
			phase -= 1.f;

		float p = phase + offset;
		p -= std::floor(p);

		float tri = 4.f * fabsf(p - 0.5f) - 1.f;
		return tri;
	}

	void syncToSongTime(double seconds)
	{
		double cycles = seconds * rate;
		phase = (float)(cycles - std::floor(cycles));
		phase += offset;
		phase -= std::floor(phase);
	}
};

class Phaser : public FX
{
public:
	Phaser(RipplerAudioProcessor& p, int layer);
	~Phaser() override;

	void prepare(float _srate) override;
	void processBlock(float* left, float* right, int nsamps, int blockoffset, bool audioRate) override;
	void clear() override;

private:
    float srate = 44100.f;
	std::atomic<float>* rateSyncParam = nullptr;
	std::atomic<float>* syncParam = nullptr;
	Modulation::Param* centerParam = nullptr;
	Modulation::Param* depthParam  = nullptr;
	Modulation::Param* rateParam   = nullptr;
	Modulation::Param* resParam    = nullptr;
	Modulation::Param* morphParam  = nullptr;
	Modulation::Param* stereoParam = nullptr;
	Modulation::Param* mixParam    = nullptr;

	VitalPhaser lphaser{};
	VitalPhaser rphaser{};
	Lerp lfo_center = 1000.f; // Hz
	TriLFO lfoL;
	TriLFO lfoR;
	Lerp mix = 0.f;

	float getLfoRate(int sync);
};
