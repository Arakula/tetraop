#pragma once

#include "JuceHeader.h"
#include <vector>
#include <cmath>
#include <functional>
#include <algorithm>
#include "../Globals.h"

using namespace globals;
using SIMDF = mipp::Reg<float>;
using SIMDI = mipp::Reg<int32_t>;
using SIMDFx2 = mipp::Regx2<float>;
using SIMDM = mipp::Msk<4>;

/*
* A copy of LookupTableTransform with additional cubic interpolation
*/
class LookupTable
{
public:
    LookupTable() = default;

    template <typename Func>
    LookupTable(Func fn, float min_, float max_, size_t size_)
    {
        init(fn, min_, max_, size_);
    }

    template <typename Func>
    void init(Func fn, float min_, float max_, size_t size_)
    {
        if (max_ <= min_) throw std::invalid_argument("max must be greater than min");
        if (size_ < 2) throw std::invalid_argument("size must be at least 2");
        min = min_;
        max = max_;
        size = size_;
        values.resize(size);

        scaler = (size > 1) ? (size - 1) / (max - min) : 0.0f;
        offset = -min * scaler;

        for (size_t i = 0; i < size; ++i) {
            float x = static_cast<float>(i) / (size - 1); // Normalized [0, 1]
            float mappedX = min + x * (max - min);
            mappedX = std::clamp(mappedX, min, max);
            values[i] = fn(mappedX);
        }
    }

    inline float operator()(float input) const
    {
        input = std::clamp(input, min, max);
        float normalizedIndex = input * scaler + offset;
        size_t index = static_cast<size_t>(std::floor(normalizedIndex));

        if (index >= size - 1)
            return values.back();

        float frac = normalizedIndex - index;
        return values[index] + frac * (values[index + 1] - values[index]);
    }

    inline float cubic(float input) const
    {
        input = std::clamp(input, min, max);
        float index = input * scaler + offset;
        int i = (int)index;
        float t = index - i;

        int i0 = std::max(0, i - 1);
        int i1 = i;
        int i2 = std::min((int)size - 1, i + 1);
        int i3 = std::min((int)size - 1, i + 2);

        float y0 = values[i0];
        float y1 = values[i1];
        float y2 = values[i2];
        float y3 = values[i3];

        float a0 = y3 - y2 - y0 + y1;
        float a1 = y0 - y1 - a0;
        float a2 = y2 - y0;
        float a3 = y1;

        return (a0 * t * t * t) + (a1 * t * t) + (a2 * t) + a3;
    }

    const std::vector<float>& getValues() const { return values; }
    size_t getSize() const { return size; }
    float getMin() const { return min; }
    float getMax() const { return max; }

private:
    std::vector<float> values;
    float min = 0.0f;
    float max = 1.0f;
    float scaler = 0.0f;
    float offset = 0.0f;
    size_t size = 0;
};

class Utils
{
public:
    inline static float snapToGrid(float norm, int grid)
    {
        return std::round(norm * grid) / grid;
    }

    inline static SIMDF round(SIMDF f)
    {
        auto i = f.trunc();
        auto frac = f - i;
        return mipp::blend(i+1, i, frac.abs() >= 0.5f);
    }

    inline static void wrapPhase(SIMDF& p)
    {
        auto frac = p - p.trunc();
        p = frac.blend(frac + 1.f, p >= 0.f);
    }

    inline static SIMDFx2 panToGain(SIMDF& pan)
    {
        pan = (pan + 1.f) * 0.5f;
        return pan.cossin();
    }

    // expects pan norm 0..1
    inline static SIMDFx2 panToGainCheap(SIMDF& pan)
    {
        auto l = SIMDF(1.f) - pan * pan;
        auto r = -(pan * pan) + pan * 2.f;
        return SIMDFx2(l, r);
    }

    inline static SIMDM laneToMask(int lane)
    {
        bool mask[4] = { 0, 0, 0, 0};
        mask[lane] = 1;
        return SIMDM(mask);
    }

    inline static SIMDF maskToFloat(SIMDM mask)
    {
        return SIMDF(1.f).blend(0.f, mask);
    }

    inline static SIMDM floatToMask(SIMDF mask)
    {
        return mask != 0.f;
    }

    inline static void setMasked(SIMDF& r1, const SIMDF& r2, SIMDM mask)
    {
        r1 = r2.blend(r1, mask);
    }

    inline static bool equal(const SIMDF& r1, const SIMDF& r2)
    {
       SIMDM msk = ((r1 - r2).abs() > 0.000001f);
       return msk.testz();
    }

    inline static bool allLanesZero (const SIMDF& reg)
    {
        return (reg != 0.f).testz();
    };

    inline static bool allLanesPositiveOrZero (const SIMDF& reg)
    {
        return (reg < 0.f).testz();
    };

    inline static bool allLanesNegativeOrZero (const SIMDF& reg)
    {
        return (reg > 0.f).testz();
    };

    inline static float centsToRatio(float cents)
    {
        constexpr float LN2_OVER_1200 = 0.00057762265f; // ln(2)/1200
        return std::exp(cents * LN2_OVER_1200);
    }

    inline static SIMDF centsToRatio(SIMDF& cents)
    {
        constexpr float LN2_OVER_1200 = 0.00057762265f; // ln(2)/1200
        return (cents * LN2_OVER_1200).exp();
    }

    inline static constexpr float LOG_MAX_OVER_MIN_FREQ = 6.907755278982137f; // log(20000 / 20)

    inline static float normalToFreq(float norm)
    {
        return 20.f * std::exp(norm * LOG_MAX_OVER_MIN_FREQ);
    }

    inline static float freqToNormal(float norm)
    {
        return std::log(norm / 20.f) / LOG_MAX_OVER_MIN_FREQ;
    }

    inline static float gainTodB(float gain)
    {
        return gain == 0 ? -60.0f : 20.0f * std::log10(gain);
    }

    static float normalToLog(float min, float max, float norm)
    {
        if (min <= 0.f) min = 1e-5f;
        return min * std::exp(norm * std::log(max / min));
    }

    static float logToNormal(float min, float max, float val)
    {
        if (val <= 0.f) return 0.f;
        if (min <= 0.f) min = 1e-5f;
        auto norm = std::log(val / min) / std::log(max / min);
        return std::fmax(0.0f, std::fmin(1.0f, norm));
    }

    static inline float lerp(float min, float max, float t)
    {
        return min + (max - min) * t;
    }

    static inline SIMDF lerp(SIMDF min, SIMDF max, SIMDF t)
    {
        return min + (max - min) * t;
    }

    static float noSnap(float min, float max, float value)
    {
        (void)min; (void)max;
        return value;
    }

    static bool semverLessOrEqual(const juce::String& v, const juce::String& target)
    {
        auto parse = [](const juce::String& s)
            {
                juce::StringArray parts;
                parts.addTokens(s, ".", "");
                parts.trim();
                parts.removeEmptyStrings();

                std::array<int, 3> result{ 0, 0, 0 };
                for (int i = 0; i < std::min(3, parts.size()); ++i)
                    result[i] = parts[i].getIntValue();
                return result;
            };

        auto a = parse(v);
        auto b = parse(target);

        for (int i = 0; i < 3; ++i) {
            if (a[i] < b[i])  return true;
            if (a[i] > b[i])  return false;
        }
        return true; // equal
    }

    /*
    * Cubic interpolation with wrap around
    */
    static inline float fold(const float* buf, int size, float pos)
    {
        while (pos < 0.0) pos += size;
        while (pos >= size) pos -= size;

        int i = (int)pos;
        float t = pos - i;

        int i0 = (i - 1 + size) % size;
        int i1 = i;
        int i2 = (i + 1) % size;
        int i3 = (i + 2) % size;

        float y0 = buf[i0];
        float y1 = buf[i1];
        float y2 = buf[i2];
        float y3 = buf[i3];

        float a0 = y3 - y2 - y0 + y1;
        float a1 = y0 - y1 - a0;
        float a2 = y2 - y0;
        float a3 = y1;

        return ((a0 * t + a1) * t + a2) * t + a3;
    }

    // LUT used to balance mix without losing amplitude
    static const LookupTable& sinHalfPi()
    {
        static LookupTable table = []()
            {
                LookupTable t;
                t.init([](float norm) {
                    return std::sin(norm * MathConstants<float>::halfPi);
                    }, 0.0f, 1.0f, 256);
                return t;
            }();
        return table;
    }

    // LUT used to balance mix without losing amplitude
    static const LookupTable& cosHalfPi()
    {
        static LookupTable table = []()
            {
                LookupTable t;
                t.init([](float norm) {
                    return std::cos(norm * MathConstants<float>::halfPi);
                    }, 0.0f, 1.0f, 256);
                return t;
            }();
        return table;
    }
};

class Lerp {
    float value, target;
    float step = 0.0f;
    int samplesLeft = 0;
    int duration = 0;
    bool isReset = true;

public:
    Lerp(float start = 0.0) : value(start), target(start) {}

    void setDuration(int duration_) {
        duration = duration_;
    }

    void set(float target_) {
        target = target_;
        if (duration > 0 && !isReset) {
            samplesLeft = duration;
            step = (target - value) / samplesLeft;
        }
        else {
            value = target;
            step = 0.0;
            samplesLeft = 0;
            isReset = false;
        }
    }

    void tick() {
        if (samplesLeft > 0) {
            value += step;
            --samplesLeft;
        }
    }

    void reset() {
        isReset = true;
        value = target;
        samplesLeft = 0;
        step = 0.0;
    }
    inline float get() const { return value; }
    bool isDone() const { return samplesLeft == 0; }
};

class RCFilter
{
public:
    double r = 1.0f;
    double state = 0.0f;
    float output = 0.f;
    float eps = 1e-5f;

    void setup(float resistance, float _srate)
    {
        r = 1.0f / (resistance * _srate + 1);
    }

    float process(float input)
    {
        double targ = (double)input;
        state += r * (targ - state);
        if (std::fabs(targ - state) < eps) // snap
            state = targ;

        output = (float)state;
        return output;
    }

    void reset(float value = 0.0f)
    {
        state = value;
    }
};

class RCFilterBlock
{
public:
    float r = 1.0f;
    float k = 1.f;
    float state = 0.0f;
    float output = 0.0f;
    float srate;
    float resistance = 0.f;
    float eps = 1e-5f;

    void setup(float _resistance, float _srate)
    {
        resistance = _resistance;
        srate = _srate;
        r = 1.0f / (_resistance * srate + 1);
        k = _resistance <= 0.f ? 0.f : -srate * std::log(1.f - r);
    }

    float process(float input, float dt)
    {
        // no smoothing
        if (r >= 1.f) {
            state = input;
        }
        else {
            float alpha = 1.f - std::exp(-k * dt);
            state += alpha * (input - state);
        }

        if (std::abs(state - input) < eps)
            state = input;

        output = state;
        return output;
    }

    void reset(float value = 0.0f)
    {
        output = state = value;
    }
};

class WhiteNoiseGen {
public:
    WhiteNoiseGen() : state(0) {}

    WhiteNoiseGen(uint32_t seed)
        : state(seed)
    {
    }

    float next()
    {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;

        // Convert to float in [-1, 1]
        return (int32_t)x * mult; // float(x)/2^31
    }

    void reseed(uint32_t seed)
    {
        state = seed;
    }

private:
    const float mult = 1.0f / 2147483648.0f;
    uint32_t state;
};

class PinkNoiseGen
{
public:
    PinkNoiseGen() : state(0) {}

    PinkNoiseGen(uint32_t seed)
    {
        reseed(seed);
    }

    float next()
    {
        float white = next_white();

        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;

        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;

        return pink * 0.11f; // normalization
    }

    void reseed(uint32_t seed)
    {
        state = seed ? seed : 1;

        b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.f;
    }

private:
    uint64_t state;

    float b0{}, b1{}, b2{}, b3{}, b4{}, b5{}, b6{};

    float next_white()
    {
        // xorshift64*
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;

        uint32_t r = (state * 2685821657736338717ULL) >> 32;

        return (float)int32_t(r) * (1.0f / 2147483648.0f);
    }
};

class DCBlocker
{
public:
    DCBlocker() {}
    ~DCBlocker() {}

    void setSampleRate(float sampleRate_)
    {
        sampleRate = sampleRate_;
        recalc();
    }

    void setCutoff(float cutoff_ /* Hz */)
    {
        cutoff = cutoff_;
        recalc();
    }

    float process(float x)
    {
        float y = x - z;
        z = x - y * b;
        return y;
    }

    void reset()
    {
        b = 0;
        z = 0;
    }

private:
    float sampleRate = 1;
    float cutoff = 10.0f;

    float b = 0;
    float z = 0;

    void recalc()
    {
        b = std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / sampleRate);
    }
};


class SIMDCBlocker
{
public:
    SIMDCBlocker() {}
    ~SIMDCBlocker() {}

    void setSampleRate(float sampleRate_)
    {
        sampleRate = sampleRate_;
        recalc();
    }

    void setCutoff(SIMDF cutoff_)
    {
        cutoff = cutoff_;
        recalc();
    }

    SIMDF process(SIMDF x)
    {
        SIMDF y = x - z;
        z = x - y * b;
        return y;
    }

    void reset()
    {
        b = 0.f;
        z = 0.f;
    }

private:
    float sampleRate = 1;
    SIMDF cutoff = 10.0f;

    SIMDF b = 0.f;
    SIMDF z = 0.f;

    void recalc()
    {
        b = ((cutoff * -MathConstants<float>::twoPi) / sampleRate).exp();
    }
};