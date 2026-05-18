#include "TetraVerb.h"

void TetraVerb::prepare(float _srate)
{
	srate = _srate;
	//void RutaVerb::setSamplerate ( float value )
	//sampleRate = fast::max ( 1.0f, value );
	//shimmer.setSamplerate ( sampleRate );
	//predelay.ensureDistance ();
	//lowCut.ensureDistance ();
	//highCut.ensureDistance ();
	//damp.ensureDistance ();
	//currentSizeType = -1;
}

void TetraVerb::setSize(float _size)
{
	if (_size == size)
		return;

	constexpr float EARLY_A1L = 293;	constexpr float EARLY_B1L = 839;
	constexpr float EARLY_A1R = 383;	constexpr float EARLY_B1R = 1321;
	constexpr float EARLY_A2L = 463;	constexpr float EARLY_B2L = 1871;
	constexpr float EARLY_A2R = 569;	constexpr float EARLY_B2R = 2297;
	constexpr float EARLY_A3L = 743;	constexpr float EARLY_B3L = 3323;
	constexpr float EARLY_A3R = 941;	constexpr float EARLY_B3R = 2729;
	constexpr float EARLY_A4L = 1231;	constexpr float EARLY_B4L = 3881;
	constexpr float EARLY_A4R = 1451;	constexpr float EARLY_B4R = 4721;
	constexpr float EARLY_A5L = 1747;	constexpr float EARLY_B5L = 5443;
	constexpr float EARLY_A5R = 1987;	constexpr float EARLY_B5R = 6173;
	constexpr float EARLY_A6L = 2503;	constexpr float EARLY_B6L = 7159;
	constexpr float EARLY_A6R = 2657;	constexpr float EARLY_B6R = 5039;

	constexpr float AP_1_TIME_L = 263;
	constexpr float AP_1_TIME_R = 373;
	constexpr float AP_2A_TIME_L = 373;
	constexpr float AP_2A_TIME_R = 557;
	constexpr float AP_2B_TIME_L = 659;
	constexpr float AP_2B_TIME_R = 383;
	constexpr float AP_3A_TIME_L = 571;
	constexpr float AP_3A_TIME_R = 307;
	constexpr float AP_3B_TIME_L = 197;
	constexpr float AP_3B_TIME_R = 541;

	size = size * 0.85f + 0.15f;
	const auto srScale = srate / 44100.f;

	early1L.setTime ( 0, getNextPrime ( EARLY_A1L * size * 1.2f  * srScale ) );
	early1L.setTime ( 1, getNextPrime ( EARLY_A2L * size * 1.25f * srScale ) );
	early1L.setTime ( 2, getNextPrime ( EARLY_A3L * size * 1.3f  * srScale ) );
	early1L.setTime ( 3, getNextPrime ( EARLY_A4L * size * 1.35f * srScale ) );
	early1L.setTime ( 4, getNextPrime ( EARLY_A5L * size * 1.4f  * srScale ) );
	early1L.setTime ( 5, getNextPrime ( EARLY_A6L * size * 1.45f * srScale ) );

	early1R.setTime ( 0, getNextPrime ( EARLY_A1R * size * 1.25f * srScale ) );
	early1R.setTime ( 1, getNextPrime ( EARLY_A2R * size * 1.3f  * srScale ) );
	early1R.setTime ( 2, getNextPrime ( EARLY_A3R * size * 1.35f * srScale ) );
	early1R.setTime ( 3, getNextPrime ( EARLY_A4R * size * 1.4f  * srScale ) );
	early1R.setTime ( 4, getNextPrime ( EARLY_A5R * size * 1.45f * srScale ) );
	early1R.setTime ( 5, getNextPrime ( EARLY_A6R * size * 1.5f  * srScale ) );

	early2L.setTime ( 0, getNextPrime ( EARLY_B1L * size * 1.2f  * srScale ) );
	early2L.setTime ( 1, getNextPrime ( EARLY_B2L * size * 1.25f * srScale ) );
	early2L.setTime ( 2, getNextPrime ( EARLY_B3L * size * 1.3f  * srScale ) );
	early2L.setTime ( 3, getNextPrime ( EARLY_B4L * size * 1.35f * srScale ) );
	early2L.setTime ( 4, getNextPrime ( EARLY_B5L * size * 1.4f  * srScale ) );
	early2L.setTime ( 5, getNextPrime ( EARLY_B6L * size * 1.45f * srScale ) );

	early2R.setTime ( 0, getNextPrime ( EARLY_B1R * size * 1.25f * srScale ) );
	early2R.setTime ( 1, getNextPrime ( EARLY_B2R * size * 1.3f  * srScale ) );
	early2R.setTime ( 2, getNextPrime ( EARLY_B3R * size * 1.35f * srScale ) );
	early2R.setTime ( 3, getNextPrime ( EARLY_B4R * size * 1.4f  * srScale ) );
	early2R.setTime ( 4, getNextPrime ( EARLY_B5R * size * 1.45f * srScale ) );
	early2R.setTime ( 5, getNextPrime ( EARLY_B6R * size * 1.5f  * srScale ) );

	ap1L.setDelay ( getNextPrime ( AP_1_TIME_L * size * 1.33f * srScale ) );
	ap1R.setDelay ( getNextPrime ( AP_1_TIME_R * size * 1.35f * srScale ) );

	// AP2 (two cascaded stages)
	ap2AL.setDelay(getNextPrime(AP_2A_TIME_L * size * 1.41f * srScale));
	ap2AR.setDelay(getNextPrime(AP_2A_TIME_R * size * 1.43f * srScale));
	ap2BL.setDelay(getNextPrime(AP_2B_TIME_L * size * 1.11f * srScale));
	ap2BR.setDelay(getNextPrime(AP_2B_TIME_R * size * 1.31f * srScale));

	// AP3 (two cascaded stages)
	ap3AL.setDelay(getNextPrime(AP_3A_TIME_L * size * 1.37f * srScale));
	ap3AR.setDelay(getNextPrime(AP_3A_TIME_R * size * 1.17f * srScale));
	ap3BL.setDelay(getNextPrime(AP_3B_TIME_L * size * 1.21f * srScale));
	ap3BR.setDelay(getNextPrime(AP_3B_TIME_R * size * 1.25f * srScale));

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