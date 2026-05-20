#include "TetraVerb.h"

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

	//allpMx = std::make_unique<AllpassMatrix>();

	//initialize();
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

	static_assert ( std::size ( timesLB ) == AllpassMatrix::MATRIX_SIZE );
	static_assert ( std::size ( timesRB ) == AllpassMatrix::MATRIX_SIZE );

	for ( auto i = 0; i < AllpassMatrix::MATRIX_SIZE; ++i )
	{
		//allpMx->setTimeL ( i, getNextPrime ( timesLB[ i ] * size * srScale ) );
		//allpMx->setTimeR ( i, getNextPrime ( timesRB[ i ] * size * srScale ) );
	}
}

void TetraVerb::processBlock(float* left, float* right, int nsamps)
{
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

		// =========================
		// LATE
		spl0 = ap2AL.process(spl0);
		spl1 = ap2AR.process(spl1);

		spl0 = ap2BL.process(spl0);
		spl1 = ap2BR.process(spl1);

		spl0 = ap3AL.process(spl0);
		spl1 = ap3AR.process(spl1);

		spl0 = ap3BL.process(spl0);
		spl1 = ap3BR.process(spl1);

		//allpMx.process(spl0, spl1);

		left[i] = spl0 + eL;
		right[i] = spl1 + eR;
	}
}