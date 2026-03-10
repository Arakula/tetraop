#pragma once
#include <JuceHeader.h>
#include "../Globals.h"
#include "../engine/Utils.h"

using namespace globals;
using DistFn = SIMDF(*)(SIMDF, SIMDF);
using WindowFn = void(*)(SIMDF&, const SIMDF&);

class PhaseDist
{
public:
	enum Mode
	{
		Off,
		Bend,
		Skew,
		Bias,
		Pulse,
		Sync,
		Formant,
		Quantize,
		Fold,
	};

	static constexpr float almostOne = { 1.0f - std::numeric_limits<float>::epsilon() };
	static constexpr float almostZero = std::numeric_limits<float>::epsilon();

	static inline SIMDF bypass(SIMDF phase, SIMDF)
	{
		return phase;
	}

	static inline SIMDF pulse(SIMDF phase, SIMDF amt)
	{
		phase = phase.blend(SIMDF(1.f) - phase, amt < 0.f);
		phase = (phase * (amt.abs() * 1.60943791243f).exp()).min(almostOne);
		return phase.blend(SIMDF(1.f) - phase, amt < 0.f).min(almostOne); // invert wave
	}

	static inline SIMDF quantize(SIMDF phase, SIMDF amt)
	{
		static constexpr float LN2 = 0.6931471805599453f;
		amt = (SIMDF(1.f) - mipp::sqrt(amt.abs())) * 0.95f;
		auto dist = (amt * 16.0f * LN2).exp();
		return ((phase * dist + 0.5).trunc() / dist).min(almostOne);
	}

	static inline SIMDF bias(SIMDF phase, SIMDF amt)
	{
		auto split = (SIMDF(0.5f) + amt * 0.49f).sat(0.001f, 0.999f);
		return mipp::blend(
			phase / split * 0.5f,
			(phase - split) / (SIMDF(1.f) - split) * 0.5 + 0.5f,
			phase < split
		).min(almostOne);
	}

	static inline SIMDF bend(SIMDF phase, SIMDF amt)
	{
		SIMDF one = 1.f;
		SIMDF k = one + (amt*amt).abs() * 32.f;
		auto x = mipp::blend(phase, one - phase, phase < 0.5f).max(almostZero);

		auto v_pos = mipp::exp(mipp::log(x * 2.f) * k) * 0.5f;
		auto v_neg = (one - mipp::exp(mipp::log(one - x * 2.f) * k)) * 0.5f;
		auto v = mipp::blend(v_neg, v_pos, amt < 0.f);

		return mipp::blend(v, one - v, phase < 0.5f).min(almostOne);
	}

	// Bend distortion is heavy, separate pos and neg paths
	static inline SIMDF bendPos(SIMDF phase, SIMDF amt)
	{
		SIMDF one = 1.f;
		SIMDF k = one + (amt * amt).abs() * 32.f;
		auto x = mipp::blend(phase, one - phase, phase < 0.5f).max(almostZero);
		auto v = mipp::exp(mipp::log(x * 2.f) * k) * 0.5f;
		return mipp::blend(v, one - v, phase < 0.5f).min(almostOne);
	}

	static inline SIMDF bendNeg(SIMDF phase, SIMDF amt)
	{
		SIMDF one = 1.f;
		SIMDF k = one + (amt * amt).abs() * 32.f;
		auto x = mipp::blend(phase, one - phase, phase < 0.5f).max(almostZero);
		auto v = (one - mipp::exp(mipp::log(one - x * 2.f) * k)) * 0.5f;
		return mipp::blend(v, one - v, phase < 0.5f).min(almostOne);
	}

	static inline SIMDF skew(SIMDF phase, SIMDF amt)
	{
		auto p1 = SIMDF(0.25f) + amt * 0.25f;
		auto p2 = SIMDF(0.75f) - amt * 0.25f;
		auto y1 = SIMDF(0.25f);
		auto y2 = SIMDF(0.75f);
		SIMDF one = SIMDF(1.f);

		auto r1 = phase / p1 * y1;
		auto r2 = (phase - p1) / (p2 - p1) * (y2 - y1) + y1;
		auto r3 = (phase - p2) / (one - p2) * (one - y2) + y2;

		auto r23 = blend(r2, r3, phase < p2);
		return blend(r1, r23, phase < p1).min(almostOne);
	}

	static inline SIMDF fold(SIMDF phase, SIMDF amt)
	{
		SIMDF one = 1.f;
		SIMDF half = 0.5f;
		amt = amt.abs();
		auto y = phase + phase * (one - phase) * (half - phase) * amt * 16.f;
		y = y.abs();
		return y - y.trunc();
	}

	static inline SIMDF sync(SIMDF phase, SIMDF amt)
	{
		phase = phase * (SIMDF(1.f) + (amt * amt) * 15.f);
		return phase - phase.trunc();
	}

	static inline void windowHalfSine(SIMDF& value, const SIMDF& phase)
	{
		value *= (phase * MathConstants<float>::pi).sin();
	}

	static inline void windowBypass(SIMDF&, const SIMDF&)
	{
	}
};
