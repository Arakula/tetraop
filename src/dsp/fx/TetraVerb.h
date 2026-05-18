// copyright 2026 tilr
#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cstring>
#include "../DelayLine.h"

class AllpassMatrix
{
public:
	enum
	{
		maxBufferSize = 200000,
		MATRIX_SIZE = 8,
	};
private:
	static constexpr float kDecayRange = 0.7f;
	//static constexpr float B = 0.8f;
	//static constexpr float C = -0.85f;
};

class AllpassDelay
{
public:
    void resize(int maxDelaySamples)
    {
        delayLine.resize(maxDelaySamples);
    }

    void setDelay(float samples)
    {
        delay = samples;
    }

    void setFeedback(float value)
    {
        g = value;
    }

    void clear()
    {
        delayLine.clear();
    }

    inline float process(float in) noexcept
    {
        const float z = delayLine.read(delay);

        const float out = z + g * in;

        // standard allpass
        delayLine.write(in - g * out);

        return out;
    }

    inline float processDelay(float in) noexcept
    {
        const float z = delayLine.read(delay);
        const float out = z + g * in;
        delayLine.write(-g * out);

        return out;
    }

private:
    DelayLine delayLine;

    float delay = 1.0f;   // delay in samples
    float g = 0.5f;       // feedback coefficient
};

class TapDelay
{
public:
    static constexpr int MAX_TAPS = 10;

    void resize(int maxDelaySamples)
    {
        delayLine.resize(maxDelaySamples);
    }

    void clear()
    {
        delayLine.clear();
    }

    void setAmount(int amt)
    {
        amount = std::clamp(amt, 0, MAX_TAPS);
    }

    void setTime(int i, float samples)
    {
        time[i] = samples;
    }

    void setFeedback(int i, float value)
    {
        feedback[i] = value;
    }

    inline float process(float in) noexcept
    {
        float out = in;

        for (int i = 0; i < amount; ++i)
        {
            out += delayLine.read(time[i]) * feedback[i];
        }

        delayLine.write(in);

        return out;
    }

private:
    DelayLine delayLine;

    std::array<float, MAX_TAPS> feedback{};
    std::array<float, MAX_TAPS> time{};

    int amount = 0;
};

class TetraVerb
{
	static constexpr int PRIME_LIMIT = 100000;

	static int getNextPrime(float number)
    {
        constexpr int LIMIT = 100000;

        const auto& table = getPrimeTable();
        int idx = std::clamp((int)number, 0, LIMIT);
        return table[idx];
    }

    TapDelay	early1L, early1R,
                early2L, early2R;

    AllpassDelay ap1L, ap1R;
    AllpassDelay ap2AL, ap2AR;
    AllpassDelay ap2BL, ap2BR;

    AllpassDelay ap3AL, ap3AR;
    AllpassDelay ap3BL, ap3BR;

    void prepare(float srate);
    void setSize(float size);


private:
    float srate = 44100.f;
	float size = 0.0f;

	static const std::array<int, PRIME_LIMIT + 1>& getPrimeTable()
    {
        static const auto table = []()
        {
            std::array<int, PRIME_LIMIT + 1> nextPrime{};
            std::vector<bool> prime(PRIME_LIMIT + 1000, true);

            prime[0] = prime[1] = false;

            for (int p = 2; p * p < prime.size(); ++p)
            {
                if (!prime[p]) continue;
                for (int k = p * p; k < prime.size(); k += p)
                    prime[k] = false;
            }

            int next = -1;

            for (int i = (int)prime.size() - 1; i >= 0; --i)
            {
                if (prime[i]) next = i;
                if (i <= PRIME_LIMIT)
                    nextPrime[i] = next;
            }

            return nextPrime;
        }();

        return table;
    }
};