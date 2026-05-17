// copyright 2026 tilr
// based of nexus reverb
#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cstring>

class TapDelay
{
public:
    static constexpr int MAX_TAPS = 10;
    static constexpr int MAX_SIZE = 200000;

    TapDelay()
    {
        write = 0;
        amount = 0;
        std::memset(buffer, 0, sizeof(buffer));
        std::memset(feedback, 0, sizeof(feedback));
        std::memset(time, 1, sizeof(time));
    }

    void setAmount(int amt) { amount = amt; }

    void setTime(int i, int t) { time[i] = t; }

    void setFeedback(int i, float f) { feedback[i] = f; }

    void process(float& sample)
    {
        float out = sample;

        for (int i = 0; i < amount; i++)
        {
            int r = write - time[i];
            if (r < 0) r += MAX_SIZE;

            out += buffer[r] * feedback[i];
        }

        buffer[write] = sample;

        sample = out;

        if (++write >= MAX_SIZE)
            write = 0;
    }

private:
    float buffer[MAX_SIZE];
    float feedback[MAX_TAPS];
    int   time[MAX_TAPS];
    int   write;
    int   amount;
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

    TapDelay	early1L;
	TapDelay	early1R;
	TapDelay	early2L;
	TapDelay	early2R;

private:
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