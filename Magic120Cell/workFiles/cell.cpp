#include <stdafx.h>
#include "cell.h"

#include "helper.h"
#include "puzzle.h"

#pragma unmanaged


Cell::Cell() 
{
	m_visiblePhysical = true;
	m_visibleLogical = true;
	m_changingFace = -1;

	for( int f=0; f<12; f++ )
	{
		m_adjacentCells[f] = -1;
		m_adjacentFaces[f] = -1;
	}
}

Cell & 
Cell::operator = ( const Cell & rhs )
{
	m_physicalCell = rhs.m_physicalCell;
	m_visiblePhysical = rhs.m_visiblePhysical;
	m_visibleLogical = rhs.m_visibleLogical;
	m_changingFace = rhs.m_changingFace;
	m_nStickers = rhs.m_nStickers;
	m_n2DFaces = rhs.m_n2DFaces;
	m_nStickersPer2DFace = rhs.m_nStickersPer2DFace;
	m_cellTransforms = rhs.m_cellTransforms;
	for( int i=0; i<12; i++ )
	{
		m_adjacentCells[i] = rhs.m_adjacentCells[i];	
		m_adjacentFaces[i] = rhs.m_adjacentFaces[i];
	}
	m_adjacentCellPositions = rhs.m_adjacentCellPositions;
	m_stickerPositionMap = rhs.m_stickerPositionMap;
	copyGeometryInfo( rhs );
	return *this;
}

const CVector4D & 
Cell::getCenter() const
{
	return getBounds().getCenter();
}

namespace
{
	void highlight_or_dim( bool highlight, CColor & c, bool & depthTestOn )
	{
		if( highlight )
		{
			depthTestOn = true;
		}
		else
		{
			depthTestOn = false;
			c.m_a = .05;
		}
	}

	void fade( CColor & c, double percent, bool & depthTestOn )
	{
		c.m_a *= percent;
		depthTestOn;
	}
}

void 
Cell::render( const CSettings & settings, const CState & state, bool forceVisible, 
	int highlightSticker, bool forPicking, double percentTwisted ) 
{
	// Did the projecting go ok?
	// NOTE: I found it much nicer visually if we render depending on the validity of the
	//		 central sticker instead of the validity of all pieces (the full cell validitiy).
	//if( !m_fullCell.getValid() )
	//	return;
	SPrimitive3D & sticker_1C = getSticker( 0 );
	if( !sticker_1C.getValid() )
		return;

	// Inverted?
	// NOTE: We ignore this if we are highlighting stickers, but always pay attention when picking.
	if( forPicking || !forceVisible )
	{
		if( isInverted() && !settings.m_drawInvertedCells )
			return;
	}

	// Things are easier if we are just drawing for picking.
	if( forPicking )
	{
		glEnable( GL_DEPTH_TEST );
		for( int i=0; i<m_nStickers; i++ )
		{
			glColor3b( m_physicalCell, i, 0 );
			getSticker( i ).render();
		}
		return;
	}

	// An array of our colors.
	// XXX - array sizes excessive for MS4D.
	CColor colors[63];
	bool depthTest[63];
	for( int i=0; i<m_nStickers; i++ )
		colors[i] = state.getStickerColor( m_physicalCell, i );

	// Setup the colors on every piece.
	for( int i=0; i<m_nStickers; i++ )
	{
		CColor & c = colors[i];
		bool & depthTestOn = depthTest[i];
		
		// -2 forces the full cell to dim.
		if( -2 == highlightSticker )
		{
			highlight_or_dim( false, c, depthTestOn );
		}
		else if( -1 != highlightSticker )
		{
			highlight_or_dim( highlightSticker == i, c, depthTestOn );
		}
		else
		{
			// NOTE - if I don't dim with colors, this could be improved performance-wise.
			//		 (highlight_or_dim would only need to be called once for every piece 
			//		 type instead of per-sticker). It seems to be fast enough though.
			if( 0 == i )
				highlight_or_dim( settings.m_highlight1C, c, depthTestOn );
			else if( i < 13 )
				highlight_or_dim( settings.m_highlight2C, c, depthTestOn );
			else if( i < 43 )
				highlight_or_dim( settings.m_highlight3C, c, depthTestOn );
			else
				highlight_or_dim( settings.m_highlight4C, c, depthTestOn );
		}
	}

	// Do we need to apply fading to a face?
	if( -1 != m_changingFace )
	{
		double percent = isVisible() ? 1 - percentTwisted : percentTwisted;
		for( int i=0; i<m_nStickersPer2DFace; i++ )
		{
			int s = getStickerIndex( m_changingFace, i );
			CColor & c = colors[s];
			fade( c, percent, depthTest[s] );
		}
	}

	// Now render.
	if( isVisible() )
	{
		for( int i=0; i<m_nStickers; i++ )
		{
			depthTest[i] ? glEnable( GL_DEPTH_TEST ) : glDisable( GL_DEPTH_TEST );
			setupColor( colors[i] );
			getSticker( i ).render();
		}
	}
	else
	{
		// We should only make it here if we had a changing face
		// or if there were stickers to highlight.
		assert( -1 != m_changingFace || ( -1 != highlightSticker && -2 != highlightSticker ) );
		if( -1 != m_changingFace )
		{
			for( int i=0; i<m_nStickersPer2DFace; i++ )
			{
				int s = getStickerIndex( m_changingFace, i );
				depthTest[s] ? glEnable( GL_DEPTH_TEST ) : glDisable( GL_DEPTH_TEST );
				setupColor( colors[s] );
				getSticker( s ).render();
			}
		}
		else
		{
			int s = highlightSticker;
			depthTest[s] ? glEnable( GL_DEPTH_TEST ) : glDisable( GL_DEPTH_TEST );
			setupColor( colors[s] );
			getSticker( s ).render();
		}
	}
}

namespace
{
	template <class T>
	void applySettingTransforms( T & item, const CVector4D & cellCenter, const CSettings & settings )
	{
		//
		// Apply cellSize, stickerSize, and cellDistance transformations.
		//

		// Move to center.
		item.offset( cellCenter * -1 );

		// Move to constant point.
		CVector4D constantStickerPoint = item.getConstantPoint();
		item.offset( constantStickerPoint * -1 );
		assert( settings.m_stickerSize >= 0 && settings.m_stickerSize <= 1 );
		item.scaleAboutOrigin( settings.m_stickerSize );
		item.offset( constantStickerPoint );

		// Scale.
		assert( settings.m_cellSize >= 0 && settings.m_cellSize <= 1 );
		item.scaleAboutOrigin( settings.m_cellSize );

		// Move back.
		item.offset( cellCenter );

		// Get distance offset.
		assert( settings.m_cellDistance >= 1 );
		CVector4D center( cellCenter );
		center *= settings.m_cellDistance;
		CVector4D offset = center - cellCenter;

		// Apply the offset.
		item.offset( offset );

		// Do the projection and track if it went ok.
		item.setValid( item.doProjection( settings.m_projection4Distance ) );
	}
}

void 
Cell::twist( const STwist & twist, double rotation, 
	const CSettings & settings, CPuzzle * pPuzzle, bool calculateNewState, 
	bool updateState, CState & state, double mViewRot[][4] ) 
{
	//
	// Do the twist on us.
	//

	// Regen, then twist.
	generateStickers( settings );

	const int & ts = twist.m_sticker;

	// Get the twist axis
	CVector3D axis;	
	assert( 0 != ts );
	const CVector4D & _axis = getSticker( ts ).getConstantPoint();
	axis.m_components[0] = _axis.m_components[0];
	axis.m_components[1] = _axis.m_components[1];
	axis.m_components[2] = _axis.m_components[2];

	// Handle direction.
	if( twist.m_leftClick )
		rotation *= -1;

	// Transform every sticker.
	for( int i=0; i<m_nStickers; i++ )
		getSticker( i ).rotateAboutAxis( axis, rotation );

	// Update the state?
	if( calculateNewState )
	{
		VecToIntMapIterator it;
		for( int i=0; i<63; i++ )
		{
			it = getStandardCell().m_stickerPositionMap.find( getSticker( i ).getConstantPoint() );
			assert( it != getStandardCell().m_stickerPositionMap.end() );
			int newPosition = it->second;
			state.setStickerColorIndex( m_physicalCell, newPosition, 
				state.getStickerColorIndex( m_physicalCell, i ) );
		}
	}

	// We need to reapply our transforms.
	applyCellTransforms();
	applyViewTransforms( mViewRot );
	applySettingTransforms( settings );

	//
	// Apply this twist to the relevant parts of adjacent cells.
	//

	// Now transform all the stickers on the correct faces of every adjacent cell.
	for( int i=0; i<m_n2DFaces; i++ )
	{
		//
		// This (now short) block of code was so difficult to figure out
		// (though after ironing out various bugs and cleaning it up,
		// in retrospect it seems like it shouldn't have been).
		// But I stayed up waaaay late working on it and lost sleep!!!!
		//

		Cell & aCell = pPuzzle->getCell( m_adjacentCells[i] );
		__int8 aFace = m_adjacentFaces[i];
	
		// We also need to track the unprojected center of the face to do the setting transforms.
		// XXX - Perhaps don't make a whole dodecahedron here.
		SDodecahedron unProjectedCenter;

		aCell.regenStickers( aFace );
		aCell.applyCellTransforms( aFace, aCell.m_cellTransforms, false );
		unProjectedCenter.applyCellTransforms( aCell.m_cellTransforms, false );
		
		// Rotate (we need to move adjacent to parent cell, do the rotation, then move back).
		aCell.applyCellTransforms( aFace, m_cellTransforms, true );
		aCell.rotateAboutAxis( aFace, axis, rotation );
		aCell.applyCellTransforms( aFace, m_cellTransforms, false );
		unProjectedCenter.applyCellTransforms( m_cellTransforms, true );
		unProjectedCenter.rotateAboutAxis( axis, rotation );
		unProjectedCenter.applyCellTransforms( m_cellTransforms, false );
		
		if( calculateNewState )
		{
			// Which cell did this face go to?
			VecToIntMapIterator it = m_adjacentCellPositions.find( unProjectedCenter.getCenter() );
			assert( it != m_adjacentCellPositions.end() );
			__int8 newCellInt = m_adjacentCells[ it->second ];
			__int8 newFace = m_adjacentFaces[ it->second ];
			Cell & newCell = pPuzzle->getCell( newCellInt );

			// Track if this means this visibility will change.
			// XXX - isVisible method doesn't currently deal with inverted cells.
			//		 so twist fading isn't working for that.
			if( ( aCell.isVisible() && !newCell.isVisible() ) ||
				( !aCell.isVisible() && newCell.isVisible() ) )
			{
				aCell.m_changingFace = aFace;
			}

			// Now update the stickers on the new cell/face.			
			for( int i=0; i<m_nStickersPer2DFace; i++ )
			{
				int faceStickerIndex = getStickerIndex( aFace, i );
				it = newCell.m_stickerPositionMap.find( aCell.getSticker( faceStickerIndex ).getConstantPoint() );
				assert( it != newCell.m_stickerPositionMap.end() );
				int newPosition = it->second;
				state.setStickerColorIndex( newCell.m_physicalCell, newPosition, 
					state.getStickerColorIndex( aCell.m_physicalCell, faceStickerIndex ) );
			}
		}

		unProjectedCenter.rotateFromMatrix( mViewRot );
		aCell.applyViewTransforms( aFace, mViewRot );

		// Now we can project.
		CVector4D cellCenter = unProjectedCenter.getCenter();		// XXX - same problem where this copy was required.
		::applySettingTransforms( unProjectedCenter, cellCenter, settings );
		aCell.applySettingTransforms( aFace, cellCenter, settings );

		// Mark stickers invalid that we won't want to draw.
		if( !unProjectedCenter.getValid() || ( unProjectedCenter.isInverted() && !settings.m_drawInvertedCells ) )
		{
			for( int i=0; i<m_nStickersPer2DFace; i++ )
				aCell.getSticker( getStickerIndex( aFace, i ) ).setValid( false );
		}
	}

	if( updateState )
	{
		state.commitChanges( m_physicalCell );
		state.commitChanges( m_adjacentCells, m_n2DFaces );
	}
}

void 
Cell::applyCellTransforms() 
{
	applyCellTransforms( m_cellTransforms, false );
}

void 
Cell::applyCellTransforms( const CVector4D & cellTransforms, bool undo ) 
{
	getBounds().applyCellTransforms( cellTransforms, undo );
	for( int i=0; i<m_nStickers; i++ )
		getSticker( i ).applyCellTransforms( cellTransforms, undo );
}

void 
Cell::applyViewTransforms( double mRot[][4] )
{
	getBounds().rotateFromMatrix( mRot );
	for( int i=0; i<m_nStickers; i++ )
		getSticker( i ).rotateFromMatrix( mRot );
}

void 
Cell::applySettingTransforms( const CSettings & settings )
{
	// Go ahead and do the projection and all that.
	// XXX - not sure why this was required (using a reference instead of a copy caused problems
	//		 with the dodecahedron primitive).  Investigate.
	CVector4D cellCenter( getCenter() );
	::applySettingTransforms( getBounds(), cellCenter, settings );

	// Transform every sticker.
	for( int i=0; i<m_nStickers; i++ )
		::applySettingTransforms( getSticker( i ), cellCenter, settings );
}

void 
Cell::applyCellTransforms( __int8 face, const CVector4D & transforms, bool undo ) 
{
	for( int i=0; i<m_nStickersPer2DFace; i++ )
		getSticker( getStickerIndex( face, i ) ).applyCellTransforms( transforms, undo );
}

void 
Cell::applyViewTransforms( __int8 face, double mRot[][4] ) 
{
	for( int i=0; i<m_nStickersPer2DFace; i++ )
		getSticker( getStickerIndex( face, i ) ).rotateFromMatrix( mRot );
}

void 
Cell::applySettingTransforms( __int8 face, const CVector4D & cellCenter, const CSettings & settings ) 
{
	for( int i=0; i<m_nStickersPer2DFace; i++ )
		::applySettingTransforms( getSticker( getStickerIndex( face, i ) ), cellCenter, settings );
}

void 
Cell::rotateAboutAxis( __int8 face, CVector3D axis, double rotation ) 
{
	for( int i=0; i<m_nStickersPer2DFace; i++ )
		getSticker( getStickerIndex( face, i ) ).rotateAboutAxis( axis, rotation );
}

void 
Cell::copyGeometryInfo( const Cell & source )
{
	getBounds() = source.getBounds();
	for( int i=0; i<m_nStickers; i++ )
		getSticker( i ) = source.getSticker( i );
}

// See http://stackoverflow.com/questions/123758/how-do-i-remove-code-duplication-between-similar-const-and-non-const-member-funct
// for the motivation behind this ugliness.
SPrimitive3D & 
Cell::getBounds() 
{
	return const_cast<SPrimitive3D&>( static_cast<const Cell&>(*this).getBounds() );
}

SPrimitive3D & 
Cell::getSticker( int sticker ) 
{
	return const_cast<SPrimitive3D&>( static_cast<const Cell&>(*this).getSticker( sticker ) );
}
