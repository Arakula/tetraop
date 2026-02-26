#include <cmath>
#include "Envelope.h"

inline float Envelope::calcValue(float attack_t, float release_t, float offset, bool release, float relgain) const
{
	float t = mode == AHD
		? attack_t + release_t
		: release ? release_t : attack_t;

	t += offset;

	Envelope::Stage stg = release && mode != AHD ? kRelease
		: t < del ? kDelay
		: t < del + att ? kAttack
		: t < del + att + hld ? kHold
		: t < del + att + hld + dec ? kDecay
		: kSustain;

	if (stg == kDelay) return 0.0f;
	if (stg == kHold) return 1.0f;
	if (stg == kSustain) return sus;
	if (stg == kRelease && t > rel) return 0.0;

	float ten = stg == kAttack ? -tenatt
		: stg == kDecay ? tendec : tenrel;

    float pwr = stg == kAttack ? pwratt
		: stg == kDecay ? pwrdec : pwrrel;

	float x = stg == kAttack ? (t - del) / att
        : stg == kDecay ? (t - (del + att + hld)) / dec
        : t / rel;

	x = std::fmin(std::fmax(x, 0.0f), 1.0f);

	float y1 = stg == kAttack ? 0.0f
		: stg == kDecay ? 1.0f : relgain;

	float y2 = stg == kAttack ? 1.0f
		: stg == kDecay ? sus : 0.0f;

    return ten >= 0
		? std::pow(x, pwr) * (y2 - y1) + y1
    	: -1 * (std::pow(1 - x, pwr) - 1) * (y2 - y1) + y1;
}

void Envelope::init(Mode _mode, float delay, float a, float h, float d, float s, float r, float tatt, float tdec, float trel)
{
	mode = _mode;
	del = mode == DADSR || mode == DAHDSR ? delay : 0.f;
	att = a;
	hld = mode == AHD || mode == DAHDSR ? h : 0.f;
	dec = d;
	sus = mode != AHD ? s : 0.f;
	rel = mode != AHD ? r : 0.00001f;
	tenatt = tatt;
	tendec = tdec;
	tenrel = trel;
	tmax = delay + a + h + d + r;
	pwratt = pow(1.1f, std::fabs(tatt * POWER_CURVE_POWER));
	pwrdec = pow(1.1f, std::fabs(tdec * POWER_CURVE_POWER));
	pwrrel = pow(1.1f, std::fabs(trel * POWER_CURVE_POWER));
}

float Envelope::getValue(float attack_t, float release_t, float offset_t, bool release)
{
	float relgain = release ? getValue(attack_t, 0.f) : 1.f;
	lrelgain = relgain;

    return calcValue(attack_t, release_t, offset_t, release, relgain);
}

float Envelope::getMatchingAttackTimeFromRelease(float attack_t, float release_t)
{
	float y1 = getValue(attack_t, 0.f); // get release start y
	float v = calcValue(attack_t, release_t, 0.f, true, y1); // get release y

	return tenatt >= 0 
		? std::pow(v, 1.0f / pwratt) * attack_t
		: 1.0f - std::pow(1.0f - v, 1.0f / pwratt);
}