#include <cmath>
#include "RandGen.h"

RandGen::RandGen()
{
	voices[0].seed = (uint32_t)rand();
}

void RandGen::init(float _duration, Mode _mode, bool _global)
{
	duration = _duration;
	mode = _mode;
	globalSync = _global;
}

void RandGen::trigger(int voiceId, float phaseOffset)
{
	auto& voice = voices[voiceId];
	voice.phase_offset = phaseOffset;
	voice.seed = globalSync
		? voices[0].seed
		: (uint32_t)rand();
}

float RandGen::getValue(float elapsed, int voiceId)
{
	auto& voice = voices[voiceId];
	elapsed += globalSync 
		? voices[0].phase_offset
		: 0.f;

	auto cycle = (int)(elapsed / duration);

	if (mode == Mode::Hold) {
		return makeRand(voiceId, cycle, voice.seed);
	}

	if (mode == Mode::Perlin) {
		auto rand1 = makeRand(voiceId, cycle, voice.seed);
		auto rand2 = makeRand(voiceId, cycle + 1, voice.seed);
		auto t = (elapsed - cycle * duration) / duration;

		return perlinInterpolate(rand1, rand2, t);
	}

	return 0.f;
}