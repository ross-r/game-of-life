#pragma once

#include <cstdint>
#include <algorithm>

// RGB -> BGR
// LSB ( AAAAAAAA ) ( BBBBBBBB ) ( GGGGGGGG ) ( RRRRRRRR ) MSB
// LSB ( AAAAAAAA ) ( RRRRRRRR ) ( GGGGGGGG ) ( BBBBBBBB ) MSB
#define ARGB_TO_ABGR( value ) ( value & 0xFF000000 ) | ( ( value & 0xFF0000 ) >> 16 ) | ( value & 0x00FF00 ) | ( ( value & 0x0000FF ) << 16 )

class Colour {
	// https://en.wikipedia.org/wiki/HSL_and_HSV
	struct hsl_t {
		float hue, sat, lum;
	};

private:
	union {
		struct {
			uint8_t m_r;
			uint8_t m_g;
			uint8_t m_b;
			uint8_t m_a;
		};

		uint32_t m_abgr;
	};

public:
	Colour() : m_abgr( 0x00000000 ) {}
	Colour( const uint32_t argb ) : m_abgr( ARGB_TO_ABGR( argb ) ) {}
	
	// TODO: Fix these constructors.
	Colour( const uint8_t r, const uint8_t g, const uint8_t b ) : m_r( r ), m_g( g ), m_b( b ), m_a( 255 ) {}
	Colour( const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a ) : Colour( r, g, b ) {
		m_a = a;
	}

	Colour( const Colour& colour ) : m_abgr( colour.m_abgr ) {}
	Colour( const Colour& colour, const uint8_t alpha ) : m_abgr( colour.m_abgr ) {
		m_a = alpha;
	}

public:
	const uint32_t argb() const {
		return m_abgr;
	}

	const uint8_t r() const {
		return m_r;
	}

	const uint8_t g() const {
		return m_g;
	}

	const uint8_t b() const {
		return m_b;
	}

	const uint8_t a() const {
		return m_a;
	}

public:
	const hsl_t to_hsl() const {
		hsl_t hsl{};

		//
		// https://www.niwa.nu/2013/05/math-behind-colorspace-conversions-rgb-hsl/
		//

		const float r = ( float ) m_r / 255.F;
		const float g = ( float ) m_g / 255.F;
		const float b = ( float ) m_b / 255.F;
		
		const float max = std::max( { r, g, b } );
		const float min = std::min( { r, g, b } );

		hsl.lum = 0.5F * ( max + min );
		hsl.sat = hsl.lum > 0.5F ? ( max - min ) / ( 2.F - ( max + min ) ) : ( max - min ) / ( max + min );

		if( min == max ) {
			hsl.hue = 0.F;
		}
		else if( r == max ) {
			hsl.hue = ( g - b ) / ( max - min ) + ( g < b ? 6 : 0 );
		}
		else if( g == max ) {
			hsl.hue = ( ( b - r ) / ( max - min ) ) + 2;
		}
		else {
			hsl.hue = ( ( r - g ) / ( max - min ) ) + 4;
		}

		hsl.hue /= 6;

		return hsl;
	}

	void from_hsl( const hsl_t& hsl ) {
		const auto& t = []( const float p, const float q, float t ) -> float {
			if( t < 0.F ) t += 1.F;
			if( t > 1.F ) t -= 1.F;
			if( t < 1.F / 6.F ) return p + ( q - p ) * 6.F * t;
			if( t < 1.F / 2.F ) return q;
			if( t < 2.F / 3.F ) return p + ( q - p ) * ( 2.F / 3.F - t ) * 6.F;
			return p;
		};

		if( hsl.sat == 0.F ) {
			m_r = m_g = m_b = ( uint8_t ) ( hsl.sat * 255.F );
		}
		else {
			const float q = hsl.lum < 0.5F ? hsl.lum * ( 1 + hsl.sat ) : hsl.lum + hsl.sat - hsl.lum * hsl.sat;
			const float p = 2.F * hsl.lum - q;
			float r = t( p, q, hsl.hue + 1.F / 3.F );
			float g = t( p, q, hsl.hue );
			float b = t( p, q, hsl.hue - 1.F / 3.F );

			m_r = ( uint8_t ) ( r * 255.F );
			m_g = ( uint8_t ) ( g * 255.F );
			m_b = ( uint8_t ) ( b * 255.F );
		}

		m_a = 255;
	}
};