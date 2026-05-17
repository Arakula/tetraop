// copyright 2026 tilr
// based of nexus reverb
#pragma once
#include <array>
#include <vector>
#include <algorithm>

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