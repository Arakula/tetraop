#pragma once
#include <JuceHeader.h>
#include "../Globals.h"
#include "Pattern.h"
#include "../engine/Utils.h"

using namespace globals;

class LFO
{
public:
	struct Voice {
		RCFilterBlock smooth{};
		float x = 0.0f;
		float y = 0.0f;
		float phase_offset = 0.0f;
	};
	enum Mode {
		Trigger,
		Sync,
		Envelope,
	};
	enum SyncMode {
		Rate,
		Straight,
		Tripplet,
		Dotted
	};

	Pattern pattern{ -1 };
	std::array<Voice, MAX_POLYPHONY + 1> voices{}; // voice 0 is the global voice, voiceId is N+1
	float srate = 44100.0f;
	float delay = 0.f;
	float duration = 1.0; // lfo duration in seconds
	float rise = 0.0f;
	float smooth = -1.f;
	Mode mode = Trigger;

	LFO();
	~LFO() {}

	void init(double srate, float duration, float delay, float rise, Mode mode);
	void trigger(int voiceId, float phase_offset);
	void setSmooth(float smooth);
	float getValue(float elapsed, float phase_offset);
	float getSmoothedValue(float elapsed, int voiceId);
	float getAudioRateValue(float elapsed, float dt, int voiceId, std::string param);
	float getXNorm(float elapsed, float phase_offset);

	std::array<std::unordered_map<std::string, RCFilterBlock>, MAX_POLYPHONY + 1> audioRateParamSmoothCache;
};