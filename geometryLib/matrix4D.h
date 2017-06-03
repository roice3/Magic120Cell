#pragma once

#define DO_OP_MATRIX_NEW( operation ) \
	CMatrix4D ret; \
	for( int i=0; i<4; i++ ) \
		for( int j=0; j<4; j++ ) \
			ret.m[i][j] = m[i][j] operation rhs.m[i][j]; \
	return( ret );

#define DO_OP_MATRIX_INPLACE( operation ) \
	for( int i=0; i<4; i++ ) \
		for( int j=0; j<4; j++ ) \
			m[i][j] operation= rhs.m[i][j]; \
	return( *this );

#define DO_OP_M_SCALAR_NEW( operation ) \
	CMatrix4D ret; \
	for( int i=0; i<4; i++ ) \
		for( int j=0; j<4; j++ ) \
			ret.m[i][j] = m[i][j] operation a; \
	return( ret );

#define DO_OP_M_SCALAR_INPLACE( operation ) \
	for( int i=0; i<4; i++ ) \
		for( int j=0; j<4; j++ ) \
			m[i][j] operation= a; \
	return( *this );

class CMatrix4D
{
public:

	CMatrix4D()
	{
		reset();
	}

	void reset()
	{
		for( int i=0; i<4; i++ )
			for( int j=0; j<4; j++ )
				m[i][j] = 0;
	}

	void setIdentity()
	{
		for( int i=0; i<4; i++ )
			for( int j=0; j<4; j++ )
				m[i][j] = i == j ? 1 : 0;
	}

	CMatrix4D & operator = ( const CMatrix4D & rhs )
	{
		for( int i=0; i<4; i++ )
			for( int j=0; j<4; j++ )
				m[i][j] = rhs.m[i][j];
		return *this;
	}	

	void copyFrom( const double from[][4] )
	{
		for( int i=0; i<4; i++ )
			for( int j=0; j<4; j++ )
				m[i][j] = from[i][j];
	}

	CMatrix4D operator + ( const CMatrix4D & rhs ) const
	{
		DO_OP_MATRIX_NEW(+);
	}
	CMatrix4D &	operator += ( const CMatrix4D & rhs )
	{
		DO_OP_MATRIX_INPLACE(+);
	}
	CMatrix4D operator * ( const CMatrix4D & rhs ) const
	{
		CMatrix4D ret;
		for( int i=0; i<4; i++ )
			for( int j=0; j<4; j++ )
				for( int k=0; k<4; k++ )
					ret.m[i][j] += m[i][k]*rhs.m[k][j];
		return( ret );
	}
	CMatrix4D & operator *= ( const CMatrix4D & rhs )
	{
		CMatrix4D temp = (*this) * rhs;
		*this = temp;
		return *this;
	}

	CMatrix4D operator * ( double a ) const
	{
		DO_OP_M_SCALAR_NEW(*);
	}
	CMatrix4D &	operator *= ( double a )
	{
		DO_OP_M_SCALAR_INPLACE(*);
	}

	double m[4][4];
};