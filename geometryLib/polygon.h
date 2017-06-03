#pragma once

#include "gl/displayList.h"
#include "puzzle/state.h"
#include "vectorND.h"


enum Geometry
{
	Spherical,
	Euclidean,
	Hyperbolic
};

namespace
{
	bool isInfinite( const CVector3D & in )
	{
		// XXX - ugly hack I'd like to improve.
		return !in.valid() || in.abs() > 10000;
	}

	// Returns the geometry induced by a polygon with p points.
	Geometry geometry( int p )
	{
		if( p < 6 )
			return Spherical;
		else if( p == 6 )
			return Euclidean;

		return Hyperbolic;
	}
}

class CCircle;
class CSegment;
class CPolygon;

class CTransformable
{
public:

	virtual void reflect( const CSegment & s ) = 0;

	// Helpers.
	static CVector3D reflectPoint( const CCircle & c, const CVector3D & in ); 
	static CVector3D reflectPoint( const CSegment & s, const CVector3D & in );
};

// This class is 2-dimensional only for now.
// To make 3D, we'd need a normal vector.
class CCircle : public CTransformable
{
public:

	bool operator == ( const CCircle & rhs ) const;
	bool operator != ( const CCircle & rhs ) const;

	bool from3Points( const CVector3D & p1, const CVector3D & p2, const CVector3D & p3 );

	bool isPointInside( const CVector3D & test ) const;		// Strictly less than.
	bool isPointOn( const CVector3D & test ) const;
	
	// Get the intersection points with a segment.
	// Returns 0, 1, or 2 depending on how many are found (p1,p2 filled out accordingly)
	// Returns -1 if the segment is an arc coincident with the circle (infinite number of intersection points).
	int getIntersectionPoints( const CSegment & segment, CVector3D & p1, CVector3D & p2 ) const;

	// See if we intersect a poly.
	bool intersects( const CPolygon & poly ) const;

	// CTransformable.
	void reflect( const CSegment & s );

public:

	CVector3D m_center;
	double m_radius;
};

// Support for line and arc segments.
// Arc segments are 2D for the moment.
class CSegment : public CTransformable
{
public:

	enum ESegmentType
	{
		LINE,
		ARC,
	};

	CSegment() 
	{
		m_type = LINE;
		m_clockwise = false;
		m_color = CColor( 0,0,0,1 );
	};

	bool valid() const;

	bool operator == ( const CSegment & rhs ) const;
	bool operator != ( const CSegment & rhs ) const;

	// These methods only apply to arcs.
	double radius() const;
	double angle() const;	// in radians between 0 and 2*pi, unsigned
	CCircle getCircle() const;

	// Methods that apply to both segment types.
	bool pointOn( const CVector3D & test ) const;
	CVector3D midpoint() const;
	double length() const;
	void reverse();
	std::vector<CVector3D> subdivide( int numSegments ) const;

	// CTransformable.
	void reflect( const CSegment & s );

	// Get the intersection points with another segment.
	// Returns 0, 1, or 2 depending on how many are found (p1,p2 filled out accordingly)
	// Returns -1 if there are an infinite number of intersection points.
	int getIntersectionPoints( const CSegment & segment, CVector3D & p1, CVector3D & p2 ) const;

public:

	ESegmentType m_type;
	CVector3D m_p1;
	CVector3D m_p2;

	// These only apply to arc segments.
	CVector3D m_center;
	bool m_clockwise;

	CColor m_color;
};


class CPolygon : public CTransformable
{
public:

	CPolygon() { m_twistIdx = -1; }

	void clear();

	// This will return false if we have been projected to infinity.
	// (only will apply to spherical tilings).
	bool valid() const;

	// Creation helpers.
	// Returns the length of one segment.
	// These will clear out any existing data.
	void createRegular( int numSides );

	uint numSides() const { return (uint)m_segments.size(); }

	// OpenGL rendering.
	bool renderDl( const CColor & c ) const;	// Returns false if no display list.
	void compileDl( Geometry g, int lod, bool drawEdges );
	void render( Geometry g, int lod, bool drawEdges, const CColor & c ) const;
	void render( Geometry g, int lod, bool drawEdges ) const;
	void renderFill( Geometry g, int lod ) const;
	void renderEdges( int lod ) const;

	// Slice this polygon using a circle.
	// This method is not fully general yet, and will return false 
	// if you stretch it beyond its capabilities.
	// The output will be appended (vector is not cleared internally).
	bool slice( const CCircle & c, std::vector<CPolygon> & output ) const;

	// CTransformable.
	// NOTE: This currently reverses our orientation.
	void reflect( const CSegment & s );

	// Transform from a twist.
	// This is an elliptic Mobius transform.
	void twist( double rotation, const CVector3D & fixedPlus, const CVector3D & fixedNeg );
	void twistCenter( double rotation, const CVector3D & fixedPlus, const CVector3D & fixedNeg );

	// NOTE: These may not be the true values if points are at infinity.
	// NOTE: Centroid is also not fully accurate for arcs, but our calc is 
	//		 actually advantagous when it comes to the drawing we need to do.
	CVector3D centroid() const;
	double length() const;

	// Returns true if CCW, false if CW.
	// NOTE: only makes sense for 2D polygons.
	bool orientation() const;

	// Point-in-poly check.
	bool isPointInside( const CVector3D & p ) const;

private:

	void drawEdgePoints( int lod ) const;
	void drawFillPoints( int lod, bool addInfiniteEnds ) const;

public:

	// These should be in sequential order,
	// adjacent and continuous.
	// XXX - also enforce winding direction to be counterclock?
	std::vector<CSegment> m_segments;

	// NOTE: This is not always the Euclidean center.
	//		 (not the same as the centroid).
	CVector3D m_center;

	// Display list for this polygon.
	CDisplayList m_dl;

	// A cached index for the circle that will twist this polygon.
	// XXX - it would be logically cleaner to store these in the cell class.
	int m_twistIdx;
};

struct CTwistData
{
public:

	CTwistData() { m_twistingReversed = false; }

	// Everything needed for the calculations of a twist.
	std::vector<CCircle> twisters;
	std::vector<CCircle> oppTwisters;
	CVector3D fixedPlus;
	CVector3D fixedNeg;
	bool reduceSliceCount;
	int slicemask;

	// True for some slaves of non-orientable puzzles.
	bool m_twistingReversed;

	void preCalcItems( const CCircle & reflectionCircle )
	{
		fixedNeg = CTransformable::reflectPoint( reflectionCircle, fixedPlus );
		preCalcTwistOutside();
	}

	// This does all the checks of whether or not to twist a sticker.
	bool twistSticker( Geometry g, const CPolygon & s ) const;

private:

	// Tracks whether or not to twist outside for all the various twisting circles.
	std::vector<bool> twistOutside;
	std::vector<bool> oppTwistOutside;

	void preCalcTwistOutside()
	{
		for( uint i=0; i<twisters.size(); i++ )
			twistOutside.push_back( calcTwistOutside( twisters[i] ) );
		for( uint i=0; i<oppTwisters.size(); i++ )
			oppTwistOutside.push_back( calcTwistOutside( oppTwisters[i] ) );
	}

	// If our fix is outside the circle, we'll  
	// need to twist outside the circle.
	bool calcTwistOutside( const CCircle & twistingCircle ) const
	{
		// Fix at infinity is a special case
		// of this, hence the valid check.
		return !fixedPlus.valid() || 
			!twistingCircle.isPointInside( fixedPlus );
	}
};

class CTwistDataArray
{
public:

	bool willTwist( const CPolygon & tile ) const
	{
		for( uint i=0; i<m_twistDataArray.size(); i++ )
		{
			const std::vector<CCircle> & twisters = m_twistDataArray[i].twisters;
			if( !twisters.size() )
				continue;

			const CCircle & c = twisters[0];
			if( c.isPointInside( tile.m_center ) || c.intersects( tile ) )
				return true;
		}

		return false;
	}

	// If we need to twist this sticker, this returns the index of the twist data.
	int twistSticker( Geometry g, const CPolygon & s ) const
	{
		for( uint i=0; i<m_twistDataArray.size(); i++ )
		{
			if( m_twistDataArray[i].twistSticker( g, s ) )
				return i;
		}

		return -1;
	}

	const CTwistData & get( int index ) const
	{
		return m_twistDataArray[index];
	}

	void add( const CTwistData & twistData )
	{
		m_twistDataArray.push_back( twistData );
	}

	void clear()
	{
		m_twistDataArray.clear();
	}

private:

	std::vector<CTwistData> m_twistDataArray;
};

class CCell : public CTransformable
{
public:

	CCell() 
	{ 
		m_opp = -1;
		m_lod = 0;	// Level of detail
	}

	void invalidate() { m_dl.clear(); }

	// Precalculate all the stickers we'll need to twist,
	// so it doesn't have to be redone at every step.
	void preCalcTwistingSpherical( const CTwistDataArray & twistDataArray );

	// This will invalidate if the cell and relevant stickers which will be twisted.
	// Returns whether cell was invalidated or not.
	bool preCalcTwisting( Geometry g, const CTwistDataArray & twistData, bool master );

	// Rendering method.
	void render( Geometry g, bool drawEdges, bool twisting, double rotation, 
		const CTwistDataArray & twistData, const CState & state, int cellIndex );
	void renderForPicking( Geometry g );

	// CTransformable.
	void reflect( const CSegment & s );

	// Special reflection helper.
	// Returns a segment that will reflect the input tile (or corresponding cell)
	// such that the two segments are swapped.
	static CSegment reflectBySwappingSegments( const CPolygon & tile, int segIndex1, int segIndex2 );

	void rotateOne();

	// Our bounding tile and stickers.
	CPolygon m_tile;
	std::vector<CPolygon> m_stickers;

	// The twisting circles associated with this tile.
	// The non-euclidean center of twist is the center of the tile above.
	std::vector<CCircle> m_twisters;

	// A circle which goes through the vertices of the cell.
	// Used in some situations where we need a reflection circle.
	CCircle m_vertexCircle;

	// Integer of the opposite cell, if any.
	int m_opp;

	// LOD;
	int m_lod;

private:

	// Display list for this cell.
	CDisplayList m_dl;

	void renderTwisting( Geometry g, bool drawEdges, double rotation, 
		const CTwistDataArray & twistData, const CState & state, int cellIndex ) const;
};