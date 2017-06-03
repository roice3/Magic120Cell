#include <stdafx.h>
#include "helper.h"

STDAPI ColorHLSToRGB(WORD wHue, WORD wLuminance, WORD wSaturation);
STDAPI ColorRGBToHLS(COLORREF clrRGB, WORD * pwHue, WORD * pwLuminance, WORD * pwSaturation);
#pragma comment( lib, "ShLwApi.lib" )

void
CColor::generateRandom()
{
	double hue = getRandomDouble( 1.0 );
	double luminance = .25 + getRandomDouble( .5 );

	// Make most high saturation.
	double saturation = .8 + getRandomDouble( .2 );
	double randomTest = getRandomDouble( 1.0 );
	if( randomTest < .2 )
		saturation = randomTest;

	setColorHLS( hue, luminance, saturation, 1.0 );
}

void
CColor::setColorHLS( double h, double l, double s, double a )
{
	WORD hue = (WORD)( h * 240 );
	WORD luminance = (WORD)( l * 240 );
	WORD saturation = (WORD)( s * 240 );

	// Use HLS palette to change colors to RGB.
	COLORREF color = ColorHLSToRGB( hue, luminance, saturation );
	BYTE r = GetRValue( color );
	BYTE g = GetGValue( color );
	BYTE b = GetBValue( color );

	m_r = (double)r/255.0;
	m_g = (double)g/255.0;
	m_b = (double)b/255.0;
	m_a = (double)a;
}

void 
CColor::lighten() 
{
	WORD r = (WORD)(m_r*255);
	WORD g = (WORD)(m_g*255);
	WORD b = (WORD)(m_b*255);
	COLORREF colorRgb = RGB( r,g,b );
	
	WORD hw, lw, sw;
	ColorRGBToHLS( colorRgb, &hw, &lw, &sw );

	double h = (double)hw / 240;
	double l = (double)lw / 240;
	double s = (double)sw / 240;

	l *= 1.5;
	if( l > 1 )
		l = 1;
	s /= 1.5;
	if( s < 0 )
		s = 0;

	setColorHLS( h, l, s, 1.0 );
}