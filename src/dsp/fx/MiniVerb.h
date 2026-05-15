// TARONS MINIVERB
#pragma once
#include "FX.h"
#include "../SVF.h"
#include "../../engine/Utils.h"
#include "../../engine/Modulation.h"
#include "../DelayLine.h"

class MiniVerb
	: public FX
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	static constexpr int NUM_ALLPASS = 4;

	struct LP4p {
		float freq = 0.f;
		float g = 0.f;
		float g_targ = 0.f;
		float g_step = 0.f;
		float res = 0.f;
		float s1 = 0.f;
		float s2 = 0.f;
		float s3 = 0.f;
		float s4 = 0.f;

		void init(float _srate, float _freq, float _res = 0.f) {
			if (_freq != freq) {
				freq = _freq;
				g_targ = std::tan(juce::MathConstants<float>::pi * freq / _srate);
			}
			res = _res;
		}

		void prepareBlock(int nsamps) {
			g_step = (g_targ - g) / nsamps;
		}

		inline float process(float in) {
			in -= (res * s4);
			auto G = g / (1.0f + g);
			float v1 = (in - s1) * G; float y1 = v1 + s1; s1 = y1 + v1;
			float v2 = (y1 - s2) * G; float y2 = v2 + s2; s2 = y2 + v2;
			float v3 = (y2 - s3) * G; float y3 = v3 + s3; s3 = y3 + v3;
			float v4 = (y3 - s4) * G; float y4 = v4 + s4; s4 = y4 + v4;

			g += g_step;
			return y4;
		}

		void clear(float val = 0.f) {
			s1 = s2 = s3 = s4 = val;
			g = g_targ;
		}
	};

	struct HP1p {
		float freq = 0.f;
		float g = 0.f;
		float g_targ = 0.f;
		float g_step = 0.f;
		float state = 0.f;

		void init(float _srate, float _freq) {
			if (_freq != freq) {
				freq = _freq;
				g_targ = std::tan(juce::MathConstants<float>::pi * freq / _srate);
			}
		}

		void prepareBlock(int nsamps) {
			g_step = (g_targ - g) / nsamps;
		}

		inline float process(float in) {
			auto ig = g / (1.0f + g);

            float v = (in - state) * ig;
            float ylp = v + state;
            state = ylp + v;

			g += g_step;
            return in - ylp;
		}

		void clear(float val = 0.f) {
			state = val;
			g = g_targ;
		}
	};

	struct Notch {
		float freq = 0.f;
		float b0 = 0.f;
		float b1 = 0.f;
		float b2 = 0.f;
		float a0 = 0.f;
		float a1 = 0.f;
		float a2 = 0.f;
		float gain = 0.25f;
		float z1 = 0.f;
		float z2 = 0.f;
		float y1 = 0.f;
		float y2 = 0.f;

		void init(float _srate, float _freq) {
			if (_freq != freq) {
				freq = _freq;
  				auto q = 2.0f;
				auto omega = juce::MathConstants<float>::twoPi*freq/_srate;
  				auto alpha = sin(omega) / (2.f*q);
				b0 =  1.0f;
  				b1 = -2.0f * std::cos(omega);
  				b2 =  1.0f;
  				a0 =  1.0f + alpha;
  				a1 = -2.0f * std::cos(omega);
  				a2 =  1.0f - alpha;
				b0 /= a0;b1 /= a0;b2/=a0;a1 /= a0;a2 /= a0;
			}
		}

  		inline float process(float in) {
			auto out = b0*in + b1*z1 + b2*z2 - a1*y1 - a2*y2;
  			z2 = z1; z1 = in;
  			y2 = y1; y1 = out;
			return in * (1.0f - gain) + out*gain;
		}

		void clear(float val = 0.f) {
			z1 = z2 = y1 = y2 = val;
		}
	};

	struct AllPass {
		float rawsize = -1.f;
		float srate = 0.f;
		int size = 0;
		float dist = 0.f;
		std::vector<float> buf{};
		RCFilter offsetSmooth{};
		RCFilter attenuationSmooth{};
		float offset = 0.f;
		float attenuation = 1.f;
		float offset_targ = 0.f;
		float attenuation_targ = 0.f;
		float modTimer = 0.f;
		int pos = 0;
		float mod = 0.f;
		float springVel = 0.f;
		float springPos = 0.f;

		HP1p hp{};
		LP4p lp{};
		Notch notch{};

		void init(float _srate, float apdist, float _distance) {
			srate = _srate;
			buf.clear();
			dist = apdist;
			size = (int)(apdist * _distance);
			buf.resize(size, 0.f);
			notch.init(srate, 6000.f);
			offsetSmooth.setup(0.25f, srate);
			attenuationSmooth.setup(0.25f, srate);
		}

		void initFilters(float lowcut, float highcut) {
			lp.init(srate, highcut);
			hp.init(srate, lowcut);
		}

		void prepareBlock(int nsamps) {
			lp.prepareBlock(nsamps);
			hp.prepareBlock(nsamps);
		}

		void setSizeOffsets(float _size) {
			if (rawsize != _size) {
				auto mult = _size * 1.11111f;
				attenuation_targ = std::pow(10.f, -(0.2f * dist * mult) * 0.05f);
				offset_targ = (int)buf.size() * _size;
			}
			rawsize = _size;
		}

		void tick() {
			attenuation = attenuationSmooth.process(attenuation_targ);
			offset = offsetSmooth.process(offset_targ);
		}

		void setSizeEcho(float _size) {
			if (rawsize != size) {
				offset_targ = (int)buf.size() * _size;
			}
			rawsize = _size;
		}

		float springMod(float target, float speed, float damp) {
			auto force = (target - springPos) * speed;
			springVel = (springVel + force) * damp;
			springPos += springVel;
			return springPos;
		}

		inline float allPass(float in, float modDepth, float modSpeed, float feedback) {
			auto modWindow = srate;
			float flipper = (modTimer > modWindow * 0.5f) ? 1.0f : 0.0f;
			modTimer += modSpeed;
			if (modTimer > modWindow)
				modTimer -= modWindow;
			mod = springMod(modDepth * flipper, 1.0f/srate, 0.85f);

			auto fp = pos+mod+offset;
			auto ip = (int)floor(fp);
			auto frc = fp-ip;
			ip = ip % size;

			auto out = (buf[ip] + (buf[(ip+1) % size]-buf[ip]) * frc) - in * feedback;
			out = lp.process(out);
			out = hp.process(out);
			out = notch.process(out);
			buf[pos] = in + out * feedback;
			pos = (pos+1)%size;

			tick();
			return out * attenuation;
		}

		inline float echo(float in, float modDepth, float modSpeed, float feedback, float mix) {
			auto modWindow = srate;
			float flipper = (modTimer > modWindow * 0.5f) ? 1.0f : 0.0f;
			modTimer += modSpeed;
			if (modTimer > modWindow)
				modTimer -= modWindow;
			mod = springMod(modDepth * flipper, 1.0f/srate, 0.85f);

			auto fp = pos+mod+offset;
			auto ip = (int)floor(fp);
			auto frc = fp-ip;
			ip = ip % size;

			auto out = (buf[ip] + (buf[(ip+1)%size] - buf[ip]) * frc);
			out = lp.process(out);
			out = hp.process(out);
			out = notch.process(out);
			buf[pos] = in + out * feedback;
			pos = (pos+1)%size;

			tick();
			return in + (out - in) * mix;
		}

		void clear() {
			hp.clear();
			lp.clear();
			notch.clear();
			std::fill(buf.begin(), buf.end(), 0.f);
		}
	};

	MiniVerb(TetraOPAudioProcessor& p);
	~MiniVerb() override;

	void parameterChanged(const juce::String& paramId, float value) override;
	void prepare(float _srate) override;
	void processBlock(float* left, float* right, int nsamps, int blockoffset, bool audioRate) override;
	void clear() override;

private:
	std::array<AllPass, NUM_ALLPASS> allpassL;
	std::array<AllPass, NUM_ALLPASS> allpassR;
	std::array<AllPass, NUM_ALLPASS> echoL;
	std::array<AllPass, NUM_ALLPASS> echoR;
    DelayLine predelL;
    DelayLine predelR;

	Modulation::Param* sizeParam = nullptr;
	Modulation::Param* predelParam = nullptr;
	Modulation::Param* decayParam = nullptr;
	Modulation::Param* modrateParam = nullptr;
	Modulation::Param* moddepthParam = nullptr;
	Modulation::Param* mixParam = nullptr;
	Modulation::Param* lowcutParam = nullptr;
	Modulation::Param* highcutParam = nullptr;

	float mps = 0.007f; // meters per second
	float distance = 0.5; // distance in meters
	float smear = 0.7f;
	float srate = 44100.f;
};
