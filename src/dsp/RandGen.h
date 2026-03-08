#pragma once
#include <JuceHeader.h>
#include "../Globals.h"
#include "Pattern.h"
#include "../engine/Utils.h"

using namespace globals;

/*
* Modulator that generates noise of different types
*/
class RandGen
{
public:
	enum Mode {
		Perlin,
		Hold,
	};
	enum SyncMode {
		Rate,
		Straight,
		Tripplet,
		Dotted
	};
	struct RndVoice {
		uint32_t seed;
		float phase_offset;
	};

	static inline float perlinFade(float t) {
	    return t * t * (t * -2.f + 3.f);
	}

	static inline float perlinInterpolate(float a, float b, float t) {
	    return a + perlinFade(t) * (b - a);
	}

	// make a random based on voiceId and cycle
	static inline float makeRand(int voiceId, int cycle, uint32_t seed) {
	    uint32_t h = (uint32_t)voiceId * 0x85EBCA6Bu
	               ^ (uint32_t)cycle * 0xC2B2AE35u
	               ^ seed;

	    h ^= h >> 16;
	    h *= 0x7FEB352Du;
	    h ^= h >> 15;
	    h *= 0x846CA68Bu;
	    h ^= h >> 16;

	    return (float)(h & 0xFFFFFF) / (float)0x1000000;
	}

	bool globalSync = false;
	Mode mode = Perlin;
	float duration = 1.f;
	std::array<RndVoice, MAX_POLYPHONY + 1> voices;

	RandGen();
	~RandGen() {}

	void init(float duration, Mode mode, bool global);
	void trigger(int voiceId, float phaseOffset);
	float getValue(float elapsed, int voiceId);
};