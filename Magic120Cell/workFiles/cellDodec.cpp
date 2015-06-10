#include <stdafx.h>
#include "CellDodec.h"

#pragma unmanaged

namespace
{
	//
	// Static items we only need to calculate once.
	//
	static bool StaticItemsCreated = false;

	// Track the stickers on faces.
	// First index is the face. Second indexes sticker ints.
	// (This is static because it is the same for every cell).
	static __int8 StickerFaceMap[12][11];

	// This is so we don't have to generate stickers more than once.
	static CellDodec StandardCell;
}

CellDodec::CellDodec()
{
	// These need to be set before generating stickers.
	m_nStickers = 63;
	m_n2DFaces = 12;
	m_nStickersPer2DFace = 11;

	CSettings dummy;
	generateStickers( dummy );
}

CellDodec &  
CellDodec::operator = ( const CellDodec & rhs ) 
{
	__super::operator = ( rhs );
	m_ringSet = rhs.m_ringSet;
	m_ring = rhs.m_ring;
	m_ringItem = rhs.m_ringItem;
	return *this;
}

void 
CellDodec::shapeChanged()
{
	// Means we need to recreate static items.
	StaticItemsCreated = false;
}

namespace
{
	void getCellTransform( bool ringSet, __int8 ring, __int8 ringItem, CVector4D & cellTransforms )
	{
		cellTransforms.m_components[0] = ringItem % 2 ? 0 : deg2rad( 36.0 );
		cellTransforms.m_components[1] = deg2rad( (double)ringItem*36.0 );
		cellTransforms.m_components[2] = ringSet ? 0 : deg2rad( 90.0 );
		cellTransforms.m_components[3] = (double)ring * deg2rad( 72.0 );
	}
}

void 
CellDodec::setupCellTransforms( bool ringSet, __int8 ring, __int8 ringItem ) 
{
	m_ringSet = ringSet;
	m_ring = ring;
	m_ringItem = ringItem;
	getCellTransform( m_ringSet, m_ring, m_ringItem, m_cellTransforms );

	// Setup our sticker position map.
	for( int s=0; s<63; s++ )
	{
		CVector4D pos = StandardCell.getSticker( s ).getConstantPoint();
		::applyCellTransforms( pos, m_cellTransforms );
		m_stickerPositionMap[pos] = s;
	}
}

void 
CellDodec::addAdjacentCell( const Cell & aCell ) 
{
	const double comparisonDist = pow( ( 1 + golden ) / 2, 2 );
	const CVector4D & cellCenter = aCell.getCenter();

	// Make it so the adjecent cells will be lined up with our faces.
	for( int f=0; f<12; f++ )
	{
		const CVector4D & faceCenter = m_bounds.m_faces[f].getCenter();
		if( IS_ZERO( (faceCenter - cellCenter).magSquared() - comparisonDist ) )
		{
			m_adjacentCells[f] = aCell.m_physicalCell;
			m_adjacentCellPositions[ aCell.getCenter() ] = f;
			return;
		}
	}

	assert( false );
}

void 
CellDodec::findAdjacentFaces( CellDodec cells[] ) 
{
	for( int f1=0; f1<12; f1++ )
	{
		assert( -1 != m_adjacentCells[f1] );
		const CVector4D & f1Center = m_bounds.m_faces[f1].getCenter();

		CellDodec & cell = cells[m_adjacentCells[f1]];
		for( int f2=0; f2<12; f2++ )
		{
			const CVector4D & f2Center = cell.m_bounds.m_faces[f2].getCenter();
			if( f1Center.compare( f2Center ) )
			{
				m_adjacentFaces[f1] = f2;
				break;
			}
		}

		assert( -1 != m_adjacentFaces[f1] );
	}
}

namespace
{
	__int8 getNumStickers( int sticker ) 
	{
		if( 0 == sticker )
		{
			return 1;
		}

		if( sticker < 13 )
		{
			return 2;
		}

		if( sticker < 43 )
		{
			return 3;
		}

		return 4;
	}
}

void 
CellDodec::findAdjacentStickers( CellDodec cells[] ) 
{
	// NOTE: We don't need to check the 1C sticker.
	for( int s=1; s<63; s++ )
	{
		__int8 num = getNumStickers( s );
		__int8 found = 1; // We already know ourselves.

		CVector4D constantPoint = getSticker( s ).getConstantPoint();

		// Cycle through all adjacent faces and stickers on those faces
		// until we find what we need.
		for( int i=0; i<12; i++ )
		{
			CellDodec & aCell = cells[m_adjacentCells[i]];
			__int8 aFace = m_adjacentFaces[i];

			for( int t=0; t<11; t++ )
			{
				int as = StickerFaceMap[aFace][t];
				if( constantPoint.compare( aCell.getSticker( as ).getConstantPoint() ) )
				{
					setConnectedSticker( s, found, getStickerHash( aCell.m_physicalCell, as ) );
					found++;

					// There can only be one per face, so stop searching.
					break;
				}
			}

			if( num == found )
				break;
		}

		assert( found == num );
	}
}

void 
CellDodec::generateStickers( const CSettings & settings ) 
{
	// XXX - Didn't figure out why, but I had to do this to get things to work.
	if( this == &StandardCell )
		return;

	// Just copy the geometry info instead of regenerating
	generateStaticItems( settings );
	copyGeometryInfo( StandardCell );
}

void 
CellDodec::getConnectedStickers( int sticker, std::vector<short> & connected ) const
{
	connected.clear();
	if( 0 == sticker )
		return;

	if( sticker < 13 )
	{
		connected.push_back( m_2Ca[sticker-1] );
		return;
	}

	if( sticker < 43 )
	{
		connected.push_back( m_3Ca[sticker-13] );
		connected.push_back( m_3Ca[sticker-13 + 30] );
		return;
	}

	connected.push_back( m_4Ca[sticker-43] );
	connected.push_back( m_4Ca[sticker-43 + 20] );
	connected.push_back( m_4Ca[sticker-43 + 40] );
}

Cell & 
CellDodec::getStandardCell() 
{
	return StandardCell;
}

void 
CellDodec::regenStickers( __int8 face ) 
{
	if( -1 != face )
	{
		CellDodec tempCell;
		for( int i=0; i<11; i++ )
		{
			__int8 sticker = getStickerIndex( face, i );
			SPrimitive3D & stickerPrim = __super::getSticker( sticker );
			stickerPrim = tempCell.getSticker( sticker );
		}
	}
}

bool 
CellDodec::isInverted() 
{
	return m_1C.isInverted();
}

const SPrimitive3D & 
CellDodec::getSticker( int sticker ) const
{
	if( 0 == sticker )
	{
		return m_1C;
	}

	if( sticker < 13 )
	{
		return m_2C[sticker-1];
	}

	if( sticker < 43 )
	{
		return m_3C[sticker-13];
	}

	return m_4C[sticker-43];
}

int 
CellDodec::getStickerIndex( int face, int indexOnFace ) 
{
	return StickerFaceMap[face][indexOnFace];
}

void 
CellDodec::setConnectedSticker( int sticker, int num, short hash ) 
{
	if( 0 == sticker )
	{
		assert( false );
		return;
	}

	if( sticker < 13 )
	{
		assert( num == 1 );
		m_2Ca[sticker-1] = hash;
		return;
	}

	if( sticker < 43 )
	{
		assert( num <= 2 );
		m_3Ca[(sticker-13)+(num-1)*30] = hash;
		return;
	}

	assert( num <= 3 );
	m_4Ca[(sticker-43)+(num-1)*20] = hash;
}

void 
CellDodec::generateStaticItems( const CSettings & settings ) 
{
	if( StaticItemsCreated )
		return;

	const double & r1 = settings.m_r1;
	const double & r2 = settings.m_r2;
	const double & r3 = settings.m_r3;

	// Central sticker and cell bounds.
	StandardCell.m_bounds = SDodecahedron();
	StandardCell.m_1C = StandardCell.m_bounds;
	StandardCell.m_1C.scaleAboutOrigin( r1 );

	// 2C stickers.
	for( int f=0; f<12; f++ )
	{
		SPentagon top = StandardCell.m_bounds.m_faces[f];
		top.scaleAboutCenter( r2 );
		StandardCell.m_2C[f].generateFrom2Pentagons( top, StandardCell.m_1C.m_faces[f] );

		// Go ahead and do this here because it is easy.
		StickerFaceMap[f][0] = f+1;
	}

#define GENERATE_3C_PIECE( f, p1, p2 ) \
		StandardCell.m_3C[idx++].generate3CSticker( StandardCell.m_bounds.m_faces[f].m_points[p1], \
			StandardCell.m_bounds.m_faces[f].m_points[p2], r1, r2, r3 );

	int idx = 0;

	GENERATE_3C_PIECE( 0, 0, 1 )
	GENERATE_3C_PIECE( 0, 1, 2 )
	GENERATE_3C_PIECE( 0, 2, 3 )
	GENERATE_3C_PIECE( 0, 3, 4 )
	GENERATE_3C_PIECE( 0, 4, 0 )
	GENERATE_3C_PIECE( 1, 0, 1 )
	GENERATE_3C_PIECE( 1, 1, 2 )
	GENERATE_3C_PIECE( 1, 2, 3 )
	GENERATE_3C_PIECE( 1, 3, 4 )
	GENERATE_3C_PIECE( 1, 4, 0 )

	// This was a pain to figure out and totally
	// depends on the method which originally generates a dodecahedron.
	// I don't like some of the ordering here, esp in the 4C pieces.
	// I'd like to reinvestigate this but maybe it doesn't matter.
	// XXX - maybe I should have just generated single items, then copied and rotated them.
	//		 This would have made it easier to generate.
	GENERATE_3C_PIECE( 2, 3, 4 )
	GENERATE_3C_PIECE( 4, 3, 4 )
	GENERATE_3C_PIECE( 6, 3, 4 )
	GENERATE_3C_PIECE( 8, 3, 4 )
	GENERATE_3C_PIECE( 10, 3, 4 )

	GENERATE_3C_PIECE( 3, 2, 3 )
	GENERATE_3C_PIECE( 5, 2, 3 )
	GENERATE_3C_PIECE( 7, 2, 3 )
	GENERATE_3C_PIECE( 9, 2, 3 )
	GENERATE_3C_PIECE( 11, 2, 3 )

	GENERATE_3C_PIECE( 2, 0, 1 )
	GENERATE_3C_PIECE( 2, 4, 0 )
	GENERATE_3C_PIECE( 4, 0, 1 )
	GENERATE_3C_PIECE( 4, 4, 0 )
	GENERATE_3C_PIECE( 6, 0, 1 )
	GENERATE_3C_PIECE( 6, 4, 0 )
	GENERATE_3C_PIECE( 8, 0, 1 )
	GENERATE_3C_PIECE( 8, 4, 0 )
	GENERATE_3C_PIECE( 10, 0, 1 )
	GENERATE_3C_PIECE( 10, 4, 0 )

#undef GENERATE_4C_PIECE

#define GENERATE_4C_PIECE( f, v, h1, h2, h3 ) \
		StandardCell.m_4C[idx++].generate4CSticker( StandardCell.m_bounds.m_faces[f].m_points[v], StandardCell.m_1C.m_faces[f].m_points[v], \
			StandardCell.m_3C[h1], StandardCell.m_3C[h2], StandardCell.m_3C[h3] );

	idx = 0;

	GENERATE_4C_PIECE( 0, 1, 0, 1, 10 )
	GENERATE_4C_PIECE( 0, 2, 1, 2, 11 )
	GENERATE_4C_PIECE( 0, 3, 2, 3, 12 )
	GENERATE_4C_PIECE( 0, 4, 3, 4, 13 )
	GENERATE_4C_PIECE( 0, 0, 4, 0, 14 )

	GENERATE_4C_PIECE( 1, 1, 5, 6, 16 )
	GENERATE_4C_PIECE( 1, 2, 6, 7, 15 )
	GENERATE_4C_PIECE( 1, 3, 7, 8, 19 )
	GENERATE_4C_PIECE( 1, 4, 8, 9, 18 )
	GENERATE_4C_PIECE( 1, 0, 9, 5, 17 )

	GENERATE_4C_PIECE( 2, 0, 20, 17, 21 )
	GENERATE_4C_PIECE( 4, 0, 22, 18, 23 )
	GENERATE_4C_PIECE( 6, 0, 24, 19, 25 )
	GENERATE_4C_PIECE( 8, 0, 26, 15, 27 )
	GENERATE_4C_PIECE( 10, 0, 28, 16, 29 )
	
	GENERATE_4C_PIECE( 2, 1, 20, 11, 23 )
	GENERATE_4C_PIECE( 4, 1, 22, 12, 25 )
	GENERATE_4C_PIECE( 6, 1, 24, 13, 27 )
	GENERATE_4C_PIECE( 8, 1, 26, 14, 29 )
	GENERATE_4C_PIECE( 10, 1, 28, 10, 21 )

#undef GENERATE_4C_PIECE

	//
	// Create our sticker face map.
	//

	// This temp variable is to track how much is filled out.
	// (We've already filled out the 2C pieces.)
	__int8 temp[12];
	for( int f=0; f<12; f++ )
		temp[f] = 1;

	for( int s=13; s<63; s++ )
	{
		CVector4D constantPoint = StandardCell.getSticker( s ).getConstantPoint();
		for( int f=0; f<12; f++ )
		{
			for( int p=0; p<5; p++ )
			{
				CVector4D & p1 = StandardCell.m_bounds.m_faces[f].m_points[p];
				CVector4D & p2 = StandardCell.m_bounds.m_faces[f].m_points[ 4== p ? 0 : p+1 ];

				if( constantPoint.compare( p1 ) ||				// 4C
					constantPoint.compare( ( p1 + p2 ) / 2 ) )	// 3C
				{
					StickerFaceMap[f][temp[f]++] = s;
				}
			}
		}
	}

	//
	// Create our sticker position map.
	//
	for( int s=0; s<63; s++ )
		StandardCell.m_stickerPositionMap[StandardCell.getSticker( s ).getConstantPoint()] = s;

	StaticItemsCreated = true;
}
