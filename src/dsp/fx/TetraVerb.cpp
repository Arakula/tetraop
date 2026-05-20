#include "TetraVerb.h"

void APMatrix::setTimeL(int index, int time)
{
	timesL[index] = (float)time;
	for (int i = 0; i < MATRIX_SIZE; ++i)
		delL[i].resize(time);
}

void APMatrix::setTimeR(int index, int time)
{
	timesR[index] = (float)time;
	for (int i = 0; i < MATRIX_SIZE; ++i)
		delR[i].resize(time);
}

void APMatrix::setDecay(float decay)
{
	decay = decay * 0.7f;
	gpos = decay;
	gneg = -decay * 0.3f;
}

void APMatrix::setModDepth(float value)
{
	modDepth = value * 30.f;
}

void APMatrix::setModRate(float hertz)
{
	modRate = hertz;
}

void APMatrix::updateModulation(float srate, int nsamps)
{
	constexpr float TWO_PI = 6.283185307179586f;
	const float phaseInc = modRate / srate;

	modPhase += phaseInc * nsamps;
	modPhase -= std::floor(modPhase);

	const float phaseDist = 1.0f / float(MATRIX_SIZE);

	for (int i = 0; i < MATRIX_SIZE; i++)
	{
		const float phaseL = modPhase + phaseDist * float(i);
		const float phaseR = modPhase + phaseDist * float(i + 1);
		const float wrappedL = phaseL - std::floor(phaseL);
		const float wrappedR = phaseR - std::floor(phaseR);
		modL[i] = std::sin(wrappedL * TWO_PI) * modDepth;
		modR[i] = std::sin(wrappedR * TWO_PI) * modDepth;
	}
}

void APMatrix::clear()
{
	for (int i = 0; i < MATRIX_SIZE; ++i)
	{
		delL[i].clear();
		delR[i].clear();
	}
}

void APMatrix::process(float& left, float& right)
{
	const float inL = left;
	const float inR = right;

	SIMDF buf[MATRIX_SIZE];
	for (int i = 0; i < MATRIX_SIZE; ++i)
	{
		buf[i] = SIMDF({
			delL[i].read(timesL[i] + modL[i]),
			delR[i].read(timesR[i] + modR[i])
			, 0.f, 0.f});
	}

	const SIMDF pos(gpos);
	const SIMDF neg(gneg);

	const SIMDF ll = { inL, inL, 0.f, 0.f };
	const SIMDF rr = { inR, inR, 0.f, 0.f };
	const SIMDF lr = { inL, inR, 0.f, 0.f };

	const SIMDF sample = buf[0] + buf[1] + buf[2] + buf[3]
					   + buf[4] + buf[5] + buf[6] + buf[7];

	const SIMDF sum1 = 
		sample * pos + lr;

	const SIMDF sum2 = 
		(buf[0] + buf[2] + buf[4] + buf[6]) * pos + 
		(buf[1] + buf[3] + buf[5] + buf[7]) * neg + lr;

	const SIMDF sum3 =
		(buf[0] + buf[1] + buf[4] + buf[5]) * pos +
		(buf[2] + buf[3] + buf[6] + buf[7]) * neg + ll;

	const SIMDF sum4 =
		(buf[0] + buf[3] + buf[4] + buf[7]) * pos +
		(buf[1] + buf[2] + buf[5] + buf[6]) * neg + rr;

	const SIMDF sum5 =
		(buf[0] + buf[1] + buf[2] + buf[3]) * pos +
		(buf[4] + buf[5] + buf[6] + buf[7]) * neg + lr;

	const SIMDF sum6 =
		(buf[0] + buf[2] + buf[5] + buf[7]) * pos +
		(buf[1] + buf[3] + buf[4] + buf[6]) * neg + lr;

	const SIMDF sum7 =
		(buf[0] + buf[1] + buf[6] + buf[7]) * pos +
		(buf[2] + buf[3] + buf[4] + buf[5]) * neg + rr;

	const SIMDF sum8 =
		(buf[0] + buf[3] + buf[5] + buf[6]) * pos +
		(buf[1] + buf[2] + buf[4] + buf[7]) * neg + ll;

	// damping
	fs1 = sum1 * 1.f + fs1 * 0.f;
	fs2 = sum2 * 1.f + fs2 * 0.f;
	fs3 = sum3 * 1.f + fs3 * 0.f;
	fs4 = sum4 * 1.f + fs4 * 0.f;

	constexpr float fact = 0.5f;

	delL[0].write(fs1.get(0) * fact);
	delL[1].write(fs2.get(1) * fact);
	delL[2].write(fs3.get(0) * fact);
	delL[3].write(fs4.get(1) * fact);
	delL[4].write(sum5.get(0) * fact);
	delL[5].write(sum6.get(1) * fact);
	delL[6].write(sum7.get(0) * fact);
	delL[7].write(sum8.get(1) * fact);

	delR[0].write(fs1.get(1) * fact);
	delR[1].write(fs2.get(0) * fact);
	delR[2].write(fs3.get(1) * fact);
	delR[3].write(fs4.get(0) * fact);
	delR[4].write(sum5.get(1) * fact);
	delR[5].write(sum6.get(0) * fact);
	delR[6].write(sum7.get(1) * fact);
	delR[7].write(sum8.get(0) * fact);

	left = sample.get(0);
	right = sample.get(1);
}

TetraVerb::TetraVerb()
{
	early1L.setAmount(6);
	early1R.setAmount(6);
	early2L.setAmount(6);
	early2R.setAmount(6);

	early1L.setFeedback(0, 0.6f);
	early1L.setFeedback(1, 0.6f);
	early1L.setFeedback(2, 0.55f);
	early1L.setFeedback(3, 0.5f);
	early1L.setFeedback(4, 0.4f);
	early1L.setFeedback(5, 0.4f);
	early1R.setFeedback(0, 0.6f);
	early1R.setFeedback(1, 0.6f);
	early1R.setFeedback(2, 0.55f);
	early1R.setFeedback(3, 0.5f);
	early1R.setFeedback(4, 0.4f);
	early1R.setFeedback(5, 0.4f);

	early2L.setFeedback(0, 0.6f);
	early2L.setFeedback(1, 0.6f);
	early2L.setFeedback(2, 0.55f);
	early2L.setFeedback(3, 0.5f);
	early2L.setFeedback(4, 0.4f);
	early2L.setFeedback(5, 0.4f);
	early2R.setFeedback(0, 0.6f);
	early2R.setFeedback(1, 0.6f);
	early2R.setFeedback(2, 0.55f);
	early2R.setFeedback(3, 0.5f);
	early2R.setFeedback(4, 0.4f);
	early2R.setFeedback(5, 0.4f);
}

void TetraVerb::prepare(float _srate)
{
	srate = _srate;

	predelL.resize((int)std::ceil(srate) + 1);
	predelR.resize((int)std::ceil(srate) + 1);

	//void RutaVerb::setSamplerate ( float value )
	//sampleRate = fast::max ( 1.0f, value );
	//shimmer.setSamplerate ( sampleRate );
	//predelay.ensureDistance ();
	//lowCut.ensureDistance ();
	//highCut.ensureDistance ();
	//damp.ensureDistance ();
	//currentSizeType = -1;
}

void TetraVerb::setPredel(float seconds)
{
	predelSamps = (int)std::ceil(seconds * srate);
}

void TetraVerb::setDecay(float decay)
{
	decay = 1.0f - (decay * decay - 2.0f * decay + 1.0f);
	decay *= 0.99f;
	matrix.setDecay(decay);
}

void TetraVerb::setModulation(float rate, float depth)
{
	matrix.setModRate(rate);
	matrix.setModDepth(depth);
}

void TetraVerb::setSize(float sizeNorm)
{

	sizeNorm = 0.3f + sizeNorm * (3.75f - 0.3f);
	sizeNorm = sizeNorm * 0.85f + 0.15f;
	if (sizeNorm == size)
		return;
	size = sizeNorm;

	constexpr std::array<float, 6> earlyAL = {
		293, 463, 743, 1231, 1747, 2503
	};

	constexpr std::array<float, 6> earlyAR = {
		383, 569, 941, 1451, 1987, 2657
	};

	constexpr std::array<float, 6> earlyBL = {
		839, 1871, 3323, 3881, 5443, 7159
	};

	constexpr std::array<float, 6> earlyBR = {
		1321, 2297, 2729, 4721, 6173, 5039
	};

	constexpr std::array<float, 5> apTimeL = {
		263, 373, 659, 571, 197
	};

	constexpr std::array<float, 5> apTimeR = {
		373, 557, 383, 307, 541
	};

	const auto srScale = srate / 44100.f;

	early1L.setTime ( 0, (float)getNextPrime ( earlyAL[0] * size * 1.2f  * srScale ) );
	early1L.setTime ( 1, (float)getNextPrime ( earlyAL[1] * size * 1.25f * srScale ) );
	early1L.setTime ( 2, (float)getNextPrime ( earlyAL[2] * size * 1.3f  * srScale ) );
	early1L.setTime ( 3, (float)getNextPrime ( earlyAL[3] * size * 1.35f * srScale ) );
	early1L.setTime ( 4, (float)getNextPrime ( earlyAL[4] * size * 1.4f  * srScale ) );
	early1L.setTime ( 5, (float)getNextPrime ( earlyAL[5] * size * 1.45f * srScale ) );

	early1R.setTime ( 0, (float)getNextPrime ( earlyAR[0] * size * 1.25f * srScale ) );
	early1R.setTime ( 1, (float)getNextPrime ( earlyAR[1] * size * 1.3f  * srScale ) );
	early1R.setTime ( 2, (float)getNextPrime ( earlyAR[2] * size * 1.35f * srScale ) );
	early1R.setTime ( 3, (float)getNextPrime ( earlyAR[3] * size * 1.4f  * srScale ) );
	early1R.setTime ( 4, (float)getNextPrime ( earlyAR[4] * size * 1.45f * srScale ) );
	early1R.setTime ( 5, (float)getNextPrime ( earlyAR[5] * size * 1.5f  * srScale ) );

	early2L.setTime ( 0, (float)getNextPrime ( earlyBL[0] * size * 1.2f  * srScale ) );
	early2L.setTime ( 1, (float)getNextPrime ( earlyBL[1] * size * 1.25f * srScale ) );
	early2L.setTime ( 2, (float)getNextPrime ( earlyBL[2] * size * 1.3f  * srScale ) );
	early2L.setTime ( 3, (float)getNextPrime ( earlyBL[3] * size * 1.35f * srScale ) );
	early2L.setTime ( 4, (float)getNextPrime ( earlyBL[4] * size * 1.4f  * srScale ) );
	early2L.setTime ( 5, (float)getNextPrime ( earlyBL[5] * size * 1.45f * srScale ) );

	early2R.setTime ( 0, (float)getNextPrime ( earlyBR[0] * size * 1.25f * srScale ) );
	early2R.setTime ( 1, (float)getNextPrime ( earlyBR[1] * size * 1.3f  * srScale ) );
	early2R.setTime ( 2, (float)getNextPrime ( earlyBR[2] * size * 1.35f * srScale ) );
	early2R.setTime ( 3, (float)getNextPrime ( earlyBR[3] * size * 1.4f  * srScale ) );
	early2R.setTime ( 4, (float)getNextPrime ( earlyBR[4] * size * 1.45f * srScale ) );
	early2R.setTime ( 5, (float)getNextPrime ( earlyBR[5] * size * 1.5f  * srScale ) );

	ap1L.setDelay ((float)getNextPrime ( apTimeL[0] * size * 1.33f * srScale ) );
	ap1R.setDelay ((float)getNextPrime ( apTimeR[0] * size * 1.35f * srScale ) );

	// AP2 (two cascaded stages)
	ap2AL.setDelay((float)getNextPrime(apTimeL[1] * size * 1.41f * srScale));
	ap2AR.setDelay((float)getNextPrime(apTimeR[1] * size * 1.43f * srScale));
	ap2BL.setDelay((float)getNextPrime(apTimeL[2] * size * 1.11f * srScale));
	ap2BR.setDelay((float)getNextPrime(apTimeR[2] * size * 1.31f * srScale));

	// AP3 (two cascaded stages)
	ap3AL.setDelay((float)getNextPrime(apTimeL[3] * size * 1.37f * srScale));
	ap3AR.setDelay((float)getNextPrime(apTimeR[3] * size * 1.17f * srScale));
	ap3BL.setDelay((float)getNextPrime(apTimeL[4] * size * 1.21f * srScale));
	ap3BR.setDelay((float)getNextPrime(apTimeR[4] * size * 1.25f * srScale));

	constexpr int	timesLB[] = { 1543, 9187, 2179, 4211, 3203, 8999, 6047, 7001 };
	constexpr int	timesRB[] = { 2741, 6073, 9643, 7591, 3319, 6163, 7151, 3329 };

	static_assert ( std::size ( timesLB ) == APMatrix::MATRIX_SIZE );
	static_assert ( std::size ( timesRB ) == APMatrix::MATRIX_SIZE );

	for ( auto i = 0; i < APMatrix::MATRIX_SIZE; ++i )
	{
		matrix.setTimeL ( i, getNextPrime ( timesLB[ i ] * size * srScale ) );
		matrix.setTimeR ( i, getNextPrime ( timesRB[ i ] * size * srScale ) );
	}
}

void TetraVerb::processBlock(float* left, float* right, int nsamps)
{
	matrix.updateModulation(srate, nsamps);

	for (int i = 0; i < nsamps; ++i)
	{
		// predelay
		predelL.write(left[i]);
		predelR.write(right[i]);
		float spl0 = predelL.read(predelSamps + 1);
		float spl1 = predelR.read(predelSamps + 1);

		// EARLY
		float eL = spl0;
		float eR = spl1;

		eL = early1L.process(eL);
		eR = early1R.process(eR);

		eL = early2L.process(eL);
		eR = early2R.process(eR);

		eL = ap1L.processDelay(eL);
		eR = ap1R.processDelay(eR);

		// LATE
		spl0 = ap2AL.process(spl0);
		spl1 = ap2AR.process(spl1);

		spl0 = ap2BL.process(spl0);
		spl1 = ap2BR.process(spl1);

		spl0 = ap3AL.process(spl0);
		spl1 = ap3AR.process(spl1);

		spl0 = ap3BL.process(spl0);
		spl1 = ap3BR.process(spl1);

		matrix.process(spl0, spl1);

		left[i] = spl0;
		right[i] = spl1;
	}
}