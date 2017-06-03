#pragma once
#include <vector>
#include <float.h>
#include <functional>
#include <map>
#include "helper.h"

typedef unsigned int uint;

#define DO_OP_VECTOR_NEW( operation ) \
	CSameVector ret; \
	for( int i=0; i<Size; ++i ) \
        ret.m_components[i] = m_components[i] operation rhs.m_components[i]; \
	ret.CheckVector(); \
	return( ret );

#define DO_OP_VECTOR_INPLACE( operation ) \
	for( int i=0; i<Size; ++i ) \
        m_components[i] operation= rhs.m_components[i]; \
	CheckVector(); \
	return( *this );

#define DO_OP_SCALAR_NEW( operation ) \
	CSameVector ret; \
	for( int i=0; i<Size; ++i ) \
        ret.m_components[i] = m_components[i] operation a; \
	ret.CheckVector(); \
	return( ret );

#define DO_OP_SCALAR_INPLACE( operation ) \
	for( int i=0; i<Size; ++i ) \
        m_components[i] operation= a; \
	CheckVector(); \
	return( *this );

inline bool isNaN( double value )
{
	int floatType = _fpclass( value );
	return
		_FPCLASS_SNAN == floatType ||
		_FPCLASS_QNAN == floatType ||
		_FPCLASS_NINF == floatType ||
		_FPCLASS_PINF == floatType;
}

template< int nSize >
class CVectorND
{
public:

	static const int Size = nSize;

	typedef CVectorND<nSize> CSameVector;

	// Make component values public for easy access.
	// XXX - bite the bullet and make these private at some point.
	double m_components[16];

	// Convenient access.
	const double & x() const { return m_components[0]; }
	const double & y() const { return m_components[1]; }
	const double & z() const { return m_components[2]; }
	const double & u() const { return m_components[3]; }
	const double & v() const { return m_components[4]; }

	double & x() { return m_components[0]; }
	double & y() { return m_components[1]; }
	double & z() { return m_components[2]; }
	double & u() { return m_components[3]; }
	double & v() { return m_components[4]; }

	void set_x( double _x ) { m_components[0] = _x; }
	void set_y( double _y ) { m_components[1] = _y; }
	void set_z( double _z ) { m_components[2] = _z; }
	void set_u( double _u ) { m_components[3] = _u; }
	void set_v( double _v ) { m_components[4] = _v; }

	// Construction.
	CVectorND()
	{
		InitDebug();
		reset();
	}

	void reset()
	{
		for( int i = 0; i < Size; ++i )
			m_components[i] = 0;
	}

	bool valid() const
	{
		for( int i = 0; i < Size; ++i )
			if( isNaN( m_components[i] ) )
				return false;
		return true;
	}

	CSameVector & operator = ( const CSameVector & rhs )
	{
		for( int i = 0; i < Size; ++i )
			m_components[i] = rhs.m_components[i];
		return *this;
	}

	bool operator == ( const CSameVector & rhs ) const
	{
		for( int i = 0; i < Size; ++i )
			if( m_components[i] != rhs.m_components[i] )
				return false;
		return true;
	}

	// Same as above, but for use when a tolerance is needed.
	bool compare( const CSameVector & rhs ) const
	{
		for( int i = 0; i < Size; ++i )
			if( ! IS_ZERO( m_components[i] - rhs.m_components[i] ) )
				return false;
		return true;
	}

	bool operator != ( const CSameVector & rhs ) const
	{
		for( int i = 0; i < Size; ++i )
			if( m_components[i] != rhs.m_components[i] )
				return true;
		return false;
	}

	// Arithmetic operators.
	// NOTE: Operations on vectors of different dimension will just return a 0 dimension vector.
	CSameVector	operator +  ( const CSameVector & rhs ) const
	{
		DO_OP_VECTOR_NEW(+);
	}
	CSameVector &	operator += ( const CSameVector & rhs )
	{
		DO_OP_VECTOR_INPLACE(+);
	}
	CSameVector	operator -  ( const CSameVector & rhs ) const
	{
		DO_OP_VECTOR_NEW(-);
	}
	CSameVector &	operator -= ( const CSameVector & rhs )
	{
		DO_OP_VECTOR_INPLACE(-);
	}
	CSameVector	operator +  ( double a ) const
	{
		DO_OP_SCALAR_NEW(+);
	}
	CSameVector &	operator += ( double a )
	{
		DO_OP_SCALAR_INPLACE(+);
	}
	CSameVector	operator -  ( double a ) const
	{
		DO_OP_SCALAR_NEW(-);
	}
	CSameVector &	operator -= ( double a )
	{
		DO_OP_SCALAR_INPLACE(-);
	}
	CSameVector	operator *  ( double a ) const
	{
		DO_OP_SCALAR_NEW(*);
	}
	CSameVector &	operator *= ( double a )
	{
		DO_OP_SCALAR_INPLACE(*);
	}
	CSameVector	operator /  ( double a ) const
	{
		DO_OP_SCALAR_NEW(/);
	}
	CSameVector &	operator /= ( double a )
	{
		DO_OP_SCALAR_INPLACE(/);
	}
	// Unary negation
	CSameVector	operator -	() const
	{
		CSameVector ret = *this * -1;
		return ret;
	}

	// Dot Product.
	double dot( const CSameVector & rhs ) const
	{
		CheckVector();
		double ret = 0;
		for( int i=0; i<Size; ++i )
			ret += m_components[i] * rhs.m_components[i];
		return ret;
	}

	// The square of the magnitude.  This is here for optimizations.  
	// It allows avoiding square roots when doing magnitude comparisons.
	double magSquared() const
	{
		CheckVector();
		double magSquared = 0;
		for( int i=0; i<Size; ++i )
			magSquared += m_components[i] * m_components[i];
		return ( magSquared );
	}

	// Absolute Value.
	double abs() const
	{
		return( sqrt( magSquared() ) );
	}

	// Calculate the distance to another vector.
	double distance( const CSameVector & rhs ) const
	{
		CheckVector();
		double magSquared = 0;
		for( uint i=0; i<Size; ++i )
		{
			double componentOffset = rhs.m_components[i] - m_components[i];
			magSquared += componentOffset * componentOffset;
		}
		return sqrt( magSquared );
	}

	// Normalize this vector.  Returns false if degenerate.
	bool normalize()
	{
		CheckVector();
		double magnitude = abs();
		if( IS_ZERO( magnitude ) )
			return false;
		*this /= magnitude;
		return true;
	}

	// Empty out this vector.
	void empty()
	{
		for( int i=0; i<Size; ++i )
			m_components[i] = 0;
	}

	bool isEmpty()
	{
		return IS_ZERO( magSquared() );
	}

	// Rotates the point described by this vector parallel to a 
	// plane defined by 2 coordinate axis (x=0, y=1, z=2, etc.).
	void rotate( int axis1, int axis2, double angle )
	{
		// Copy of the 2 components we need for calculations
		double component1 = m_components[axis1];
		double component2 = m_components[axis2];

		// Do the rotation.
		m_components[axis1] = cos( angle ) * component1 - sin( angle ) * component2; 
		m_components[axis2] = sin( angle ) * component1 + cos( angle ) * component2;

		CheckVector();
	}

	// Unsigned (not handed) angle between 0 and pi.
	double angleTo( const CSameVector & p2 ) const
	{
		double magmult = this->abs() * p2.abs();
		if( IS_ZERO( magmult ) )
			return 0;

		// Make sure the val we take acos() of is in range.
		// Floating point errors can make us slightly off and cause acos() to return bad values.
		double val = dot( p2 ) / magmult;
		if( val > 1 )
		{
			assert( IS_ZERO( 1 - val ) );
			val = 1;
		}
		if( val < -1 )
		{
			assert( IS_ZERO( -1 - val ) );
			val = -1;
		}

		return( acos( val ) );
	}

	// Central projections.
	// dimension (x=1, y=2, etc.) should be less than or equal to Size.
	// distance is the distance to the point we are doing perspective projection from.
	CSameVector & project( int dimension, double distance )
	{
		assert( dimension <= Size );
		int index = dimension - 1;

		// Required for calulations.
		double denominator = distance - m_components[index];
		if( IS_ZERO( denominator ) )
			denominator = 0;

		// Make points with a negative denominator invalid.
		if( denominator < 0 )
			denominator = 0;

		// The projection.
		for( int i=0; i<Size; i++ )
		{
			if( i != index )
				m_components[i] *= distance / denominator;
			else
				m_components[i] = 0;
		}

		return( *this );
	}	

private:

	// debug helpers
#ifdef _DEBUG_NOT_USING
	static const int DebugSize = 8;
	void InitDebug()
	{
		for( int i = Size; i < DebugSize; ++i )
			m_components[i] = 42;
	}
	void CheckVector() const
	{
		for( int i = Size; i < DebugSize; ++i )
			assert( m_components[i] == 42 );
	}
#else
	void InitDebug() {}
	void CheckVector() const {}
#endif

};

// Specialized 3D version.
class CVector3D : public CVectorND<3>
{
public:

	CVector3D()
	{
	}
	CVector3D( const CSameVector & rhs )
	{
		static_cast<CVectorND<3>&>(*this) = rhs;
	}
	CVector3D( double x, double y, double z )
	{
		m_components[0] = x;
		m_components[1] = y;
		m_components[2] = z;
	}

	// Cross Product.
	CVector3D cross( const CVector3D & vector3D ) const
	{
		double xVal = m_components[1] * vector3D.m_components[2] - m_components[2] * vector3D.m_components[1];
		double yVal = m_components[2] * vector3D.m_components[0] - m_components[0] * vector3D.m_components[2];
		double zVal = m_components[0] * vector3D.m_components[1] - m_components[1] * vector3D.m_components[0];
		return CVector3D( xVal, yVal, zVal );
	}
	/* This was causing trouble (getting called and altering vectors when I wanted the version above to be used).
	CVector3D &	cross( const CVector3D & vector3D )
	{
		double xVal = m_components[1] * vector3D.m_components[2] - m_components[2] * vector3D.m_components[1];
		double yVal = m_components[2] * vector3D.m_components[0] - m_components[0] * vector3D.m_components[2];
		double zVal = m_components[0] * vector3D.m_components[1] - m_components[1] * vector3D.m_components[0];
		m_components[0] = xVal;
		m_components[1] = yVal;
		m_components[2] = zVal;
		return( *this );
	} */

	// NOTE: angle should be in radians.
	void rotateAboutAxis( const CVector3D & _axis, double angle )
	{
		// normalize the axis
		CVector3D axis( _axis );
		axis.normalize();
		double _x = axis.m_components[0];
		double _y = axis.m_components[1];
		double _z = axis.m_components[2];

		// build the rotation matrix - I got this from http://www.makegames.com/3dRotation/
		double c = cos( angle );
		double s = -1 * sin( angle );
		double t = 1 - c;
		double mRot[3][3] = 
		{
			{ t*_x*_x + c,		t*_x*_y - s*_z, t*_x*_z + s*_y },
			{ t*_x*_y + s*_z,	t*_y*_y + c,	t*_y*_z - s*_x },
			{ t*_x*_z - s*_y,	t*_y*_z + s*_x, t*_z*_z + c },
		};

		const double & x = m_components[0];
		const double & y = m_components[1];
		const double & z = m_components[2];

		// do the multiplication
		CVector3D result;
		result.m_components[0] = mRot[0][0] * x + mRot[1][0] * y + mRot[2][0] * z;
		result.m_components[1] = mRot[0][1] * x + mRot[1][1] * y + mRot[2][1] * z;
		result.m_components[2] = mRot[0][2] * x + mRot[1][2] * y + mRot[2][2] * z;

		// update this vector
		(*this) = result;
	}
};

// Specialized 4D version.
class CVector4D : public CVectorND<4>
{
public:

	CVector4D()
	{
	}
	CVector4D( const CSameVector & rhs )
	{
		static_cast<CVectorND<4>&>(*this) = rhs;
	}
	CVector4D( double x, double y, double z, double w )
	{
		m_components[0] = x;
		m_components[1] = y;
		m_components[2] = z;
		m_components[3] = w;
	}

	// NOTE: angle should be in radians.
	// NOTE: This only works on the first 3 components (will not alter the 4th component).
	void rotateAboutAxis( const CVector3D & _axis, double angle )
	{
		CVector3D result;
		result.m_components[0] = m_components[0];
		result.m_components[1] = m_components[1];
		result.m_components[2] = m_components[2];

		result.rotateAboutAxis( _axis, angle );

		m_components[0] = result.m_components[0];
		m_components[1] = result.m_components[1];
		m_components[2] = result.m_components[2];
	}

	// Rotate using a 4D rotation matrix.
	void rotateFromMatrix( double mRot[][4] )
	{
		CVector4D copy( *this );
		for( int i=0; i<4; i++ )
		{
			m_components[i] = 
				copy.m_components[0]*mRot[i][0] + 
				copy.m_components[1]*mRot[i][1] + 
				copy.m_components[2]*mRot[i][2] + 
				copy.m_components[3]*mRot[i][3];
		}
	}
};

// Binary Predicate for ordering CVectorNDs (so they can be used as keys to a map).
// NOTE: I made the comparison tolerance safe.
template< int nSize >
struct compareND : public std::binary_function<CVectorND<nSize>,CVectorND<nSize>,bool> 
{
	bool operator()( const CVectorND<nSize> & _Left, const CVectorND<nSize> & _Right ) const
	{
		for( int i=0; i<nSize; i++ )
		{
			if( IS_LESS_THAN( _Left.m_components[i], _Right.m_components[i] ) )
				return true;

			if( IS_GREATER_THAN( _Left.m_components[i], _Right.m_components[i] ) )
				return false;
		}

		// Making it here means we are equal.
		return false;
	}
};

// Specialized versions.
typedef compareND<3> compare3D;
typedef compareND<4> compare4D;

typedef std::map<CVector3D,int,compare3D> Vec3ToIntMap;
typedef std::map<CVector3D,bool,compare3D> Vec3ToBoolMap;

typedef std::map<CVector4D,int,compare4D> VecToIntMap;
typedef std::map<CVector4D,int,compare4D>::const_iterator VecToIntMapIterator;