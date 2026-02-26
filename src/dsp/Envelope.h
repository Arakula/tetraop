#pragma once
#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;

class Envelope
{
public:
	enum Stage
	{
		kDelay,
		kAttack,
		kHold,
		kDecay,
		kSustain,
		kRelease
	};
	enum Mode
	{
		ADSR,
		AHD,
		DADSR,
		DAHDSR
	};

	Envelope() {}
	~Envelope() {}

	Mode mode = ADSR;
	float del = 0.0f;
	float att = 0.2f;
	float hld = 0.0f;
	float dec = 0.2f;
	float sus = 1.0f;
	float rel = 0.5f;
	float tmax = 1.7f;
	float tenatt = 0.0f;
	float tendec = 0.0f;
	float tenrel = 0.0f;
	float pwratt = 1.0f; // power attack
	float pwrdec = 1.0f; // power decay
	float pwrrel = 1.0f; // power release
	float lrelgain = 1.f; // last calculated release gain, for display only

	void init(Mode mode, float dl, float a, float h, float d, float s, float r, float tatt, float tdec, float trel);
	float getValue(float att_t, float rel_t, float offset_t = 0.f, bool release = false);
	float getMatchingAttackTimeFromRelease(float att_t, float rel_t);

private:
	inline float calcValue(float attack_t, float release_t, float offset, bool release, float relgain) const;
};