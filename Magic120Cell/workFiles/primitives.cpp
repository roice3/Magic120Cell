#include <stdafx.h>
#include "cell.h"

#include "helper.h"

#pragma unmanaged


namespace
{
	void 
	vertex( CVector4D & p )
	{
		glVertex3d( 
			(GLdouble)p.m_components[0], 
			(GLdouble)p.m_components[1], 
			(GLdouble)p.m_components[2] );
	}

	CVector3D 
	getPlaneNormal( const CVector4D & p1, const CVector4D & p2, const CVector4D & o ) 
	{
		// Calculate the normal based on the pentagonal plane.
		CVector4D vec1temp = p1 - o;
		CVector4D vec2temp = p2 - o;
		CVector3D vec1, vec2;
		// XXX - make operator to get a CVector3D so this is cleaner.
		// XXX - maybe provide a way to do all this just with 4D vectors...
		for( int i=0; i<3; i++ )
		{
			vec1.m_components[i] = vec1temp.m_components[i];
			vec2.m_components[i] = vec2temp.m_components[i];
		}
		CVector3D planeNormal = vec2.cross( vec1 );
		planeNormal.normalize();
		return planeNormal;
	}

	void undoCellTransforms( CVector4D & point, const CVector4D & cellTransforms )
	{
		//
		// Applies everything above in reverse order and with magnitudes *= -1.
		//

		if( 0 != cellTransforms.m_components[2] )
		{
			point.rotate( 0, 2, -cellTransforms.m_components[2] );
			point.rotate( 1, 3, -cellTransforms.m_components[2] );
		}

		if( 0 != cellTransforms.m_components[3] )
		{
			point.rotate( 0, 1, -deg2rad( 18.0 ) - cellTransforms.m_components[3] );
			point.rotate( 1, 2, -dihedral + M_PI );
			point.rotate( 0, 1, -deg2rad( -54.0 ) );
			point.rotate( 2, 3, -deg2rad( 36.0 ) );
			point.rotate( 0, 2, -dihedral + M_PI );
		}

		point.rotate( 2, 3, -cellTransforms.m_components[1] );
		point += CVector4D( 0, 0, 0, -standardFaceOffset );
		point.rotate( 0, 1, -cellTransforms.m_components[0] );
	}

	void applyCellTransforms( CVector4D & point, const CVector4D & cellTransforms, bool undo )
	{
		if( undo )
			undoCellTransforms( point, cellTransforms );
		else
			applyCellTransforms( point, cellTransforms );
	}
}

SPrimitive3D::SPrimitive3D() 
{
	m_valid = false;
}

SPrimitive3D & 
SPrimitive3D::operator = ( const SPrimitive3D & rhs ) 
{
	m_valid = rhs.m_valid;
	return *this;
}

void 
SPrimitive3D::setValid( bool valid ) 
{
	m_valid = valid;
}

bool 
SPrimitive3D::getValid() const
{
	return m_valid;
}

SQuad::SQuad() 
{
}

SQuad & 
SQuad::operator = ( const SQuad & rhs ) 
{
	m_a = rhs.m_a;
	m_b = rhs.m_b;
	m_c = rhs.m_c;
	m_d = rhs.m_d;
	return *this;
}

void 
SQuad::offset( const CVector4D & amount ) 
{
	m_a += amount;
	m_b += amount;
	m_c += amount;
	m_d += amount;
}

void 
SQuad::scaleAboutOrigin( double factor ) 
{
	m_a *= factor;
	m_b *= factor;
	m_c *= factor;
	m_d *= factor;
}

void 
SQuad::applyCellTransforms( const CVector4D & cellTransforms, bool undo ) 
{
	::applyCellTransforms( m_a, cellTransforms, undo );
	::applyCellTransforms( m_b, cellTransforms, undo );
	::applyCellTransforms( m_c, cellTransforms, undo );
	::applyCellTransforms( m_d, cellTransforms, undo );
}

void 
SQuad::render() 
{
	CVector3D planeNormal = getPlaneNormal( m_a, m_b, m_c );
	glNormal3d( 
		(GLdouble)-planeNormal.m_components[0], 
		(GLdouble)-planeNormal.m_components[1], 
		(GLdouble)-planeNormal.m_components[2] );

	glBegin( GL_POLYGON );
		vertex( m_a );
		vertex( m_b );
		vertex( m_c );
		vertex( m_d );
	glEnd();
}

bool 
SQuad::doProjection( double projectionDistance ) 
{
	m_a.project( 4, projectionDistance );
	m_b.project( 4, projectionDistance );
	m_c.project( 4, projectionDistance );
	m_d.project( 4, projectionDistance );
	if( !m_a.valid() || !m_b.valid() || !m_c.valid() || !m_d.valid() )
		return false;

	return true;
}

void 
SQuad::rotateFromMatrix( double mRot[][4] ) 
{
	m_a.rotateFromMatrix( mRot );
	m_b.rotateFromMatrix( mRot );
	m_c.rotateFromMatrix( mRot );
	m_d.rotateFromMatrix( mRot );
}

void 
SQuad::rotateAboutAxis( CVector3D axis, double rotation ) 
{
	m_a.rotateAboutAxis( axis, rotation );
	m_b.rotateAboutAxis( axis, rotation );
	m_c.rotateAboutAxis( axis, rotation );
	m_d.rotateAboutAxis( axis, rotation );
}

SPentagon::SPentagon() 
{
	// Generate our points.
	double r = 1;
	double angle = 0;
	for( int i=0; i<5; i++ )
	{
		CVector4D point;
		point.m_components[0] = r * cos( angle );
		point.m_components[1] = r * sin( angle );
		point.m_components[2] = 0;
		point.m_components[3] = 0;
		m_points[i] = point;
		angle += deg2rad( 360.0 / 5 );
	}

	// The last point is our center.
	m_center = CVector4D();
}

SPentagon & 
SPentagon::operator = ( const SPentagon & rhs ) 
{
	for( int i=0; i<5; ++i )
		m_points[i] = rhs.m_points[i];
	m_center = rhs.m_center;
	return *this;
}

void 
SPentagon::mirrorPoints()
{
	for( int i=0; i<5; i++ )
		m_points[i] *= -1;
	m_center *= -1;
}

void 
SPentagon::reversePoints() 
{
	std::reverse( &m_points[0], &m_points[5] );
}

void 
SPentagon::rotatePoints3D( int axis, double angle )
{
	int axis1 = 0, axis2 = 0;
	switch( axis )
	{
	case 0:
		axis1 = 1;
		axis2 = 2;
		break;
	case 1:
		axis1 = 0;
		axis2 = 2;
		break;
	case 2:
		axis1 = 0;
		axis2 = 1;
		break;
	default:
		assert( false );
	}

	for( int i=0; i<5; i++ )
		m_points[i].rotate( axis1, axis2, angle );
	m_center.rotate( axis1, axis2, angle );
}

void 
SPentagon::offset( int axis, double amount )
{
	for( int i=0; i<5; i++ )
		m_points[i].m_components[axis] += amount;
	m_center.m_components[axis] += amount;
}

void 
SPentagon::offset( const CVector4D & amount ) 
{
	for( int i=0; i<5; i++ )
		m_points[i] += amount;
	m_center += amount;
}

void 
SPentagon::scaleAboutOrigin( double factor ) 
{
	for( int i=0; i<5; i++ )
		m_points[i] *= factor;
	m_center *= factor;
}

void 
SPentagon::applyCellTransforms( const CVector4D & cellTransforms, bool undo ) 
{
	for( int i=0; i<5; i++ )
		::applyCellTransforms( m_points[i], cellTransforms, undo );
	::applyCellTransforms( m_center, cellTransforms, undo );
}

void 
SPentagon::rotateFromMatrix( double mRot[][4] ) 
{
	for( int i=0; i<5; i++ )
		m_points[i].rotateFromMatrix( mRot );
	m_center.rotateFromMatrix( mRot );
}

void 
SPentagon::scaleAboutCenter( double factor ) 
{
	CVector4D center( m_center );
	offset( center * -1 );
	scaleAboutOrigin( factor );
	offset( center );
}

void 
SPentagon::render() 
{
	CVector3D planeNormal = getPlaneNormal();
	glNormal3d( 
		(GLdouble)-planeNormal.m_components[0], 
		(GLdouble)-planeNormal.m_components[1], 
		(GLdouble)-planeNormal.m_components[2] );

	glBegin( GL_POLYGON );
		for( int i=0; i<5; i++ )
			vertex( m_points[i] );
	glEnd();
}

CVector3D 
SPentagon::getPlaneNormal() 
{
	return ::getPlaneNormal( m_points[0], m_points[1], m_center );
}

bool 
SPentagon::doProjection( double projectionDistance ) 
{
	m_center.project( 4, projectionDistance );
	if( !m_center.valid() )
		return false;

	for( int p=0; p<5; p++ )
	{
		m_points[p].project( 4, projectionDistance );
		if( !m_points[p].valid() )
			return false;
	}

	return true;
}

void 
SPentagon::rotateAboutAxis( CVector3D axis, double rotation ) 
{
	m_center.rotateAboutAxis( axis, rotation );
	for( int p=0; p<5; p++ )
		m_points[p].rotateAboutAxis( axis, rotation );
}

SDodecahedron::SDodecahedron() 
{
	//
	// Our faces are 12 pentagons.
	// NOTE: The reversing is required so that our polygons are outward facing from the dodecahedron center.
	//		 I could probably avoid this if I didn't do mirroring, but no big deal to correct for it this way either...
	//

	// Top and bottom.
	m_faces[0].offset( 2, -( ( 1 + golden ) / 2 ) );
	m_faces[1] = m_faces[0];
	m_faces[1].mirrorPoints();
	m_faces[0].reversePoints();

	m_faces[2].rotatePoints3D( 1, dihedral );
	m_faces[2].offset( 0,  shortDist * ( -1 + cos( dihedral ) ) );
	m_faces[2].offset( 2,  ( -( ( 1 + golden ) / 2 ) + shortDist * sin( dihedral ) ) );

	m_faces[3] = m_faces[2];
	m_faces[3].mirrorPoints();
	m_faces[3].reversePoints();

	for( int i=0; i<4; i++ )
	{
		m_faces[2*i+4] = m_faces[2];
		m_faces[2*i+4].rotatePoints3D( 2, -1 * deg2rad( 72.0 ) * ( i+1 ) );

		m_faces[2*i+4+1] = m_faces[2*i+4];
		m_faces[2*i+4+1].mirrorPoints();
		m_faces[2*i+4+1].reversePoints();
	}
}

SPrimitive3D & 
SDodecahedron::operator = ( const SPrimitive3D & rhs ) 
{
	const SDodecahedron & dodec = (const SDodecahedron &)rhs;
	return operator = ( dodec );
}

SDodecahedron &  
SDodecahedron::operator = ( const SDodecahedron & rhs ) 
{
	SPrimitive3D::operator=( rhs );
	m_center = rhs.m_center;
	for( int i=0; i<12; ++i )
		m_faces[i] = rhs.m_faces[i];
	return *this;
}

void 
SDodecahedron::offset( const CVector4D & amount ) 
{
	// Offset the center and the points on the faces.
	m_center += amount;
	for( int f=0; f<12; f++ )
		m_faces[f].offset( amount );
}

void 
SDodecahedron::scaleAboutOrigin( double factor ) 
{
	// NOTE: We could make this work for offset centers,
	//		 but I only need it for centered guys now
	//		 and I am worried about speed so I want to
	//		 do as little computation as possible.
	for( int f=0; f<12; f++ )
		m_faces[f].scaleAboutOrigin( factor );
}

void 
SDodecahedron::applyCellTransforms( const CVector4D & cellTransforms, bool undo ) 
{
	::applyCellTransforms( m_center, cellTransforms, undo );
	for( int f=0; f<12; f++ )
		m_faces[f].applyCellTransforms( cellTransforms, undo );
}

void 
SDodecahedron::rotateFromMatrix( double mRot[][4] ) 
{
	m_center.rotateFromMatrix( mRot );
	for( int f=0; f<12; f++ )
		m_faces[f].rotateFromMatrix( mRot );
}

void 
SDodecahedron::render() 
{
	if( !getValid() )
		return;

	for( int f=0; f<12; f++ )
		m_faces[f].render();
}

bool 
SDodecahedron::doProjection( double projectionDistance ) 
{
	// Project our center.
	m_center.project( 4, projectionDistance );
	if( !m_center.valid() )
		return false;

	// Cycle through the faces and project all our points.
	// If any are DNE, we're done.
	for( int f=0; f<12; f++ )
		if( !m_faces[f].doProjection( projectionDistance ) )
			return false;

	return true;
}

void 
SDodecahedron::rotateAboutAxis( CVector3D axis, double rotation ) 
{
	m_center.rotateAboutAxis( axis, rotation );
	for( int f=0; f<12; f++ )
		m_faces[f].rotateAboutAxis( axis, rotation );
}

bool 
SDodecahedron::isInverted() 
{
	//
	// Now we are going to calculate 2 normals, which we'll use to see if we were mirrored during projection.
	// One normal will be calculated from how the polygons are wound (planeNormal),
	// the other will be calculated from the projected points (projectedNormal).
	//
	// The former needs to be the one used for lighting (it is the one actually normal to the surfaces in 3D).
	//

	// Just use the first face to figure this out.
	SPentagon & p = m_faces[0];

	// Here is the plane normal.
	CVector3D planeNormal = p.getPlaneNormal();

	// Use center of dodecahedron and the center of the pentagonal face to find the projected normal.
	CVector4D normal4D = p.m_center - getCenter();
	CVector3D projectedNormal;
	for( int i=0; i<3; i++ )
	{
		projectedNormal.m_components[i] = normal4D.m_components[i];
	}
	projectedNormal.normalize();
	normal4D.normalize();

	// ok, now we can tell if we've been flipped (mirrored, inverted, whatever).
	return projectedNormal.dot( planeNormal ) <= 0;
}

SShape2CSticker::SShape2CSticker() 
{
}

SPrimitive3D & 
SShape2CSticker::operator = ( const SPrimitive3D & rhs ) 
{
	SPrimitive3D::operator=( rhs );
	const SShape2CSticker & copy = (const SShape2CSticker &)rhs;
	m_top = copy.m_top;
	m_bottom = copy.m_bottom;
	for( int i=0; i<5; i++ )
		m_sides[i] = copy.m_sides[i];
	return *this;
}

void 
SShape2CSticker::generateFrom2Pentagons( const SPentagon & top, const SPentagon & bottom ) 
{
	m_top = top;
	m_bottom = bottom;

	for( int i=0; i<5; i++ )
	{
		int val1 = i;
		int val2 = 4==i ? 0 : i+1;

		m_sides[i].m_a = m_top.m_points[val1];
		m_sides[i].m_b = m_top.m_points[val2];
		m_sides[i].m_c = m_bottom.m_points[val2];
		m_sides[i].m_d = m_bottom.m_points[val1];
	}

	m_bottom.reversePoints();	// For correct outer orientation.
}

void 
SShape2CSticker::offset( const CVector4D & amount ) 
{
	m_top.offset( amount );
	m_bottom.offset( amount );
	for( int i=0; i<5; i++ )
		m_sides[i].offset( amount );
}

void 
SShape2CSticker::scaleAboutOrigin( double factor ) 
{
	m_top.scaleAboutOrigin( factor );
	m_bottom.scaleAboutOrigin( factor );
	for( int i=0; i<5; i++ )
		m_sides[i].scaleAboutOrigin( factor );
}

void 
SShape2CSticker::applyCellTransforms( const CVector4D & cellTransforms, bool undo ) 
{
	m_top.applyCellTransforms( cellTransforms, undo );
	m_bottom.applyCellTransforms( cellTransforms, undo );
	for( int i=0; i<5; i++ )
		m_sides[i].applyCellTransforms( cellTransforms, undo );
}

void 
SShape2CSticker::rotateFromMatrix( double mRot[][4] ) 
{
	m_top.rotateFromMatrix( mRot );
	m_bottom.rotateFromMatrix( mRot );
	for( int i=0; i<5; i++ )
		m_sides[i].rotateFromMatrix( mRot );
}

void 
SShape2CSticker::render() 
{
	if( !getValid() )
		return;

	m_top.render();
	m_bottom.render();
	for( int i=0; i<5; i++ )
		m_sides[i].render();
}

bool 
SShape2CSticker::doProjection( double projectionDistance ) 
{
	if( !m_top.doProjection( projectionDistance ) ||
		!m_bottom.doProjection( projectionDistance ) )
		return false;

	for( int i=0; i<5; i++ )
		if( !m_sides[i].doProjection( projectionDistance ) )
			return false;

	return true;
}

void 
SShape2CSticker::rotateAboutAxis( CVector3D axis, double rotation ) 
{
	m_top.rotateAboutAxis( axis, rotation );
	m_bottom.rotateAboutAxis( axis, rotation );
	for( int i=0; i<5; i++ )
		m_sides[i].rotateAboutAxis( axis, rotation );
}

SHexaHedron::SHexaHedron()
{
}

SPrimitive3D & 
SHexaHedron::operator = ( const SPrimitive3D & rhs )
{
	SPrimitive3D::operator=( rhs );
	const SHexaHedron & copy = (const SHexaHedron &)rhs;
	m_constantPoint = copy.m_constantPoint;
	for( int i=0; i<6; i++ )
		m_sides[i] = copy.m_sides[i];
	return *this;
}

namespace
{
	void scaleAboutPoint( CVector4D & v, double factor, const CVector4D p )
	{
		v -= p;
		v *= factor;
		v += p;
	}
}

#define SETUP_SIDE( s, a, b, c, d ) \
	m_sides[s].m_a = pts[a]; \
	m_sides[s].m_b = pts[b]; \
	m_sides[s].m_c = pts[c]; \
	m_sides[s].m_d = pts[d]

void 
SHexaHedron::generate3CSticker( const CVector4D & v1, const CVector4D & v2, double r1, double r2, double r3 )
{
	// XXX - this could be moved outside of this function to increase performance.
	// (this ends up calculating these scaled vertices more than necessary).
	CVector4D pts[8];
	pts[0] = v1;
	pts[1] = v2;
	pts[2] = v1 * r1;
	pts[3] = v2 * r1;
	pts[4] = v1;
	pts[5] = v2;
	pts[6] = v1;
	pts[7] = v2;

	const CVector4D mid = ( v1 + v2 ) / 2;
	m_constantPoint = mid;
	scaleAboutPoint( pts[0], r3, mid );
	scaleAboutPoint( pts[1], r3, mid );

	// OK to do these 3D rotations because at sticker generation time, we are still only in 3D.
	CVector4D _axis = v2 - v1;
	CVector3D axis;
	axis.m_components[0] = _axis.m_components[0];
	axis.m_components[1] = _axis.m_components[1];
	axis.m_components[2] = _axis.m_components[2];

	// Calculate the centers of the adjacent faces.
	static const double rotAngle = M_PI - dihedral;
	CVector4D mid2 = ( pts[0] + pts[1] ) / 4;
	CVector4D c1 = mid - mid2;
	CVector4D c2 = c1;
	c1.rotateAboutAxis( axis, rotAngle );
	c2.rotateAboutAxis( axis, -rotAngle );
	c1 += mid2;
	c2 += mid2;
	
	scaleAboutPoint( pts[4], r2, c1 );
	scaleAboutPoint( pts[5], r2, c1 );
	scaleAboutPoint( pts[6], r2, c2 );
	scaleAboutPoint( pts[7], r2, c2 );

	SETUP_SIDE( 0, 0, 1, 5, 4 );
	SETUP_SIDE( 1, 1, 0, 6, 7 );
	SETUP_SIDE( 2, 4, 5, 3, 2 );
	SETUP_SIDE( 3, 7, 6, 2, 3 );
	SETUP_SIDE( 4, 0, 4, 2, 6 );
	SETUP_SIDE( 5, 1, 7, 3, 5 );
}

void 
SHexaHedron::generate4CSticker( const CVector4D & v1, const CVector4D & v2, 
	const SHexaHedron & h1, const SHexaHedron & h2, const SHexaHedron & h3 ) 
{
	// The hexahedrons are 3C stickers 
	// (We pass them in because they already contain point information).
	// The order of them is important and assumed in the calcs below.

	CVector4D pts[8];
	pts[0] = v1;
	pts[1] = v2;
	m_constantPoint = v1;

	bool closer;
	closer = ( h1.m_sides[0].m_a - v1 ).magSquared() < ( h1.m_sides[0].m_b - v1 ).magSquared();
	pts[2] = closer ? h1.m_sides[0].m_a : h1.m_sides[1].m_a;
	pts[3] = closer ? h1.m_sides[0].m_d : h1.m_sides[1].m_d;

	closer = ( h2.m_sides[0].m_a - v1 ).magSquared() < ( h2.m_sides[0].m_b - v1 ).magSquared();
	pts[4] = closer ? h2.m_sides[0].m_a : h2.m_sides[1].m_a;
	pts[5] = closer ? h2.m_sides[0].m_d : h2.m_sides[1].m_d;

	closer = ( h3.m_sides[0].m_a - v1 ).magSquared() < ( h3.m_sides[0].m_b - v1 ).magSquared();
	pts[6] = closer ? h3.m_sides[0].m_a : h3.m_sides[1].m_a;
	pts[7] = closer ? h3.m_sides[0].m_d : h3.m_sides[1].m_d;

	SETUP_SIDE( 0, 0, 2, 3, 4 );
	SETUP_SIDE( 1, 0, 4, 5, 6 );
	SETUP_SIDE( 2, 0, 6, 7, 2 );
	SETUP_SIDE( 3, 1, 3, 4, 5 );
	SETUP_SIDE( 4, 1, 5, 6, 7 );
	SETUP_SIDE( 5, 1, 7, 2, 3 );
}

#undef SETUP_SIDE

void 
SHexaHedron::offset( const CVector4D & amount )
{
	m_constantPoint += amount;
	for( int i=0; i<6; i++ )
		m_sides[i].offset( amount );
}

void 
SHexaHedron::scaleAboutOrigin( double factor )
{
	m_constantPoint *= factor;
	for( int i=0; i<6; i++ )
		m_sides[i].scaleAboutOrigin( factor );
}

void 
SHexaHedron::applyCellTransforms( const CVector4D & cellTransforms, bool undo ) 
{
	::applyCellTransforms( m_constantPoint, cellTransforms, undo );
	for( int i=0; i<6; i++ )
		m_sides[i].applyCellTransforms( cellTransforms, undo );
}

void 
SHexaHedron::rotateFromMatrix( double mRot[][4] ) 
{
	m_constantPoint.rotateFromMatrix( mRot );
	for( int i=0; i<6; i++ )
		m_sides[i].rotateFromMatrix( mRot );
}

void 
SHexaHedron::render()
{
	if( !getValid() )
		return;

	for( int i=0; i<6; i++ )
		m_sides[i].render();
}

bool 
SHexaHedron::doProjection( double projectionDistance )
{
	m_constantPoint.project( 4, projectionDistance );
	if( !m_constantPoint.valid() )
		return false;

	for( int i=0; i<6; i++ )
		if( !m_sides[i].doProjection( projectionDistance ) )
			return false;

	return true;
}

void 
SHexaHedron::rotateAboutAxis( CVector3D axis, double rotation ) 
{
	m_constantPoint.rotateAboutAxis( axis, rotation );
	for( int i=0; i<6; i++ )
		m_sides[i].rotateAboutAxis( axis, rotation );
}
