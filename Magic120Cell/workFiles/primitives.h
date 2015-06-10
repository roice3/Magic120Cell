#pragma once
#include "vectorND.h"

#pragma managed(push, off)


struct SPrimitive2D
{
	//
	// Forced Implementation.
	//

	// Transformation helpers.
	virtual void offset( const CVector4D & amount ) = 0;
	virtual void scaleAboutOrigin( double factor ) = 0;
	virtual void applyCellTransforms( const CVector4D & cellTransforms, bool undo ) = 0;	// XXX - this could potentially go away now that I am adding rotateFromMatrix() method below.
	virtual void rotateFromMatrix( double mRot[][4] ) = 0;

	// NOTE: This is only meant to be called pre-4D transformations.
	virtual void rotateAboutAxis( CVector3D axis, double rotation ) = 0;

	// Rendering method.
	virtual void render() = 0;

	// Project our points.
	// Returns false if projection resulted in DNE for any points.
	virtual bool doProjection( double projectionDistance ) = 0;
};

struct SPrimitive3D : public SPrimitive2D
{

	SPrimitive3D();
	virtual SPrimitive3D & operator = ( const SPrimitive3D & rhs );
	
	void setValid( bool valid );
	bool getValid() const;

	virtual const CVector4D & getCenter() const 
	{ 
		assert( false );
		return dummy; 
	}

	//
	// Forced Implementation.
	//

	virtual const CVector4D & getConstantPoint() const = 0;

private:

	CVector4D dummy;

	// Used to track if projecting resulted in any DNEs.
	bool m_valid;
};

struct SQuad : public SPrimitive2D
{
	SQuad();
	SQuad & operator = ( const SQuad & rhs );

	// Standard methods.
	void offset( const CVector4D & amount );
	void scaleAboutOrigin( double factor );
	void applyCellTransforms( const CVector4D & cellTransforms, bool undo );
	void rotateFromMatrix( double mRot[][4] );
	void render();
	bool doProjection( double projectionDistance );
	void rotateAboutAxis( CVector3D axis, double rotation );

	CVector4D m_a;
	CVector4D m_b;
	CVector4D m_c;
	CVector4D m_d;
};

struct SPentagon : public SPrimitive2D
{
	// The constructor will generate a standard pentagon centered at the origin.
	SPentagon();
	SPentagon & operator = ( const SPentagon & rhs );

	// Standard methods.
	void offset( const CVector4D & amount );
	void scaleAboutOrigin( double factor );
	void applyCellTransforms( const CVector4D & cellTransforms, bool undo );
	void rotateFromMatrix( double mRot[][4] );
	void render();
	bool doProjection( double projectionDistance );
	void rotateAboutAxis( CVector3D axis, double rotation );

	// Additional transformation helpers.
	void mirrorPoints();
	void reversePoints();
	void rotatePoints3D( int axis, double angle );		// XXX - replace this method.
	void offset( int axis, double amount );
	void scaleAboutCenter( double factor );

	const CVector4D & getCenter() const
	{
		return m_center;
	}

	// NOTE: This should only be called after this pentagon has been projected to 3D.
	CVector3D getPlaneNormal();

	// XXX - make this private.
	CVector4D m_center;
	CVector4D m_points[5];
};

struct SDodecahedron : public SPrimitive3D
{
	// The constructor will generate a standard dodecahedron centered at the origin.
	SDodecahedron();
	SPrimitive3D & operator = ( const SPrimitive3D & rhs );
	SDodecahedron & operator = ( const SDodecahedron & rhs );

	// Standard methods.
	void offset( const CVector4D & amount );
	void scaleAboutOrigin( double factor );
	void applyCellTransforms( const CVector4D & cellTransforms, bool undo );
	void rotateFromMatrix( double mRot[][4] );
	void render();
	bool doProjection( double projectionDistance );
	void rotateAboutAxis( CVector3D axis, double rotation );

	const CVector4D & getCenter() const
	{
		return m_center;
	}

	const CVector4D & getConstantPoint() const { return m_center; }

	// Should only be called on a dodecahedron that has been projected already.
	bool isInverted();

public:

	// XXX - make these private.
	CVector4D m_center;
	SPentagon m_faces[12];
};

struct SShape2CSticker : public SPrimitive3D
{
	SShape2CSticker();
	SPrimitive3D & operator = ( const SPrimitive3D & rhs );

	// Standard methods.
	void offset( const CVector4D & amount );
	void scaleAboutOrigin( double factor );
	void applyCellTransforms( const CVector4D & cellTransforms, bool undo );
	void rotateFromMatrix( double mRot[][4] );
	void render();
	bool doProjection( double projectionDistance );
	void rotateAboutAxis( CVector3D axis, double rotation );

	// Creation helper.
	void generateFrom2Pentagons( const SPentagon & top, const SPentagon & bottom );

	const CVector4D & getConstantPoint() const { return m_top.getCenter(); }

public:

	// XXX - make private.
	SPentagon m_top;
	SPentagon m_bottom;
	SQuad m_sides[5];
};

// Used for 3C and 4C stickers.
struct SHexaHedron : public SPrimitive3D
{
	SHexaHedron();
	SPrimitive3D & operator = ( const SPrimitive3D & rhs );

	// Standard methods.
	void offset( const CVector4D & amount );
	void scaleAboutOrigin( double factor );
	void applyCellTransforms( const CVector4D & cellTransforms, bool undo );
	void rotateFromMatrix( double mRot[][4] );
	void render();
	bool doProjection( double projectionDistance );
	void rotateAboutAxis( CVector3D axis, double rotation );

	// Creation helper.
	// Generates from 2 dodecahedral vertices.
	void generate3CSticker( const CVector4D & v1, const CVector4D & v2, double r1, double r2, double r3 );
	void generate4CSticker( const CVector4D & v1, const CVector4D & v2, 
		const SHexaHedron & h1, const SHexaHedron & h2, const SHexaHedron & h3 );

	const CVector4D & getConstantPoint() const { return m_constantPoint; }

public:

	// XXX - make private.
	SQuad m_sides[6];

	// Point that stays in the same location when sticker is scaled.
	CVector4D m_constantPoint;
};

namespace
{
	void applyCellTransforms( CVector4D & point, const CVector4D & cellTransforms )
	{
		// These are used to generate a ring.
		point.rotate( 0, 1, cellTransforms.m_components[0] );
		point += CVector4D( 0, 0, 0, standardFaceOffset );
		point.rotate( 2, 3, cellTransforms.m_components[1] );

		// These move the full ring into it's other positions.
		if( 0 != cellTransforms.m_components[3] )
		{
			// Non overlapping rings.
			// There might be a simpler way to do this (with less rotations or something).
			// I got a bit lucky figuring this out.
			point.rotate( 0, 2, dihedral - M_PI );
			point.rotate( 2, 3, deg2rad( 36.0 ) );
			point.rotate( 0, 1, deg2rad( -54.0 ) );
			point.rotate( 1, 2, dihedral - M_PI );
			point.rotate( 0, 1, deg2rad( 18.0 ) + cellTransforms.m_components[3] );
		}

		if( 0 != cellTransforms.m_components[2] )
		{
			point.rotate( 1, 3, cellTransforms.m_components[2] );
			point.rotate( 0, 2, cellTransforms.m_components[2] );
		}
	}
}


#pragma managed(pop)