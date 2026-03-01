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
	for (auto& voice : voices) { 
		voice.smooth.srate = srate;
		voice.smooth.r = r;
		voice.smooth.k = k;
	}
	for (int i = 0; i < voices.size(); ++i) {
		for (auto& [key, smth] : audioRateParamSmoothCache[i]) {
			smth.r = r;
			smth.srate = srate;
		}
	}
}

void LFO::trigger(int voiceId, float phase_offset)
{
	float zero_val = pattern.get_y_at(phase_offset);
	voices[voiceId].smooth.reset(zero_val);
	voices[voiceId].phase_offset = phase_offset;
	voices[voiceId].x = 0.f;
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
		value = t * value + (1-t) * zero_val;
	}

	return value;
}

float LFO::getSmoothedValue(float elapsed, int voiceId)
{
	auto& voice = voices[voiceId];
	auto dt = fabs(elapsed - voice.x);
	if (dt < 1e-6) {
		return voice.y; // prevents multipe params from recalculating same lfo and smooth for this voice
	}

	voice.x = elapsed;

	auto value = getValue(elapsed, voice.phase_offset);
	voice.y = voice.smooth.process(value, dt);

	return voice.y;
}

float LFO::getAudioRateValue(float elapsed, float dt, int voiceId, const juce::String& param)
{
	auto& voice = voices[voiceId];

	auto id = param + String(voiceId);
	auto& smoothCache = audioRateParamSmoothCache[voiceId];
	if (!smoothCache.count(id)) {
		float zero_val = pattern.get_y_at(voice.phase_offset);
		smoothCache[id].setup(smooth, srate);
		smoothCache[id].reset(zero_val);
	}
	auto& paramSmooth = smoothCache[id];
	if (paramSmooth.resistance != smooth || paramSmooth.srate != srate) {
		paramSmooth.setup(smooth, srate);
	}

	auto value = getValue(elapsed + dt, voice.phase_offset);
	return paramSmooth.process(value, dt);
}

// used for UI display
float LFO::getXNorm(float elapsed, float phase_offset)
{
	if (mode == LFO::Envelope && elapsed - delay > duration)
		return 0.f;

	return elapsed < delay
		? fmod(phase_offset, duration) / duration
		: fmod(elapsed - delay + phase_offset, duration) / duration;
}