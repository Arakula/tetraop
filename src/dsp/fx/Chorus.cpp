#include "./Chorus.h"
#include "../../PluginProcessor.h"

Chorus::Chorus(TetraOPAudioProcessor& p)
	: FX(p, FX::Chorus)
{
	voicesParam = audioProcessor.params.getRawParameterValue(prefix + "voices");
	rateParam = audioProcessor.params.getRawParameterValue(prefix + "rate");
	mixParam = audioProcessor.params.getRawParameterValue(prefix + "mix");
	depthParam = audioProcessor.params.getRawParameterValue(prefix + "depth");
	highcutParam = audioProcessor.params.getRawParameterValue(prefix + "highcut");
	lowcutParam = audioProcessor.params.getRawParameterValue(prefix + "lowcut");
	feedbackParam = audioProcessor.params.getRawParameterValue(prefix + "feedback");

	rateModParam = audioProcessor.modulation->getParamHandle(prefix + "rate");
	mixModParam = audioProcessor.modulation->getParamHandle(prefix + "mix");
	depthModParam = audioProcessor.modulation->getParamHandle(prefix + "depth");
	highcutModParam = audioProcessor.modulation->getParamHandle(prefix + "highcut");
	lowcutModParam = audioProcessor.modulation->getParamHandle(prefix + "lowcut");
	feedbackModParam = audioProcessor.modulation->getParamHandle(prefix + "feedback");
}

void Chorus::prepare(float _srate)
{
	srate = _srate;
	delay_samples = 7.f * srate / 1000.f;
	jitter_samples = 1.5f * srate / 1000.f;
	for (int v = 0; v < MAX_VOICES; ++v) {
		delay_l[v].resize((int)(srate * 0.1f)); // 100 ms delay, more than enough for delay time 7ms + depth 100% + jitter
		delay_r[v].resize((int)(srate * 0.1f));
	}
	rate = rateParam->load();
	mix = mixParam->load();
	depth = depthParam->load();
	auto lpfreq_raw = highcutParam->load();
	lpfreq = 1.f - std::exp(-juce::MathConstants<float>::twoPi * lpfreq_raw / srate);
	auto hpfreq_raw = lowcutParam->load();
	hpfreq = 1.f - exp(-juce::MathConstants<float>::twoPi * hpfreq_raw / srate);
	feedback = feedbackParam->load();
}

void Chorus::processBlock(float* left, float* right, int nsamps, int /*blockoffset*/, bool /*audioRate*/)
{
	auto newvoices = (int)voicesParam->load();
	newvoices = newvoices == 0 ? 2 : newvoices * 4;

	if (voices != newvoices) {
		voices = newvoices;
		for (int i = 0; i < voices; i++) {
			delay_l[i].clear();
			delay_r[i].clear();
			auto offset = ((i % 2) ? 1 : -1) * ((i + 1) / (float)voices) * jitter_samples;
			base_delay[i] = std::fmax(0.f, delay_samples + offset);
			phase_offset[i] = i * (1.0f / (float)voices);
			const auto phase_offset_rad = phase_offset[i] * juce::MathConstants<float>::twoPi;
			phase_offset_sin[i] = std::sin(phase_offset_rad);
			phase_offset_cos[i] = std::cos(phase_offset_rad);
		}
	}
	auto sqrtvoices = sqrt((float)voices);

	auto rate_targ = audioProcessor.modulation->getValue(rateModParam, false, nsamps, srate);
	auto rate_step = (rate_targ - rate) / nsamps;
	auto depth_targ = audioProcessor.modulation->getValue(depthModParam, false, nsamps, srate);
	auto depth_step = (depth_targ - depth) / nsamps;
	auto mix_targ = audioProcessor.modulation->getValue(mixModParam, false, nsamps, srate);
	auto mix_step = (mix_targ - mix) / nsamps;

	auto lpfreq_raw = audioProcessor.modulation->getValue(highcutModParam, false, nsamps, srate);
	if (lpfreq_raw >= 20000.f) {
		lpstate_l = 0.f;
		lpstate_r = 0.f;
	}
	auto lpfreq_targ = 1.f - std::exp(-juce::MathConstants<float>::twoPi * lpfreq_raw / srate);
	auto lpfreq_step = (lpfreq_targ - lpfreq) / nsamps;

	auto hpfreq_raw = audioProcessor.modulation->getValue(lowcutModParam, false, nsamps, srate);
	if (hpfreq_raw <= 20.f) {
		hpstate_l = 0.f;
		hpstate_r = 0.f;
	}
	auto hpfreq_targ = 1.f - exp(-juce::MathConstants<float>::twoPi * hpfreq_raw / srate);
	auto hpfreq_step = (hpfreq_targ - hpfreq) / nsamps;

	auto feedback_targ = audioProcessor.modulation->getValue(feedbackModParam, false, nsamps, srate);
	auto feedback_step = (feedback_targ - feedback) / nsamps;

	auto global_phase = phase * juce::MathConstants<float>::twoPi;
	auto global_sin = std::sin(global_phase);
	auto global_cos = std::cos(global_phase);

	for (int i = 0; i < nsamps; ++i) {
		auto wet_l = 0.f;
		auto wet_r = 0.f;

		int v = 0;
        const float modDepth = depth * delay_samples;
		for (; v + 3 < voices; v += 4) {
			auto lfo_l0 = global_sin * phase_offset_cos [ v + 0 ] + global_cos * phase_offset_sin [ v + 0 ];
			auto lfo_l1 = global_sin * phase_offset_cos [ v + 1 ] + global_cos * phase_offset_sin [ v + 1 ];
			auto lfo_l2 = global_sin * phase_offset_cos [ v + 2 ] + global_cos * phase_offset_sin [ v + 2 ];
			auto lfo_l3 = global_sin * phase_offset_cos [ v + 3 ] + global_cos * phase_offset_sin [ v + 3 ];

			auto lfo_r0 = global_cos * phase_offset_cos [ v + 0 ] - global_sin * phase_offset_sin [ v + 0 ];
			auto lfo_r1 = global_cos * phase_offset_cos [ v + 1 ] - global_sin * phase_offset_sin [ v + 1 ];
			auto lfo_r2 = global_cos * phase_offset_cos [ v + 2 ] - global_sin * phase_offset_sin [ v + 2 ];
			auto lfo_r3 = global_cos * phase_offset_cos [ v + 3 ] - global_sin * phase_offset_sin [ v + 3 ];

            auto dl0 = delay_l[ v + 0 ].read(base_delay[ v + 0 ] + 0.5f * (lfo_l0 + 1.0f) * modDepth);
            auto dl1 = delay_l[ v + 1 ].read(base_delay[ v + 1 ] + 0.5f * (lfo_l1 + 1.0f) * modDepth);
            auto dl2 = delay_l[ v + 2 ].read(base_delay[ v + 2 ] + 0.5f * (lfo_l2 + 1.0f) * modDepth);
            auto dl3 = delay_l[ v + 3 ].read(base_delay[ v + 3 ] + 0.5f * (lfo_l3 + 1.0f) * modDepth);

            auto dr0 = delay_r[ v + 0 ].read(base_delay[ v + 0 ] + 0.5f * (lfo_r0 + 1.0f) * modDepth);
            auto dr1 = delay_r[ v + 1 ].read(base_delay[ v + 1 ] + 0.5f * (lfo_r1 + 1.0f) * modDepth);
            auto dr2 = delay_r[ v + 2 ].read(base_delay[ v + 2 ] + 0.5f * (lfo_r2 + 1.0f) * modDepth);
            auto dr3 = delay_r[ v + 3 ].read(base_delay[ v + 3 ] + 0.5f * (lfo_r3 + 1.0f) * modDepth);

			delay_l [ v + 0 ].write ( left [ i ] + dl0 * feedback );
			delay_l [ v + 1 ].write ( left [ i ] + dl1 * feedback );
			delay_l [ v + 2 ].write ( left [ i ] + dl2 * feedback );
			delay_l [ v + 3 ].write ( left [ i ] + dl3 * feedback );

			delay_r [ v + 0 ].write ( right [ i ] + dr0 * feedback );
			delay_r [ v + 1 ].write ( right [ i ] + dr1 * feedback );
			delay_r [ v + 2 ].write ( right [ i ] + dr2 * feedback );
			delay_r [ v + 3 ].write ( right [ i ] + dr3 * feedback );

			wet_l += dl0 + dl1 + dl2 + dl3;
			wet_r += dr0 + dr1 + dr2 + dr3;
		}

		for (; v < voices; v++) {
			auto lfo_l = global_sin * phase_offset_cos [ v ] + global_cos * phase_offset_sin [ v ];
			auto lfo_r = global_cos * phase_offset_cos [ v ] - global_sin * phase_offset_sin [ v ];

            auto dl = delay_l[v].read(base_delay[v] + 0.5f * (lfo_l + 1.0f) * modDepth);
            auto dr = delay_r[v].read(base_delay[v] + 0.5f * (lfo_r + 1.0f) * modDepth);

			delay_l [ v ].write ( left [ i ] + dl * feedback );
			delay_r [ v ].write ( right [ i ] + dr * feedback );

			wet_l += dl;
			wet_r += dr;
		}

		// normalization
		wet_l /= sqrtvoices;
		wet_r /= sqrtvoices;

		// filtering
		if (lpfreq_raw < 20000.f) {
			lpstate_l = lpstate_l + lpfreq * (wet_l - lpstate_l);
			lpstate_r = lpstate_r + lpfreq * (wet_r - lpstate_r);
			wet_l = lpstate_l;
			wet_r = lpstate_r;
		}
		if (hpfreq_raw > 20.f) {
			auto out_l = wet_l - hpstate_l;
			hpstate_l += hpfreq * out_l;
			auto out_r = wet_r - hpstate_r;
			hpstate_r += hpfreq * out_r;
			wet_l = out_l;
			wet_r = out_r;
		}

		auto drymix = Utils::cosHalfPi()(mix);
		auto wetmix = Utils::sinHalfPi()(mix);

		left[i] = left[i] * drymix + wet_l * wetmix;
		right[i] = right[i] * drymix + wet_r * wetmix;

		// increment phase
		const auto delta_radians = juce::MathConstants<float>::twoPi * rate / srate;
		const auto sin_delta = std::sin(delta_radians);
		const auto cos_delta = std::cos(delta_radians);
		const auto next_global_sin = global_sin * cos_delta + global_cos * sin_delta;
		const auto next_global_cos = global_cos * cos_delta - global_sin * sin_delta;
		global_sin = next_global_sin;
		global_cos = next_global_cos;
		phase += rate / srate;
		if (phase >= 1.0f)
			phase -= 1.0f;

		// interpolation
		rate += rate_step;
		depth += depth_step;
		mix += mix_step;
		lpfreq += lpfreq_step;
		hpfreq += hpfreq_step;
		feedback += feedback_step;
	}

	rate = rate_targ;
	depth = depth_targ;
	mix = mix_targ;
	lpfreq = lpfreq_targ;
	hpfreq = hpfreq_targ;
	feedback = feedback_targ;
}
