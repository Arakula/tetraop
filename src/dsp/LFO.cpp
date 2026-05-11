#include <cmath>
#include "LFO.h"

LFO::LFO()
{
	pattern.insertPoint(0.f, 0.f, 0.f, 1);
	pattern.insertPoint(0.5f, 1.f, 0.f, 1);
	pattern.buildSegments();
}

void LFO::init(double _srate, float _duration, float _delay, float _rise, Mode _mode)
{
	srate = (float)_srate;

	// lfos are absolute
	// compensate lfo phase when the duration changes
	// so that the lfo does not jump on rate change
	if (duration != _duration) {
		for (auto& voice : voices) {
			float currentPhase = std::fmod((voice.x + voice.phase_offset) / duration, 1.0f);
			voice.phase_offset = currentPhase * _duration - voice.x;
		}
	}

	duration = _duration;
	rise = _rise;
	delay = _delay;
	mode = _mode;
}

void LFO::setSmooth(float _smooth)
{
	smooth = _smooth;
	auto r = 1.0f / (smooth * srate + 1);
	auto k = smooth <= 0.f ? 0.f : -srate * std::log(1.f - r);
	for (int i = 0; i < int(voices.size()); ++i) {
		for (auto& [key, smth] : audioRateParamSmoothCache[i]) {
			smth.r = r;
			smth.k = k;
			smth.srate = srate;
		}
	}
}

void LFO::trigger(int voiceId, float phase_offset, float lastGlobalValue)
{
	voices[voiceId].phase_offset = phase_offset;
	voices[voiceId].x = 0.f;

	float zero_val = mode == Sync
		? lastGlobalValue
		: pattern.get_y_at(std::fmod(phase_offset, duration) / duration);

	for (auto& [key, smth] : audioRateParamSmoothCache[voiceId]) {
		smth.reset(zero_val);
	}
}

float LFO::getValue(float elapsed, float phase_offset)
{
	if (elapsed <= delay)
		return pattern.get_y_at(std::fmod(phase_offset, duration) / duration);

	elapsed = elapsed - delay;

	if (mode == Mode::Envelope && elapsed > duration) {
		elapsed = duration;
	}

	float value = pattern.get_y_at(std::fmod(elapsed + phase_offset, duration) / duration);
	if (elapsed < rise) {
		float zero_val = pattern.get_y_at(std::fmod(phase_offset, duration) / duration);
		float t = elapsed / rise;
		value = t * value + (1 - t) * zero_val;
	}

	return value;
}

float LFO::getSmoothedValue(float elapsed, float dt, int voiceId, juce::String param)
{
	auto& voice = voices[voiceId];
	auto id = param + juce::String(voiceId);

	auto& smoothCache = audioRateParamSmoothCache[voiceId];
	if (!smoothCache.count(id)) {
		float zero_val = pattern.get_y_at(std::fmod(voice.phase_offset, duration) / duration);
		smoothCache[id].setup(smooth, srate);
		smoothCache[id].reset(zero_val);
	}

	auto& paramSmooth = smoothCache[id];
	if (paramSmooth.resistance != smooth || paramSmooth.srate != srate) {
		paramSmooth.setup(smooth, srate);
	}

	voice.x = elapsed + dt; // keep track of elapsed time for global LFO

	auto value = getValue(elapsed + dt, voice.phase_offset);
	voice.y = paramSmooth.process(value, dt); // keep track of lfo value for global LFO
	return voice.y;
}

// used for UI display
float LFO::getXNorm(float elapsed, float phase_offset) const
{
	if (mode == LFO::Envelope && elapsed - delay > duration)
		return 0.f;

	return elapsed < delay
		? fmod(phase_offset, duration) / duration
		: fmod(elapsed - delay + phase_offset, duration) / duration;
}
