#pragma once
#include <vector>
#include <cmath>

class DelayLine {
public:
	int size = 0;

	void resize ( int newSize )
	{
		if ( newSize <= 0 ) {
			size = 0;
			buf.clear ();
			pos = 0;
			return;
		}

		// quantize to next power of 2 so we can mask the position
		newSize = 1 << ( int ) std::ceil ( std::log2 ( newSize ) );

		size = newSize;
		mask = size - 1;

		if ( newSize > ( int ) buf.size () ) {
			size_t oldBufSize = buf.size ();
			buf.resize ( newSize, 0.0f );
			std::fill ( buf.begin () + oldBufSize, buf.end (), 0.0f );
		}

		pos %= size;
	}

	void clear ()
	{
		std::fill ( buf.begin (), buf.end (), 0.f );
	}

	inline void write ( float s ) noexcept
	{
		buf [ pos ] = s;
		pos = ( pos + 1 ) & mask;
	}

	// assumes delay is less than size
	inline float read ( float delay ) const noexcept
	{
		float readPos = float ( pos ) - delay;

		if ( readPos < 0.0f )
			readPos += float ( size );

		int i0 = int ( readPos );
		const float frac = readPos - float ( i0 );
        i0 &= mask;
        const int i1 = ( i0 + 1 ) & mask;

		const float y0 = buf [ i0 ];
		const float y1 = buf [ i1 ];
		return y0 + frac * ( y1 - y0 );
	}

	// assumes delay is positive and less than size
	inline float read ( int delay ) const noexcept
	{
		int _pos = pos - delay;

		if ( _pos < 0 )
			_pos += size;

		return buf [ _pos ];
	}

	inline float read3 ( float delay ) const noexcept
	{
		float _pos = pos - delay;

		while ( _pos < 0 ) _pos += ( float ) size;
		while ( _pos >= size ) _pos -= ( float ) size;

		int i1 = ( int ) std::floor ( _pos );
		int i0 = ( i1 - 1 + size ) % size;
		int i2 = ( i1 + 1 ) % size;
		int i3 = ( i1 + 2 ) % size;

		float frac = _pos - ( float ) i1;

		float y0 = buf [ i0 ];
		float y1 = buf [ i1 ];
		float y2 = buf [ i2 ];
		float y3 = buf [ i3 ];

		float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
		float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
		float a2 = -0.5f * y0 + 0.5f * y2;
		float a3 = y1;

		return ( ( a0 * frac + a1 ) * frac + a2 ) * frac + a3;
	}

private:
	std::vector<float> buf;
	int pos = 0;
	int mask = 0;
};
