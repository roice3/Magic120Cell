#pragma once
#include <string>
#include <algorithm>
#include <limits>

#define _USE_MATH_DEFINES
#include <math.h>

// Radian/Degree conversion.
#define CONSTANT_PI	M_PI
template <class T>
T rad2deg(T r) { return (T)(180. * (r / CONSTANT_PI)); }
template <class U>
U deg2rad(U r) { return (U)(CONSTANT_PI * (r / 180.)); }

// Floating point stuff.
#define TOLERANCE_STRICT	0.0000000000001
#define TOLERANCE_LOOSE		0.0000001
#define TOLERANCE			TOLERANCE_LOOSE		// Figure out a way to make this project configurable?

#define IS_ZERO( value )					( ( ( (value) > -TOLERANCE ) && ( (value) < TOLERANCE ) ) ? TRUE : FALSE )
#define IS_LESS_THAN( value1, value2 )		( value1 < (value2 - TOLERANCE) )
#define IS_GREATER_THAN( value1, value2 )	( value1 > (value2 + TOLERANCE) )
#define IS_EQUAL( value1, value2 )			( !IS_LESS_THAN( value1, value2 ) && !IS_GREATER_THAN( value1, value2 ) )

// Some standard geometry values.
static const double golden = ( 1 + sqrt( 5.0 ) ) / 2;
static const double dihedral = 2 * atan( golden );
static const double shortDist = cos( deg2rad( 36.0 ) );
static const double standardFaceOffset = ( 1 + golden ) / ( 2 * tan( deg2rad( 18.0 ) ) );	// Offset in 4D from origin.
#define LARGE_DISTANCE 10000000

namespace
{

// Get random values between 0 and n.
inline double getRandomDouble( double n ) 
{
	double random = (double)rand() / RAND_MAX;
	random *= n;
	return( random );
}
inline int getRandomInt( int n ) 
{	
	double randomDouble = getRandomDouble( (double)( n + 1 ) );
	
	// Very small chance we were n+1.
	if( randomDouble == (double)( n + 1 ) )
		randomDouble -= 1;

	return( int(randomDouble) );
}

inline double getSmoothedRotation( double rotation, double max )
{
	return (max/2.0) * (-cos( M_PI*rotation/max ) + 1);
}

template< class T >
inline bool even( T in )
{
	return 0 == in % 2;
}

template< class T >
inline bool odd( T in )
{
	return !even( in );
}

}

struct CColor
{
public:

	// Make member variables public for easy access.
	double m_r;
	double m_g;
	double m_b;
	double m_a;

	// Construction.
	CColor();
	CColor( double r, double g, double b, double a );

	// == operator
	bool operator == ( const CColor & rhs ) const
	{
		return( 
			m_r == rhs.m_r &&
			m_g == rhs.m_g &&
			m_b == rhs.m_b &&
			m_a == rhs.m_a );
	}

	void generateRandom();

	// Setup based on HLS scheme.
	void setColorHLS( double h, double l, double s, double a );

	void lighten();
};

inline CColor::CColor( )
{
	// White
	m_r = 1;
	m_g = 1;
	m_b = 1;
	m_a = 1;
}

inline CColor::CColor( double r, double g, double b, double a )
{
	m_r = r;
	m_g = g;
	m_b = b;
	m_a = a;
}

namespace
{

void 
setColorHelper( const CColor & color, float ambient[4], float diffuse[4], float specular[4], float emissive[4] )
{
	float red = (float)color.m_r;
	float green = (float)color.m_g;
	float blue = (float)color.m_b;
	float alpha = (float)color.m_a;

	ambient[0] = red * 0.5f; ambient[1] = green * 0.5f; ambient[2] = blue * 0.5f; ambient[3] = alpha;
	diffuse[0] = red; diffuse[1] = green; diffuse[2] = blue; diffuse[3] = alpha;
	specular[0] = red * 0.8f; specular[1] = green * 0.8f; specular[2] = blue * 0.8f; specular[3] = alpha;	// XXX - play with improving contrast.
	emissive[0] = emissive[1] = emissive[2] = emissive[3] = 0;
}

void 
setupColor( const CColor & color )
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float emissive[4];

	setColorHelper( color, ambient, diffuse, specular, emissive );

	glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, ambient );
	glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse );
	glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, specular );
	glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, emissive );
	glMateriali( GL_FRONT_AND_BACK, GL_SHININESS, 10 );
}

void
setupColorNoLight( const CColor & color )
{
	glColor3d( color.m_r, color.m_g, color.m_b );
}

}