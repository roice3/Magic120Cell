#include <stdafx.h>
#include "polygon.h"
#include "puzzle/twist.h"


namespace
{
	//
	// These helper methods are all Euclidean functions.
	//

	// Returns the counterclock angle between two vectors (between 0 and 2*pi)
	// NOTE: A unique counter clockwise angle really only makes sense onces you've picked a plane normal direction.
	//		 So as coded, this function is really only intended to be used with 2D vector inputs.
	double angleToCounterClock( const CVector3D & v1, const CVector3D & v2 )
	{
		// This calc could be done more efficiently using atan2
		// instead of dot and cross products.
		// But we may be able to extend the latter method later to 3D
		// (so I'm leaving what I did in, but commented out.
		double angle = atan2( v2.y(), v2.x() ) - atan2( v1.y(), v1.x() );
		if ( angle < 0 )
			return angle + 2*M_PI;
		return angle;
		
		/*
		double result = v1.angleTo( v2 );
		double sign = (v1.cross( v2 )).dot( CVector3D(0,0,1) ) > 0 ? 1 : -1;
		result *= sign;
		if( result < 0 )
			result += 2*M_PI;
		return result;*/
	}

	double angleToClock( const CVector3D & v1, const CVector3D & v2 )
	{
		double result = angleToCounterClock( v1, v2 );
		return( 2*M_PI - result );
	}

	double distancePointLine( const CVector3D & p, const CVector3D & lineP1, const CVector3D & lineP2 )
	{
		// The line vector
		CVector3D v1 = lineP2 - lineP1;
		double lineMag = v1.abs();
		if( IS_ZERO( lineMag ) )
		{
			// Line definition points are the same.
			// Guess we'll fall back on the distance between the two points.
			// XXX - happening with large number of reflections.
			//assert( false );
			return p.distance( lineP1 );
		}

		CVector3D v2 = p - lineP1;
		double distance = ( v1.cross( v2 ) ).abs() / lineMag;
		return distance;
	}

	CVector3D projectOntoLine( const CVector3D & p, const CVector3D & lineP1, const CVector3D & lineP2 )
	{
		CVector3D v1 = lineP2 - lineP1;
		double lineMag = v1.abs();
		if( IS_ZERO( lineMag ) )
		{
			assert( false );
			return CVector3D();
		}
		v1.normalize();

		CVector3D v2 = p - lineP1;
		double distanceAlongLine = v2.dot( v1 );
		return lineP1 + v1 * distanceAlongLine;
	}

	// XXX - Assumes lines are in 2D plane.
	int intersectionLineLine( const CVector3D & p1, const CVector3D & p2, 
		const CVector3D & p3, const CVector3D & p4, CVector3D & intersection ) 
	{
		CVector3D n1 = p2 - p1;
		CVector3D n2 = p4 - p3;

		// Intersect?
		// XXX - Handle the case where lines are one and the same separately? 
		//		 (infinite interesection points)
		if( IS_ZERO( n1.cross( n2 ).abs() ) )
			return 0;

		const double d3 = distancePointLine( p3, p1, p2 );
		const double d4 = distancePointLine( p4, p1, p2 );
		
		// Distances on the same side?
		// This tripped me up.
		const double a3 = angleToClock( p3 - p1, n1 );
		const double a4 = angleToClock( p4 - p1, n1 );
		bool sameSide = a3 > M_PI ? a4 > M_PI : a4 <= M_PI;

		double factor = sameSide ? 
			d3 / ( d3 - d4 ) :
			d3 / ( d3 + d4 );
		intersection = p3 + n2 * factor;

		// XXX - Unfortunately, this is happening sometimes.
		if( !IS_ZERO( distancePointLine( intersection, p1, p2 ) ) )
		{
			//assert( false );
		}

		return 1;
	}

	int intersectionLineCircle( const CVector3D & lineP1, const CVector3D & lineP2, const CCircle & circle, 
		CVector3D & p1, CVector3D & p2 )
	{
		// Distance from the circle center to the closest point on the line.
		double d = distancePointLine( circle.m_center, lineP1, lineP2 );

		// No intersection points.
		const double & r = circle.m_radius;
		if( d > r )
			return 0;

		// One intersection point.
		p1 = projectOntoLine( circle.m_center, lineP1, lineP2 );
		if( IS_EQUAL( d, r ) )
			return 1;

		// Two intersection points.
		// Special case when the line goes through the circle center,
		// because we can see numerical issues otherwise.
		//
		// I had further issues where my default tolerance was too strict for this check.
		// The line was close to going through the center and the second block was used,
		// so I had to loosen the tolerance used by my comparison macros.
		if( IS_ZERO( d ) )
		{
			CVector3D line = lineP2 - lineP1;
			line.normalize();
			line *= r;
			p1 = circle.m_center + line;
			p2 = circle.m_center - line;
		}
		else
		{
			// To origin.
			p1 -= circle.m_center;
			
			p1.normalize();
			p1 *= r;
			p2 = p1;
			double angle = acos( d / r );
			p1.rotate( 0, 1, angle );
			p2.rotate( 0, 1, -angle );
			
			// Back out.
			p1 += circle.m_center;
			p2 += circle.m_center;
		}
		return 2;
	}

	bool pointOnLineSegment( const CVector3D & p, const CSegment & seg )
	{
		// This will be so if the point and the segment ends represent
		// the vertices of a degenerate triangle.
		double d1 = ( seg.m_p2 - seg.m_p1 ).abs();
		double d2 = ( p - seg.m_p1 ).abs();
		double d3 = ( seg.m_p2 - p ).abs();
		return IS_EQUAL( d1, d2 + d3 );
	}

	int intersectionCircleCircle( const CCircle & c1, const CCircle & c2, CVector3D & p1, CVector3D & p2 )
	{
		// A useful page describing the cases in this function is:
		// http://ozviz.wasp.uwa.edu.au/~pbourke/geometry/2circle/

		p1.empty();
		p2.empty();

		// Vector and distance between the centers.
		CVector3D v = c2.m_center - c1.m_center;
		double d = v.abs();
		const double & r1 = c1.m_radius;
		const double & r2 = c2.m_radius;
		
		// Circle centers coincident.
		if( IS_ZERO( d ) )
		{
			if( IS_EQUAL( r1, r2 ) )
				return -1;
			else
				return 0;
		}

		// We should be able to normalize at this point.
		if( !v.normalize() )
		{
			assert( false );
			return 0;
		}

		// No intersection points.
		// First case is disjoint circles.
		// Second case is where one circle contains the other.
		if( IS_GREATER_THAN( d, r1 + r2 ) || 
			IS_LESS_THAN( d, abs( r1 - r2 ) ) )
			return 0;

		// One intersection point.
		if( IS_EQUAL( d, r1 + r2 ) )
		{
			p1 = c1.m_center + v * r1;
			return 1;
		}
		// XXX - unhandled case? IS_EQUAL( d, abs( r1 - r2 ) )

		// There must be two intersection points.
		p1 = p2 = v * r1;
		double temp = ( r1*r1 - r2*r2 + d*d ) / ( 2*d );
		double angle = acos( temp / r1 );
		assert( !IS_ZERO( angle ) && !IS_EQUAL( angle, M_PI ) );
		p1.rotate( 0, 1, angle );
		p2.rotate( 0, 1, -angle );
		p1 += c1.m_center;
		p2 += c1.m_center;
		return 2;
	}

	bool pointOnArcSegment( const CVector3D & p, const CSegment & seg )
	{
		double maxAngle = seg.angle();
		CVector3D v1 = seg.m_p1 - seg.m_center;
		CVector3D v2 = p - seg.m_center;
		assert( IS_EQUAL( v1.abs(), v2.abs() ) );
		double angle = seg.m_clockwise ?
			angleToClock( v1, v2 ) :
			angleToCounterClock( v1, v2 );

		// XXX - need tolerance safe comparison?
		return angle <= maxAngle;
	}
}

CVector3D 
CTransformable::reflectPoint( const CCircle & c, const CVector3D & in ) 
{
	// Handle infinities.
	double infinity = std::numeric_limits<double>::infinity();
	CVector3D infinityVector( infinity, infinity, infinity );
	if( in.compare( c.m_center ) )
		return infinityVector;
	if( in == infinityVector )
		return c.m_center;

	CVector3D v = in - c.m_center;
	double d = v.abs();
	v.normalize();
	return c.m_center + v * ( c.m_radius * c.m_radius / d );
} 

CVector3D 
CTransformable::reflectPoint( const CSegment & s, const CVector3D & in ) 
{
	if( CSegment::ARC == s.m_type )
	{
		CCircle c = s.getCircle();
		return reflectPoint( c, in );
	}
	else
	{
		CVector3D p = projectOntoLine( in, s.m_p1, s.m_p2 );
		return in + ( p - in ) * 2;
	}
}

bool
CCircle::operator == ( const CCircle & rhs ) const
{
	return( 
		m_center.compare( rhs.m_center ) &&
		IS_EQUAL( m_radius, rhs.m_radius ) );
}

bool
CCircle::operator != ( const CCircle & rhs ) const
{
	return !( *this == rhs );
}

bool
CCircle::from3Points( const CVector3D & p1, const CVector3D & p2, const CVector3D & p3 ) 
{
	/* Some links
	http://mathforum.org/library/drmath/view/54323.html
	http://delphiforfun.org/Programs/Math_Topics/circle_from_3_points.htm
	There is lots of info out there about solving via equations,
	but as with other code in this project, I wanted to use geometrical constructions. */

	// Midpoints.
	CVector3D m1 = ( p1 + p2 ) / 2;
	CVector3D m2 = ( p1 + p3 ) / 2;

	// Perpendicular bisectors.
	CVector3D b1 = ( p2 - p1 ) / 2;
	CVector3D b2 = ( p3 - p1 ) / 2;
	b1.normalize();
	b2.normalize();
	b1.rotate( 0, 1, M_PI/2 );
	b2.rotate( 0, 1, M_PI/2 );

	int found = intersectionLineLine( m1, m1 + b1, m2, m2 + b2, m_center );
	if( 0 == found )
	{
		// We don't support 3 colinear points.
		return false;
	}

	m_radius = ( p1 - m_center ).abs();
	assert( IS_EQUAL( m_radius, ( p2 - m_center ).abs() ) );
	assert( IS_EQUAL( m_radius, ( p3 - m_center ).abs() ) );
	return true;
}

bool 
CCircle::isPointInside( const CVector3D & test ) const
{
	return IS_LESS_THAN( (test - m_center).abs(), m_radius );
}

bool 
CCircle::isPointOn( const CVector3D & test ) const
{
	return IS_EQUAL( (test - m_center).abs(), m_radius );
}

void 
CCircle::reflect( const CSegment & s ) 
{
	if( CSegment::ARC == s.m_type )
	{
		CCircle c = s.getCircle();

		// Reflecting to a line?
		// XXX - extend circle class to make lines a more proper special case?
		if( isPointOn( c.m_center ) )
		{
			m_center = CVector3D();
			m_radius = std::numeric_limits<double>::infinity();
		}
		else
		{
			// NOTE: We can't just reflect the center.
			//		 See http://mathworld.wolfram.com/Inversion.html
			const double & a = m_radius;
			const double & k = c.m_radius;
			CVector3D v = m_center - c.m_center;
			double t = k*k / ( v.magSquared() - a*a );
			m_center = c.m_center + v * t;
			m_radius = fabs( t ) * a;
		}
	}
	else
	{
		// We just need to reflect the center.
		m_center = reflectPoint( s, m_center );
	}
}

namespace
{
	int checkOnHelper( int result, const CSegment & segment, CVector3D & p1, CVector3D & p2 )
	{
		int ret = result;
		if( 1 == result || 2 == result )
		{
			bool p1On = segment.pointOn( p1 );
			if( !p1On )
			{
				p1.empty();
				ret--;
			}

			if( 2 == result )
			{
				bool p2On = segment.pointOn( p2 );
				if( !p2On )
				{
					p2.empty();
					ret--;
				}

				// Depending on what happened above, we may need to move p2 into p1.
				if( !p1On && p2On )
				{
					p1 = p2;
					p2.empty();
					assert( 1 == ret );
				}
			}
		}

		return ret;
	}
}

int 
CCircle::getIntersectionPoints( const CSegment & segment, CVector3D & p1, CVector3D & p2 ) const
{
	int result;
	if( CSegment::ARC == segment.m_type )
	{
		CCircle tempCircle = segment.getCircle();
		result = intersectionCircleCircle( tempCircle, *this, p1, p2 );
	}
	else
		result = intersectionLineCircle( segment.m_p1, segment.m_p2, *this, p1, p2 );

	// We need to make sure any found points are on the segment.
	return checkOnHelper( result, segment, p1, p2 );
}

bool 
CCircle::intersects( const CPolygon & poly ) const
{
	for( uint i=0; i<poly.m_segments.size(); i++ )
	{
		CVector3D dummy1, dummy2;
		if( getIntersectionPoints( poly.m_segments[i], dummy1, dummy2 ) > 0 )
			return true;
	}

	return false;
}

int 
CSegment::getIntersectionPoints( const CSegment & segment, CVector3D & p1, CVector3D & p2 ) const
{
	if( CSegment::ARC == m_type &&
		CSegment::ARC == segment.m_type )
	{
		CCircle tempCircle = getCircle();
		int result = tempCircle.getIntersectionPoints( segment, p1, p2 );
		
		// Make sure the points are on us.
		return checkOnHelper( result, *this, p1, p2 );
	}
	else if( 
		CSegment::LINE == m_type &&
		CSegment::LINE == segment.m_type )
	{
		// XXX - intersectionLineLine never returns -1!
		int result = intersectionLineLine( m_p1, m_p2, segment.m_p1, segment.m_p2, p1 );
		
		// The point must be on us and the segment.
		result = checkOnHelper( result, *this, p1, p2 );
		return checkOnHelper( result, segment, p1, p2 );
	}
	else
	{
		const CSegment & arcSeg = CSegment::ARC == m_type ? *this : segment;
		const CSegment & lineSeg = CSegment::ARC == m_type ? segment : *this;
		CCircle tempCircle = arcSeg.getCircle();
		int result = tempCircle.getIntersectionPoints( lineSeg, p1, p2 );

		// Make sure the points are on the arc.
		return checkOnHelper( result, arcSeg, p1, p2 );
	}
}

bool
CSegment::operator == ( const CSegment & rhs ) const
{
	return( 
		m_type == rhs.m_type &&
		m_p1.compare( rhs.m_p1 ) &&
		m_p2.compare( rhs.m_p2 ) &&
		m_center.compare( rhs.m_center ) &&
		m_clockwise == rhs.m_clockwise &&
		m_color == rhs.m_color );
}

bool
CSegment::operator != ( const CSegment & rhs ) const
{
	return !( *this == rhs );
}

double 
CSegment::radius() const
{
	assert( CSegment::ARC == m_type );
	return( ( m_p1 - m_center ).abs() );
}

double 
CSegment::angle() const
{
	if( CSegment::ARC != m_type )
	{
		assert( false );
		return 0;
	}

	CVector3D v1 = m_p1 - m_center;
	CVector3D v2 = m_p2 - m_center;
	return m_clockwise ? 
		angleToClock( v1, v2 ) :
		angleToCounterClock( v1, v2 );
}

CCircle 
CSegment::getCircle() const
{
	assert( CSegment::ARC == m_type );
	CCircle c;
	c.m_center = m_center;
	c.m_radius = radius();
	return c;
}

bool 
CSegment::pointOn( const CVector3D & test ) const 
{
	return CSegment::ARC == m_type ?
		pointOnArcSegment( test, *this ) : 
		pointOnLineSegment( test, *this );
}

CVector3D 
CSegment::midpoint() const
{
	if( CSegment::ARC == m_type )
	{
		double a = angle() / 2;
		CVector3D ret = m_p1 - m_center;
		ret.rotate( 0, 1, m_clockwise ? -a : a );
		ret += m_center;
		return ret;
	}
	else
	{
		return ( m_p1 + m_p2 ) / 2;
	}
}

double 
CSegment::length() const
{
	if( CSegment::ARC == m_type )
	{
		return radius() * angle();
	}
	else
	{
		return m_p1.distance( m_p2 );
	}
}

namespace
{
	void swap( CVector3D & v1, CVector3D & v2 )
	{
		CVector3D t = v1;
		v1 = v2;
		v2 = t;
	}
}

void 
CSegment::reverse() 
{
	swap( m_p1, m_p2 );
	if( CSegment::ARC == m_type )
		m_clockwise = !m_clockwise;
}

std::vector<CVector3D> 
CSegment::subdivide( int numSegments ) const
{
	std::vector<CVector3D> ret;
	if( numSegments < 1 )
	{
		assert( false );
		return ret;
	}

	if( m_type == CSegment::ARC )
	{
		CVector3D v = m_p1 - m_center;
		const double angle = this->angle() / numSegments;
		for( int i=0; i<numSegments; i++ )
		{
			ret.push_back( m_center + v );
			v.rotate( 0, 1, m_clockwise ? -angle : angle );
		}
	}
	else
	{
		CVector3D v = m_p2 - m_p1;
		v.normalize();
		for( int i=0; i<numSegments; i++ )
			ret.push_back( m_p1 + v * i * length() / numSegments );
	}

	// Add in the last point and return.
	ret.push_back( m_p2 );
	return ret;
}

// The implementation of this method is quite important to have right!
// XXX - Clean up the organization and make it clear that there are
//		 essentially four cases we are handling here:
//			ARC/ARC
//			ARC/LINE
//			LINE/LINE
//			LINE/ARC
// I could potentially simplify this implementation by coding similar
// to how I ended up doing the CPolygon::twist() method, but if it ain't broke...
void 
CSegment::reflect( const CSegment & s ) 
{
	// We set this at the beginning because some operations 
	// change the type, and so it isn't ok to check m_type later.
	bool isArc = CSegment::ARC == m_type;
	if( isArc )
	{
		// Arc reflected in Arc.
		if( CSegment::ARC == s.m_type )
		{
			CCircle c = getCircle();

			// Are we getting reflected into a line segment?
			if( c.isPointOn( s.m_center ) )
			{
				m_center.empty();
				m_type = CSegment::LINE;
			}
			else
			{
				// We are reflected into another arc.

				// Our orientation gets switched, but only if the
				// inversion circle center is not inside the reflected circle.
				if( !c.isPointInside( s.m_center ) )
					m_clockwise = !m_clockwise;
				
				c.reflect( s );
				m_center = c.m_center;
			}
		}

		// Arc reflected in line.
		else
		{
			m_center = reflectPoint( s, m_center );
			m_clockwise = !m_clockwise;
		}
	}

	// NOTE: This must be done after the arc handling,
	//		 since it affects the results of getCircle()!!
	// NOTE: This handles the case of a line reflected in a line.
	m_p1 = reflectPoint( s, m_p1 );
	m_p2 = reflectPoint( s, m_p2 );

	// Line reflected in arc.
	if( !isArc && CSegment::ARC == s.m_type )
	{
		// Make sure our line didn't go through the center of the inversion circle.
		if( m_p1.valid() && m_p2.valid() && 
			!IS_ZERO( distancePointLine( s.m_center, m_p1, m_p2 ) ) )
		{
			m_type = CSegment::ARC;

			// The 3 points of a circle defining are new arc
			// are the two reflected points and s.m_center.
			CCircle newCircle;
			newCircle.from3Points( m_p1, m_p2, s.m_center );
			m_center = newCircle.m_center;

			double angle = angleToClock( m_p1 - m_center, m_p2 - m_center );
			m_clockwise = angle <= M_PI;
		}
	}
}

namespace
{
	CVector3D 
	getPlaneNormal( const CVector3D & p1, const CVector3D & p2, const CVector3D & o ) 
	{
		// Calculate the normal based on the pentagonal plane.
		CVector3D v1 = p1 - o;
		CVector3D v2 = p2 - o;
		CVector3D planeNormal = v2.cross( v1 );
		planeNormal.normalize();
		return planeNormal;
	}

	void 
	vertex( const CVector3D & p )
	{
		glVertex3d( 
			(GLdouble)p.x(), 
			(GLdouble)p.y(), 
			(GLdouble)p.z() );
	}
}

void 
CPolygon::clear() 
{
	m_segments.clear();
}

bool 
CPolygon::valid() const
{
	// We'll return false if any of the segments 
	// have invalid points (NAN, infinity, etc.).
	for( uint i=0; i<numSides(); i++ )
	{
		if( !m_segments[i].valid() )
			return false;
	}

	// Our center can also make us invalid.
	if( isInfinite( m_center ) )
		return false;

	return true;
}

bool 
CSegment::valid() const
{
	// NOTE!
	// We purposefully don't do an isInfinite() check on m_center,
	// since large circles are expected, and this was causing problems.
	if( isInfinite( m_p1 ) ||
		isInfinite( m_p2 ) ||
		( CSegment::ARC == m_type && !m_center.valid() ) )
		return false;

	return true;
}

namespace
{
	// This code from Don Hatch.
	namespace don 
	{
		double
		expm1(double x)
		{
			double u = exp(x);
			if (u == 1.)
				return x;
			if (u-1. == -1.)
				return -1;
			return (u-1.)*x/log(u);
		}

		double
		log1p(double x)
		{
			double u = 1.+x;
			return log(u) - ((u-1.)-x)/u;
		}

		double
		tanh(double x)
		{
			double u = expm1(x);
			return u / (u*(u+2.)+2.) * (u+2.);
		}

		double
		atanh(double x)
		{
			return .5 * log1p(2.*x/(1.-x));
		}

		double
		sinh(double x)
		{
			double u = expm1(x);
			return .5 * u / (u+1) * (u+2);
		}

		double
		asinh(double x)
		{
			return log1p(x * (1. + x / (sqrt(x*x+1.)+1.)));
		}

		double
		cosh(double x)
		{
			double e_x = exp(x);
			return (e_x + 1./e_x) * .5;
		}

		double
		acosh(double x)
		{
			return 2 * log(sqrt((x+1)*.5) + sqrt((x-1)*.5));
		}

		// hyperbolic to euclidean norm (distance from 0,0) in Poincare disk.
		double h2eNorm(double hNorm){
			if( isNaN( hNorm ) )
				return 1.;
			return tanh(.5*hNorm);
		}
	}

	double getNormalizedCircumRadius( Geometry g, int p )
	{
		// Many of the numbers here were picked manually to 
		// to make the amount showing on the screen nice.

		switch( g )
		{
		case Spherical:
			
			switch( p )
			{
			case 2:
				return 3;
			case 3:
				return 2;
			case 4:
				return 1;
			case 5:
				return .8;
			}

		case Euclidean:

			return 2.5;

		case Hyperbolic:
			{
				// Hyperbolic law of cosines
				// http://en.wikipedia.org/wiki/Hyperbolic_law_of_cosines
				//
				// We have a 2,3,p hyperbolic triangle, where the 
				// right angle alpha is opposite the hypotenuse (the length we want).
				double alpha = M_PI/2;
				double beta = M_PI/3;
				double gamma = M_PI/p;

				double hypot = don::acosh( ( cos(alpha) + cos(beta)*cos(gamma) ) / ( sin(beta)*sin(gamma) ) );
				return don::h2eNorm( hypot ) * 12;
			}
		}

		assert( false );
		return 1;
	}

	// Generate a standard polygon centered at the origin.
	void generateStandardPolygon( CPolygon & poly, int nPoints ) 
	{
		poly.clear();
		std::vector<CVector3D> points;

		Geometry g = geometry( nPoints );
		double r = getNormalizedCircumRadius( g, nPoints );

		double angle = 0;
		for( int i=0; i<nPoints; i++ )
		{
			CVector3D p;
			p.set_x( r * cos( angle ) );
			p.set_y( r * sin( angle ) );
			points.push_back( p );
			angle += deg2rad( 360.0 / nPoints );
		}

		// Turn this into segments.
		for( uint i=0; i<points.size(); i++ )
		{
			int idx1 = i;
			int idx2 = i == points.size() - 1 ? 0 : i+1;
			CSegment newSegment;
			newSegment.m_p1 = points[idx1];
			newSegment.m_p2 = points[idx2];

			// Our segments are arcs in Non-Euclidean geometries.
			// Magically, the same formula turned out to work for both.
			// (Maybe this is because the Poincare Disc model of the
			// hyperbolic plane is stereographic projection as well).
			if( g != Euclidean )
			{
				newSegment.m_type = CSegment::ARC;

				if( 2 == nPoints )
				{
					// Our magic formula below breaks down for digons.
					double factor = tan( M_PI / 6 );
					newSegment.m_center = newSegment.m_p1.x() > 0 ?
						CVector3D( 0,-r,0 ) * factor :
						CVector3D( 0, r,0 ) * factor;
				}
				else
				{
					double t1 = M_PI / nPoints;
					double t2 = M_PI / 6 - t1;
					double factor = ( tan( t1 ) / tan( t2 ) + 1 ) / 2;
					newSegment.m_center = ( newSegment.m_p1 + newSegment.m_p2 ) * factor;
				}
				
				newSegment.m_clockwise = Spherical == g ? false : true;
			}

			// XXX - Make this configurable?
			// This is the color of cell boundary lines.
			newSegment.m_color = CColor( 1, 1, 0, 1 );
			poly.m_segments.push_back( newSegment );
		}
	}
}

void 
CPolygon::createRegular( int numSides ) 
{
	generateStandardPolygon( *this, numSides );
}

namespace
{
	static const double finiteScale = 1000;

	// XXX - Make resolution depend on circle size for improved drawing?
	//		 Or maybe make this a user option.
	static const double arcResolution = deg2rad( 4.5 );

	static const double lodScale = 0.5;
	double getLodFactor( int lod )
	{
		switch( lod )
		{
		case 0:
		case 1:
			return 1;
		case 2:
			return 1.5;
		default:
			return 3;
		}
	}
}

void 
CPolygon::drawEdgePoints( int lod ) const
{
	const double lodFactor = getLodFactor( lod );
	for( uint i=0; i<numSides(); i++ )
	{
		const CSegment & s = m_segments[i];
		
		// Check to see if we don't want to draw this segment.
		if( 0 == s.m_color.m_a )
		{
			glEnd();
			glBegin( GL_LINE_STRIP );
			continue;
		}
		else
			setupColorNoLight( s.m_color );

		// First point.
		assert( ! (isInfinite( s.m_p1 ) && isInfinite( s.m_p2 )) );
		CVector3D p1 = isInfinite( s.m_p1 ) ? s.m_p2 * finiteScale : s.m_p1;
		vertex( p1 );

		// For arcs, add in a bunch of extra points.
		if( CSegment::ARC == s.m_type )
		{
			double maxAngle = s.angle();
			CVector3D vs = s.m_p1 - s.m_center;
			int numSegments = (int)( maxAngle / ( arcResolution * lodFactor ) );
			const double angle = maxAngle / numSegments;
			for( int i=1; i<numSegments; i++ )
			{
				vs.rotate( 0, 1, s.m_clockwise ? -angle : angle );	
				vertex( vs + s.m_center );
			}
		}

		// Last point.
		CVector3D p2 = isInfinite( s.m_p2 ) ? s.m_p1 * finiteScale : s.m_p2;
		vertex( p2 );
	}
}

void
CPolygon::drawFillPoints( int lod, bool addInfiniteEnds ) const
{
	const double lodFactor = getLodFactor( lod );
	for( uint i=0; i<numSides(); i++ )
	{
		const CSegment & s = m_segments[i];

		if( !s.valid() )
			continue;

		if( addInfiniteEnds )
		{
			glBegin( GL_QUAD_STRIP );
			vertex( s.m_p1 * finiteScale );
		}
		vertex( s.m_p1 );

		// For arcs, add in a bunch of extra points.
		if( CSegment::ARC == s.m_type )
		{
			double maxAngle = s.angle();
			CVector3D vs = s.m_p1 - s.m_center;
			int numSegments = (int)( maxAngle / ( arcResolution * lodFactor ) );
			const double angle = maxAngle / numSegments;
			for( int i=1; i<numSegments; i++ )
			{
				vs.rotate( 0, 1, s.m_clockwise ? -angle : angle );
				if( addInfiniteEnds )
					vertex( (vs + s.m_center) * finiteScale );
				vertex( vs + s.m_center );
			}
		}

		if( addInfiniteEnds )
		{
			vertex( s.m_p2 * finiteScale );
			vertex( s.m_p2 );
			glEnd();
		}
	}

	if( !addInfiniteEnds && numSides() > 0 )
	{
		const CSegment & s = m_segments[0];
		vertex( s.m_p1 );
	}
}

bool 
CPolygon::renderDl( const CColor & c ) const
{
	setupColorNoLight( c );	// Comment out this line for a very cool accidental effect!
	return m_dl.call();
}

void 
CPolygon::compileDl( Geometry g, int lod, bool drawEdges )
{
	if( m_dl.isRecorded() )
		return;

	m_dl.start( false );
	render( g, lod, drawEdges );
	m_dl.end();
}

void 
CPolygon::render( Geometry g, int lod, bool drawEdges, const CColor & c ) const
{
	setupColorNoLight( c );
	render( g, lod, drawEdges );
}

void 
CPolygon::render( Geometry g, int lod, bool drawEdges ) const
{
	// Only use LOD for hyperbolic.
	if( g != Hyperbolic )
		lod = 0;

	// Fill
	renderFill( g, lod );

	// Edges
	if( drawEdges )
		renderEdges( lod );
}

void 
CPolygon::renderFill( Geometry g, int lod ) const
{
	// Are we inverted? Spherical projections only since
	// the latter check is expensive.
	if( Spherical == g && ( !valid() || !isPointInside( m_center ) ) )
	{
		drawFillPoints( lod, true );
	}
	else
	{
		// The reason for the triangle fan is to deal with concave polygons.
		glBegin( GL_TRIANGLE_FAN );

			// Performance: only use centroid in spherical geometry cases.
			// NOTE: We used to use m_center as the center of the fan everywhere, 
			//		 but to avoid overlap in spherical cases, it turned out to be much 
			//		 better to use the Euclidean center (centroid) rather than the 
			//		 transformed center.
			if( Spherical == g )
				vertex( centroid() );
			else
				vertex( m_center );
			drawFillPoints( lod, false );

		glEnd();
	}
}

void 
CPolygon::renderEdges( int lod ) const
{
	glBegin( GL_LINE_STRIP );
		drawEdgePoints( lod );
	glEnd();
}

namespace
{
	int getNextSegmentIndex( const CPolygon & walking, int idx )
	{
		int ret = idx+1;
		if( ret == walking.numSides() )
			ret = 0;
		return ret;
	}

	int getPrevSegmentIndex( const CPolygon & walking, int idx )
	{
		int ret = idx-1;
		if( ret < 0 )
			ret = walking.numSides()-1;
		return ret;
	}

	// NOTE: sIdx1 and sIdx2 can be updated by this function!
	void startNewPoly( const CPolygon & walking, int & sIdx1, int & sIdx2, const CSegment & newSeg, CPolygon & poly )
	{
		// Handle the edge case of newSeg going through endpoints of our walking polygon segments.
		// This block was required for even-length puzzles.
		const CSegment & s1temp	= walking.m_segments[ sIdx1 ];
		const CSegment & s2temp	= walking.m_segments[ sIdx2 ];
		if( s1temp.m_p1.compare( newSeg.m_p1 ) )
			sIdx1 = getPrevSegmentIndex( walking, sIdx1 );
		if( s2temp.m_p2.compare( newSeg.m_p2 ) )
			sIdx2 = getNextSegmentIndex( walking, sIdx2 );

		const CSegment & s1	= walking.m_segments[ sIdx1 ];
		const CSegment & s2 = walking.m_segments[ sIdx2 ];

		// This block is dedicated to Percy >^..^<
		poly.clear();
		poly.m_segments.push_back( s1 );
		poly.m_segments[0].m_p2 = newSeg.m_p1;
		poly.m_segments.push_back( newSeg );
		poly.m_segments.push_back( s2 );
		poly.m_segments[2].m_p1 = newSeg.m_p2;
	}

	// XXX - This is an invitation for an infinite loop.
	void walkSegments( const CPolygon & walking, uint startSegment, const CVector3D & finishPoint, CPolygon & poly )
	{
		uint iSeg = startSegment;
		while( !walking.m_segments[iSeg].m_p2.compare( finishPoint ) )
		{
			iSeg = getNextSegmentIndex( walking, iSeg );
			poly.m_segments.push_back( walking.m_segments[iSeg] );
		}
	}

	bool isDuplicate( const std::vector<CVector3D> & points, const CVector3D & p )
	{
		for( uint i=0; i<points.size(); i++ )
		{
			if( points[i].compare( p ) )
				return true;
		}

		return false;
	}

	void addIfNotDuplicate( std::vector<CVector3D> & iPoints, std::vector<uint> & sIndex,
		const CVector3D & p, uint i )
	{
		if( !isDuplicate( iPoints, p ) )
		{
			iPoints.push_back( p );
			sIndex.push_back( i );
		}
	}
}

bool 
CPolygon::slice( const CCircle & c, std::vector<CPolygon> & output ) const
{
	// We must be a digon at a minimum.
	if( numSides() < 2 )
		return false;

	// Cycle through our segments and get all 
	// the boundary intersection points.
	std::vector<CVector3D> iPoints;
	std::vector<uint> sIndex;
	for( uint i=0; i<numSides(); i++ )
	{
		const CSegment & s = m_segments[i];
		CVector3D p1, p2;
		int num = c.getIntersectionPoints( s, p1, p2 );
		switch( num )
		{
		case -1:
			assert( false );
			return false;
		case 0:
			break;
		case 1:
			// NOTE: We avoid duplicates in the array.
			//		 This can happen if the circle went through segment endpoints.
			addIfNotDuplicate( iPoints, sIndex, p1, i );
			break;
		case 2:
			addIfNotDuplicate( iPoints, sIndex, p1, i );
			addIfNotDuplicate( iPoints, sIndex, p2, i );
			break;
		default:
			assert( false );
			return false;
		}
	}

	// Are we done? (no intersections)
	if( !iPoints.size() )
	{
		output.push_back( *this );
		return true;
	}

	// Tangent or through a vertex?
	// XXX - We really need to verify the circle is outside the polygon here,
	//		 which is tricky.  NOTE: this->isPointInside( c.m_center ) isn't enough.
	if( 1 == iPoints.size() )
	{
		output.push_back( *this );
		return true;
	}

	// XXX - We only deal with an even number of intersection points,
	//		 that is, we can't have any intersection points that don't 
	//		 cross the boundary (tangencies to segments and slices through
	//		 polygon vertices may both lead to this).
	if( odd( iPoints.size() ) )
	{
		assert( false );
		return false;
	}

	// XXX - To be even more restrictive starting off,
	//		 We only support slicing a polygon in two.
	//		 This should handle everything I'm trying to do for now.
	//		 What can I say, I'm lazy.
	// XXX - On the digon puzzle, this restriction limits us to a max of 5 per side.
	if( 2 != iPoints.size() )
	{
		assert( false );
		return false;
	}

	// XXX - Code assumes well-formed polygon: closed (has connected segments), 
	//		 no repeated vertices.  Assert all this?
	// Code also assumes CCW orientation.
	assert( this->orientation() );

	// Some shortcuts.
	int sIdx1 = sIndex[0];
	int sIdx2 = sIndex[1];

	// Did we start inside or outside the circle?
	const CSegment & firstSeg = m_segments[0];
	bool startedIn = c.isPointInside( firstSeg.m_p1 );

	// XXX - still seeing some problems on simplex when starting point
	//		 is on the circle.  This code helped length-6 case, 
	//		 but broke Megaminx :(
	//if( c.isPointOn( firstSeg.m_p1 ) )
	//	startedIn = true;

	// New segment to splice in.
	CSegment newSeg;
	newSeg.m_type = CSegment::ARC;
	newSeg.m_center = c.m_center;
	newSeg.m_p1 = iPoints[0];
	newSeg.m_p2 = iPoints[1];
	newSeg.m_clockwise = !startedIn;

	CPolygon poly;
	if( sIdx1 == sIdx2 )
	{
		// Ensure the intersection points are ordered on the segment.
		const CSegment & s = m_segments[sIdx1];
		if( CSegment::ARC == s.m_type )
		{
			CVector3D t1 = s.m_p1 - s.m_center;
			CVector3D t2 = iPoints[0] - s.m_center;
			CVector3D t3 = iPoints[1] - s.m_center;
			double a1 = angleToCounterClock( t2, t1 );
			double a2 = angleToCounterClock( t3, t1 );
			if( (!s.m_clockwise && a1 > a2) || (s.m_clockwise && a1 < a2) )
				swap( newSeg.m_p1, newSeg.m_p2 );
		}
		else
		{
			double d1 = (iPoints[0] - s.m_p1).magSquared();
			double d2 = (iPoints[1] - s.m_p1).magSquared();
			if( d1 > d2 )
				swap( newSeg.m_p1, newSeg.m_p2 );
		}

		newSeg.m_clockwise = false;
		CSegment copy = s;
		copy.m_p1 = newSeg.m_p2;
		copy.m_p2 = newSeg.m_p1;
		copy.m_clockwise = false;

		poly.m_segments.push_back( newSeg );
		poly.m_segments.push_back( copy );
		output.push_back( poly );

		newSeg.reverse();
		startNewPoly( *this, sIdx2, sIdx1, newSeg, poly );
		walkSegments( *this, sIdx1, m_segments[sIdx2].m_p1, poly );
		output.push_back( poly );
	}
	else
	{
		startNewPoly( *this, sIdx1, sIdx2, newSeg, poly );
		walkSegments( *this, sIdx2, m_segments[sIdx1].m_p1, poly );
		output.push_back( poly );

		newSeg.reverse();
		startNewPoly( *this, sIdx2, sIdx1, newSeg, poly );
		walkSegments( *this, sIdx1, m_segments[sIdx2].m_p1, poly );
		output.push_back( poly );
	}

	return true;
}

void 
CPolygon::reflect( const CSegment & s ) 
{
	// Just reflect all our segments.
	for( uint i=0; i<m_segments.size(); i++ )
		m_segments[i].reflect( s );
	m_center = reflectPoint( s, m_center );
}

#include <complex>
using namespace std;

namespace
{
	// XXX - Move these to vectorND.h?
	complex<double> vectorToComplex( const CVector3D & in )
	{
		return std::complex<double>( in.x(), in.y() );
	}

	CVector3D complexToVector( const complex<double> & in )
	{
		return CVector3D( in.real(), in.imag(), 0 );
	}

	bool valid( const complex<double> & in )
	{
		return( !isNaN( in.real() ) && !isNaN( in.imag() ) );
	}

	complex<double> elliptic( complex<double> z, complex<double> fixedPlus, complex<double> fixedNeg, double rotation )
	{
		static const complex<double> one( 1, 0 );
		static const complex<double> i( 0, 1 );

		complex<double> ret = z;

		// Since ad - bc == 0 below, we use some trickery to deal with this.
		if( !valid( z ) )
		{
			complex<double> inv = ( fixedPlus + fixedNeg ) / complex<double>( 2, 0 );
			if( IS_EQUAL( rotation, M_PI ) )
				return inv;

			ret = inv;
			rotation += M_PI;
		}

		// Transform to origin.
		bool originTransform1 = valid( fixedNeg ) && valid( fixedPlus );
		bool originTransform2 = valid( fixedPlus ) && !valid( fixedNeg );
		complex<double> a = one;
		complex<double> b = -fixedPlus;
		complex<double> c = one;
		complex<double> d = -fixedNeg;
		if( originTransform1 )
		{
			ret = ( a*ret + b ) / ( c*ret + d );
		}
		if( originTransform2 )
		{
			ret += b;
		}

		// Twist.
		ret *= exp( i * rotation );

		// Transform back.
		if( originTransform1 )
		{
			// This is the inverse of the above function.
			// See http://en.wikipedia.org/wiki/Möbius_transformation
			ret = ( d*ret - b ) / ( -c*ret + a );
		}
		if( originTransform2 )
		{
			ret -= b;
		}
		
		return ret;
	}

	void rotateHelper( CVector3D & z, complex<double> fixedPlus, complex<double> fixedNeg, double rotation )
	{
		std::complex<double> out = elliptic( vectorToComplex( z ), fixedPlus, fixedNeg, rotation );
		z = complexToVector( out );
	}
}

void 
CPolygon::twist( double rotation, const CVector3D & fixedPlus, const CVector3D & fixedNeg ) 
{
	const std::complex<double> f1 = vectorToComplex( fixedPlus );
	const std::complex<double> f2 = vectorToComplex( fixedNeg );

	std::complex<double> out;
	for( uint i=0; i<numSides(); i++ )
	{
		CSegment & s = m_segments[i];

		// NOTES:
		// Arcs can go to lines, and lines to arcs.
		// Rotations may reverse arc directions as well.
		// Arc centers can't be transformed directly.

		// NOTE: We must calc this before altering the endpoints.
		CVector3D mid = s.midpoint();
		if( isInfinite( mid ) )
			mid = isInfinite( s.m_p1 ) ? s.m_p2 * finiteScale : s.m_p1 * finiteScale;

		rotateHelper( s.m_p1, f1, f2, rotation );
		rotateHelper( s.m_p2, f1, f2, rotation );
		rotateHelper( mid, f1, f2, rotation );

		// Can we make a circle out of the rotated points?
		CCircle temp;
		if( !isInfinite( s.m_p1 ) && !isInfinite( s.m_p2 ) && !isInfinite( mid ) && 
			temp.from3Points( s.m_p1, mid, s.m_p2 ) )
		{
			s.m_type = CSegment::ARC;
			s.m_center = temp.m_center;

			// Work out the orientation of the arc.
			CVector3D t1 = s.m_p1 - s.m_center;
			CVector3D t2 = mid - s.m_center;
			CVector3D t3 = s.m_p2 - s.m_center;
			double a1 = angleToCounterClock( t2, t1 );
			double a2 = angleToCounterClock( t3, t1 );
			s.m_clockwise = a2 > a1;
		}
		else
		{
			// The circle construction fails if the points
			// are colinear (if the arc has been transformed into a line).
			s.m_type = CSegment::LINE;

			// XXX - need to do something about this.
			// Turn into 2 segments?
			//if( isInfinite( mid ) )
			// Actually the check should just be whether mid is between p1 and p2.
		}
	}

	rotateHelper( m_center, f1, f2, rotation );
}

void 
CPolygon::twistCenter( double rotation, const CVector3D & fixedPlus, const CVector3D & fixedNeg ) 
{
	const std::complex<double> f1 = vectorToComplex( fixedPlus );
	const std::complex<double> f2 = vectorToComplex( fixedNeg );
	rotateHelper( m_center, f1, f2, rotation );
}

CVector3D 
CPolygon::centroid() const
{
	CVector3D average;
	for( uint i=0; i<numSides(); i++ )
	{
		// NOTE: This is not fully accurate for arcs (using midpoint instead of true centroid).
		//		 This was done on purpose to help avoid drawing overlaps.
		//		 (it biases the calculated centroid towards large arcs.)
		const CSegment & s = m_segments[i];
		if( s.valid() )
			average += s.midpoint() * s.length();
	}

	average /= length();
	return average;
}

double 
CPolygon::length() const
{
	double totalLength = 0;
	for( uint i=0; i<numSides(); i++ )
	{
		const CSegment & s = m_segments[i];
		if( s.valid() )
			totalLength += s.length();	
	}

	return totalLength;
}

bool 
CPolygon::orientation() const
{
	// Special case digons.
	if( 2 == numSides() )
	{
		assert( CSegment::ARC == m_segments[0].m_type );
		return !m_segments[0].m_clockwise;
	}

	// Calculate the signed area.
	// XXX - make that a method?
	// XXX - Except for digon special casing, I'm ignoring complication from arcs at this point,
	//		 though I don't think we have any weirdly behaving polygons that would throw this off.
	double sArea = 0;
	for( uint i=0; i<numSides(); i++ )
	{
		const CSegment & s = m_segments[i];
		sArea += s.m_p1.x() * s.m_p2.y() - s.m_p1.y() * s.m_p2.x();
	}
	sArea /= 2;

	return sArea > 0;
}

bool 
CPolygon::isPointInside( const CVector3D & p ) const
{
	// We use the ray casting since that will work for arcs as well.
	// NOTE: This impl is known to not be fully general yet,
	//		 since some issues won't arise in MagicTile.
	//		 See http://en.wikipedia.org/wiki/Point_in_polygon about
	//		 some of the degenerate cases that are possible.

	if( IS_ZERO( p.magSquared() ) )
		return true;

	CSegment ray;
	ray.m_p1 = p;
	ray.m_p2 = p * 10000;
	ray.m_p2.x() += 500; // HACK! (Avoiding issues on Rubik's Cube and Megaminx where ray goes through vertices).
	ray.m_p2.y() -= 500;

	// Cycle through our segments and get all 
	// the boundary intersection points.
	std::vector<CVector3D> iPoints;
	std::vector<uint> sIndex;
	for( uint i=0; i<numSides(); i++ )
	{
		const CSegment & s = m_segments[i];
		CVector3D p1, p2;
		int num = ray.getIntersectionPoints( s, p1, p2 );
		switch( num )
		{
		case -1:
			assert( false );
			return false;
		case 0:
			break;
		case 1:
			// NOTE: We avoid duplicates in the array.
			//		 This can happen if the circle went through segment endpoints.
			addIfNotDuplicate( iPoints, sIndex, p1, i );
			break;
		case 2:
			addIfNotDuplicate( iPoints, sIndex, p1, i );
			addIfNotDuplicate( iPoints, sIndex, p2, i );
			break;
		default:
			assert( false );
			return false;
		}
	}

	// Even number of intersection points means we're outside, odd means inside
	const bool inside = odd( iPoints.size() );
	return inside;
}

bool CTwistData::twistSticker( Geometry g, const CPolygon & s ) const
{
	uint numSlices = (uint)(twisters.size() + oppTwisters.size() + 1);
	if( this->reduceSliceCount )
		numSlices--;

	// Find out which slice we are on by working from 
	// the furthest slices inward.
	int slice = 0;
	for( uint i=0; i<numSlices; i++ )
	{
		bool inSlice = false;
		if( i == numSlices-1 )
		{
			// This means we've searched everything else.
			if( Spherical == g )
				inSlice = true;
			else
			{
				// We don't want to twist everything else in flat/hyperbolic cases.
				// The behavior doesn't make sense considering repeating units of cells.
				return false;
			}
		}
		else
		{
			CCircle twistingCircle;
			bool tOutside = false;
			if( i<twisters.size() )
			{
				twistingCircle = twisters[i];
				tOutside = twistOutside[i];
			}
			else if( oppTwisters.size() )
			{
				uint oppIndex = i - twisters.size();
				oppIndex = (uint)( oppTwisters.size() - 1 ) - oppIndex;
				if( this->reduceSliceCount )
					oppIndex--;
				twistingCircle = oppTwisters[oppIndex];
				tOutside = oppTwistOutside[oppIndex];
			}
			else
			{
				assert( false );
			}

			// Is the twisting circle a line?
			if( isNaN( twistingCircle.m_radius ) )
			{
				double d1 = ( s.m_center - fixedPlus ).abs();
				double d2 = ( s.m_center - fixedNeg ).abs();
				inSlice = d1 < d2;
			}
			else
			{
				inSlice = tOutside ? 
					!twistingCircle.isPointInside( s.m_center ) :
					twistingCircle.isPointInside( s.m_center );
			}
		}

		if( inSlice )
		{
			slice = i+1;
			break;
		}
	}

	// Now check the slicemask.
	if( sliceToMask( slice ) & slicemask )
		return true;

	return false;
}

void 
CCell::preCalcTwistingSpherical( const CTwistDataArray & twistDataArray ) 
{
	for( uint i=0; i<m_stickers.size(); i++ )
	{
		CPolygon & s = m_stickers[i];
		s.m_twistIdx = twistDataArray.twistSticker( Spherical, s );
	}
}

bool 
CCell::preCalcTwisting( Geometry g, const CTwistDataArray & twistDataArray, bool master ) 
{
	// Invalidate us if we are going to get twisted.
	if( twistDataArray.willTwist( this->m_tile ) )
	{
		m_dl.clear(); 
		for( uint i=0; i<m_stickers.size(); i++ )
		{
			CPolygon & s = m_stickers[i];
			int twist = twistDataArray.twistSticker( g, s );
			s.m_twistIdx = twist;
		}

		return true;
	}
	else
	{
		// Nothing will be twisted.
		for( uint i=0; i<m_stickers.size(); i++ )
			m_stickers[i].m_twistIdx = -1;
	}

	return false;
}

void 
CCell::render( Geometry g, bool drawEdges, bool twisting, double rotation, const CTwistDataArray & twistDataArray, const CState & state, int cellIndex )
{
	if( m_dl.call() )
		return;

	// Done rotating?
	// Draw and generate a new display list.
	if( !twisting )
	{
		if( g != Spherical )
		{
			// Compile sticker display lists.
			// NOTE: It appears that to nest display lists, you have to compile them first.?.
			for( uint i=0; i<m_stickers.size(); i++ )
				m_stickers[i].compileDl( g, m_lod, drawEdges );

			m_dl.start();
			for( uint i=0; i<m_stickers.size(); i++ )
			{
				const CColor & c = state.getStickerColor( cellIndex, i );
				m_stickers[i].renderDl( c );
			}
			m_dl.end();
		}
		else
		{
			for( uint i=0; i<m_stickers.size(); i++ )
			{
				const CColor & c = state.getStickerColor( cellIndex, i );
				m_stickers[i].render( g, m_lod, drawEdges, c );
			}
		}
	}
	else
		renderTwisting( g, drawEdges, rotation, twistDataArray, state, cellIndex );
}

void 
CCell::renderTwisting( Geometry g, bool drawEdges, double rotation, 
	const CTwistDataArray & twistDataArray, const CState & state, int cellIndex ) const
{
	// Render in twisting mode.
	// We don't display list anything in this path.
	for( uint i=0; i<m_stickers.size(); i++ )
	{
		const CColor & c = state.getStickerColor( cellIndex, i );
		const CPolygon & s = m_stickers[i];

		// Do we need to twist this one?
		if( -1 == s.m_twistIdx )
		{
			if( g == Spherical )
				s.render( g, m_lod, drawEdges, c );
			else
			{
				if( !s.renderDl( c ) )
					s.render( g, m_lod, drawEdges, c );
			}
		}
		else
		{
			const CTwistData & twistData = twistDataArray.get( s.m_twistIdx );
			CVector3D fixedPlus = twistData.fixedPlus;
			CVector3D fixedNeg = twistData.fixedNeg;
			CPolygon copy = s;
			copy.twist( twistData.m_twistingReversed ? -rotation : rotation, fixedPlus, fixedNeg );
			copy.render( g, m_lod, drawEdges, c );
		}			
	}
}

void 
CCell::renderForPicking( Geometry g ) 
{
	m_tile.renderFill( g, m_lod );
}

void 
CCell::reflect( const CSegment & s ) 
{
	for( uint i=0; i<m_stickers.size(); i++ )
		m_stickers[i].reflect( s );
	for( uint i=0; i<m_twisters.size(); i++ )
		m_twisters[i].reflect( s );
	m_vertexCircle.reflect( s );
	m_tile.reflect( s );
}

namespace
{
	void bringIntoRange( int & val, int range )
	{
		if( val >= range )
			val -= range;
	}
}

CSegment 
CCell::reflectBySwappingSegments( const CPolygon & tile, int segIndex1, int segIndex2 ) 
{
	CSegment reflect;

	int numSides = (int)tile.numSides();
	if( segIndex1 < 0 || segIndex1 >= numSides ||
		segIndex2 < 0 || segIndex2 >= numSides ||
		segIndex1 == segIndex2 )
	{
		assert( false );
		return reflect;
	}

	if( segIndex2 < segIndex1 )
		segIndex2 += numSides;

	CVector3D p1, p3;
	CVector3D p2 = tile.m_center;
	if( isInfinite( p2 ) )
		p2.empty();
	if( even( numSides ) )
	{
		int diff = segIndex2 - segIndex1;
		int r1 = segIndex1 + diff / 2;
		bringIntoRange( r1, numSides );

		int r2 = r1 + numSides/2;
		bringIntoRange( r2, numSides );

		// We might need to reflect through an edge midpoint or a vertex.
		if( even( diff ) )
		{
			p1 = tile.m_segments[r1].midpoint();
			p3 = tile.m_segments[r2].midpoint();
		}
		else
		{
			p1 = tile.m_segments[r1].m_p2;
			p3 = tile.m_segments[r2].m_p2;
		}
	}
	else
	{
		// Implement me!
		assert( false );
		return reflect;
	}

	reflect.m_p1 = p1;
	reflect.m_p2 = p3;

	// Can we make a circle out of the points?
	CCircle temp;
	if(	temp.from3Points( p1, p2, p3 ) )
	{
		reflect.m_type = CSegment::ARC;
		reflect.m_center = temp.m_center;
	}
	else
	{
		// The circle construction fails if the points
		// are colinear (if the arc has been transformed into a line).
		reflect.m_type = CSegment::LINE;
	}

	return reflect;
}

void 
CCell::rotateOne() 
{
	CVector3D fixedPlus = m_tile.m_center;
	CVector3D fixedNeg = CTransformable::reflectPoint( m_vertexCircle, fixedPlus );
	double rotation = 2*M_PI/m_tile.numSides();

	for( uint i=0; i<m_stickers.size(); i++ )
		m_stickers[i].twist( rotation, fixedPlus, fixedNeg );
}