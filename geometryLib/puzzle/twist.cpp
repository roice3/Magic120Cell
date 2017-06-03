#include <stdafx.h>

#include "../matrix4d.h"
#include "twist.h"


namespace
{
	// Calc a rotation matrix to rotate p1 -> p2.
	// Please don't ask me to explain this, I don't understand it.
	// See the paper http://www.geometrictools.com/Documentation/RotationsFromPowerSeries.pdf for more details.
	void calcRotMatrix( const CVector4D & p1, const CVector4D & p2, double angle, CMatrix4D & R )
	{
		// Calc the normalized components of our rotation.
		double a = p1.m_components[0] * p2.m_components[1] - p1.m_components[1] * p2.m_components[0];
		double b = p1.m_components[0] * p2.m_components[2] - p1.m_components[2] * p2.m_components[0];
		double c = p1.m_components[1] * p2.m_components[2] - p1.m_components[2] * p2.m_components[1];
		double d = p1.m_components[0] * p2.m_components[3] - p1.m_components[3] * p2.m_components[0];
		double e = p1.m_components[1] * p2.m_components[3] - p1.m_components[3] * p2.m_components[1];
		double f = p1.m_components[2] * p2.m_components[3] - p1.m_components[3] * p2.m_components[2];
		double mag = sqrt( a*a + b*b + c*c + d*d + e*e + f*f );
		a /= mag;
		b /= mag;
		c /= mag;
		d /= mag;
		e /= mag;
		f /= mag;
		assert( IS_ZERO( a*f - b*e + c*d ) );

		a *= angle;
		b *= angle;
		c *= angle;
		d *= angle;
		e *= angle;
		f *= angle;

		const double STemp[4][4] = 
		{
			{  0,  a,  b,  d },
			{ -a,  0,  c,  e },
			{ -b, -c,  0,  f },
			{ -d, -e, -f,  0 }
		};
		CMatrix4D S;
		S.copyFrom( STemp );

		CMatrix4D I;
		I.setIdentity();
		R = I + S + (S*S)*((1-cos(angle))/(angle*angle)) + (S*S*S)*((angle - sin(angle))/(angle*angle*angle));
	}
}

void 
STwist::getViewRotationMatrix( double angle, CMatrix4D & R )
{
	if( 0 == angle || 0 == m_viewRotationMagnitude )
	{
		R.setIdentity();
		return;
	}

	calcRotMatrix( m_viewRotationP1, m_viewRotationP2, angle, R );
}