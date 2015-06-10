#include <stdafx.h>
#include "magic120cell.h"

#pragma unmanaged
#pragma warning(disable:4996)


CMagic120Cell::CMagic120Cell() : CPuzzle( 120, 63 )
{
	generateCells();
	fillLayerMap( m_layerMap );
	fillLayerMap( m_layerMapLogical );
	fillHypercubeMap();
	fillAdjacentInfo();
	generate4Cube();
}

bool 
CMagic120Cell::isCellVisible( int i )
{
	switch( m_settings.m_symmetry )
	{
	case 0:
		{
			// Track which torus we are on.
			int torus = i/60;

			// Is this torus visible?
			if( ! m_settings.m_visibility[torus] )
				return false;

			break;
		}
	case 1:
		{
			std::map<__int8,__int8>::iterator map_iterator;
			map_iterator = m_hypercubeMap.find( i );
			if( map_iterator == m_hypercubeMap.end() )
			{
				assert( false );
				return false;
			}
			int cube = map_iterator->second;

			if( ! m_settings.m_visibility[cube] )
				return false;

			break;
		}
	case 2:
		{
			std::map<__int8,__int8>::iterator map_iterator;
			map_iterator = m_layerMap.find( i );
			if( map_iterator == m_layerMap.end() )
			{
				assert( false );
				return false;
			}
			int layer = map_iterator->second;

			if( ! m_settings.m_visibility[layer] )
				return false;

			break;
		}
	case 3:
		{
			// Track which ring we are on.
			int ring = i/10;

			// Is this ring visible?
			if( ! m_settings.m_visibility[ring] )
				return false;

			break;
		}
	}

	return true;
}

void 
CMagic120Cell::render( const CVector3D & lookFrom, bool forPicking ) 
{
	__super::render( lookFrom, forPicking );

	// Draw hypercube cells?
	if( 1 == m_settings.m_symmetry && 
		m_settings.m_visibility[9] && !forPicking )
	{
		ambientOnly();
		CColor & c = m_settings.m_colors[9];
		glColor4d( c.m_r, c.m_g, c.m_b, c.m_a );
		draw4Cube( m_hypercubePoints );
	}

	/* Draw our axes.
	glPointSize( 5.0 );
	glBegin( GL_POINTS );
		glColor4d( 1,1,1,1 );
		vertex( CVector4D( 0,0,0,0 ) );
		glColor4d( 1,0,0,1 );
		vertex( CVector4D( 3,0,0,0 ) );
		glColor4d( 0,1,0,1 );
		vertex( CVector4D( 0,3,0,0 ) );
		glColor4d( 0,0,1,1 );
		vertex( CVector4D( 0,0,3,0 ) );
	glEnd();
	*/
}

void 
CMagic120Cell::generate4Cube() 
{
	// XXX - Clean up this code and make it shorter.
	// WOW! golden ratio shows up everywhere.
	const double c = standardFaceOffset /* * m_settings.m_cellDistance*/ / golden;

	m_hypercubePoints[0] = CVector4D(  -c, c, -c, c );
	m_hypercubePoints[1] = CVector4D(  -c, c, c, c );
	m_hypercubePoints[2] = CVector4D(  c, c, c, c );
	m_hypercubePoints[3] = CVector4D(  c, c, -c, c );

	m_hypercubePoints[4] = CVector4D(  -c, -c, -c, c );
	m_hypercubePoints[5] = CVector4D(  -c, -c, c, c );
	m_hypercubePoints[6] = CVector4D(  c, -c, c, c );
	m_hypercubePoints[7] = CVector4D(  c, -c, -c, c );

	m_hypercubePoints[8] = CVector4D(  -c, c, -c, -c );
	m_hypercubePoints[9] = CVector4D(  -c, c, c, -c );
	m_hypercubePoints[10] = CVector4D(  c, c, c, -c );
	m_hypercubePoints[11] = CVector4D(  c, c, -c, -c );

	m_hypercubePoints[12] = CVector4D(  -c, -c, -c, -c );
	m_hypercubePoints[13] = CVector4D(  -c, -c, c, -c );
	m_hypercubePoints[14] = CVector4D(  c, -c, c, -c );
	m_hypercubePoints[15] = CVector4D(  c, -c, -c, -c );

	// Same as 4 above but switch x/y and z/w values
	m_hypercubePoints[16] = CVector4D(  c, -c, c, -c );
	m_hypercubePoints[17] = CVector4D(  c, -c, c, c );
	m_hypercubePoints[18] = CVector4D(  c, c, c, c );
	m_hypercubePoints[19] = CVector4D(  c, c, c, -c );

	m_hypercubePoints[20] = CVector4D(  -c, -c, c, -c );
	m_hypercubePoints[21] = CVector4D(  -c, -c, c, c );
	m_hypercubePoints[22] = CVector4D(  -c, c, c, c );
	m_hypercubePoints[23] = CVector4D(  -c, c, c, -c );

	m_hypercubePoints[24] = CVector4D(  c, -c, -c, -c );
	m_hypercubePoints[25] = CVector4D(  c, -c, -c, c );
	m_hypercubePoints[26] = CVector4D(  c, c, -c, c );
	m_hypercubePoints[27] = CVector4D(  c, c, -c, -c );

	m_hypercubePoints[28] = CVector4D(  -c, -c, -c, -c );
	m_hypercubePoints[29] = CVector4D(  -c, -c, -c, c );
	m_hypercubePoints[30] = CVector4D(  -c, c, -c, c );
	m_hypercubePoints[31] = CVector4D(  -c, c, -c, -c );

	double angle = -dihedral/2;
	for( int i=0; i<32; i++ )
		m_hypercubePoints[i].rotate( 0, 2, angle );

	for( int i=0; i<32; i++ )
		m_hypercubePointsCopy[i] = m_hypercubePoints[i];
}

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

}

void 
CMagic120Cell::draw4Cube( CVector4D points[32] ) const
{
	// We need to work with a copy since we are projecting.
	CVector4D copy[32];
	for( int i=0; i<32; i++ )
	{
		copy[i] = points[i];
		copy[i].project( 4, m_settings.m_projection4Distance );
	}

	for( int i=0; i<8; i++ )
	{
		glBegin( GL_LINE_LOOP );
			for( int j=i*4; j<i*4+4; j++ )
				vertex( copy[j] );
		glEnd();
	}
}

void 
CMagic120Cell::regenStandardCell() 
{
	CellDodec::shapeChanged();
}

void 
CMagic120Cell::physicalVisibilityChanged()
{
	// Update all the cell visibilities.
	for( int c=0; c<m_nCells; c++ )
		m_cells[c].m_visiblePhysical = m_cellsUnprojected[c].m_visiblePhysical = isCellVisible( c );
}

void 
CMagic120Cell::logicalVisibilityChanged() 
{
	for( int c=0; c<m_nCells; c++ )
	{
		// Logical layer vis settings. 
		std::map<__int8,__int8>::iterator map_iterator;
		map_iterator = m_layerMapLogical.find( c );
		if( map_iterator == m_layerMapLogical.end() )
		{
			assert( false );
			m_cells[c].m_visibleLogical = false;
			continue;
		}
		int layer = map_iterator->second;
		if( layer+1 > m_settings.m_logicalVisibility )
			m_cells[c].m_visibleLogical = false;
		else
			m_cells[c].m_visibleLogical = true;
	}
}

void 
CMagic120Cell::puzzleChanged() 
{
	// XXX - make this an enumeration
	if( 4 == m_settings.m_puzzle )
	{
		// The state sets this up herself.
		m_state.setupFullColors();
	}

	// Antipodal
	if( 5 == m_settings.m_puzzle )
	{
		// The state sets this up herself.
		m_state.setupAntipodalColors();
	}

	int ring = 0, torus = 0;
	for( int i=0; i<m_nCells; i++ )
	{
		switch( m_settings.m_puzzle )
		{
		case 0:
			{
				// Track which ring we are on.
				if( 0 == i%10 )
					ring++;
				m_state.setCellColor( i, m_settings.m_colors[ring-1] );
				break;
			}
		case 1:
			{
				std::map<__int8,__int8>::iterator map_iterator;
				map_iterator = m_layerMap.find( i );
				if( map_iterator == m_layerMap.end() )
				{
					assert( false );
					continue;
				}
				int layer = map_iterator->second;
				m_state.setCellColor( i, m_settings.m_colors[layer] );
				break;
			}
		case 2:
			{
				// Track which torus we are on.
				if( 0 == i%60 )
					torus++;
				m_state.setCellColor( i, m_settings.m_colors[torus-1] );
				break;
			}
		case 3:
			{
				std::map<__int8,__int8>::iterator map_iterator;
				map_iterator = m_hypercubeMap.find( i );
				if( map_iterator == m_hypercubeMap.end() )
				{
					assert( false );
					continue;
				}
				int cube = map_iterator->second;
				m_state.setCellColor( i, m_settings.m_colors[cube] );
				break;
			}
		}
	}
}

void 
CMagic120Cell::setPuzzleType( int type ) 
{
	m_settings.m_puzzle = type;
	// See 4D_Cubing post 9-17-08 about this.
	//reset();
	puzzleChanged();
}

double 
CMagic120Cell::getCurrentTwistMagnitude() const
{
	const STwist & twist = m_currentTwist;

	if( twist.m_viewRotation )
		return twist.m_viewRotationMagnitude;

	int s = twist.m_sticker;
	assert( 0 != s );

	// There are only 3 valid values here, 72 degrees, 180 degrees, 120 degrees.
	if( s < 13 )
	{
		return deg2rad( 72.0 );
	}

	if( s < 43 )
	{
		return deg2rad( 180.0 );
	}

	return deg2rad( 120.0 );
}

namespace
{

bool 
setupViewRotation( STwist & twist, Cell & newCell, Cell & oldCell ) 
{
	twist.m_viewRotation = true;
	twist.m_viewRotationP1 = oldCell.getCenter();
	twist.m_viewRotationP2 = newCell.getCenter();
	const CVector4D & p1 = twist.m_viewRotationP1;
	const CVector4D & p2 = twist.m_viewRotationP2;

	// Don't allow view rotations from the cell already in the center.
	if( p1.compare( p2 ) )
	{
		twist.m_viewRotationMagnitude = 0;
		return false;
	}

	// XXX - deal with antipodal cells.
	// For now, just don't allow
	// Plane to rotate through is defined by origin, p1, p2 and this is not unique.
	if( p1.compare( p2 * -1 ) )
	{
		twist.m_viewRotationMagnitude = 0;
		return false;
	}

	twist.m_cell = newCell.m_physicalCell;
	twist.m_viewRotationMagnitude = p1.angleTo( p2 );
	if( !twist.m_leftClick )
	{
		// Swap the points.
		CVector4D t = twist.m_viewRotationP1;
		twist.m_viewRotationP1 = twist.m_viewRotationP2;
		twist.m_viewRotationP2 = t;
	}
	return true;
}

}

bool 
CMagic120Cell::calcViewTwist( int clickedCell, bool leftClick, STwist & twist )
{
	CSettings & settings = getSettings();

	int newCellIndex = clickedCell;
	int oldCellIndex = settings.m_centerCell;
	Cell & newCell = m_cellsUnprojected[newCellIndex];
	Cell & oldCell = m_cellsUnprojected[oldCellIndex];

	twist.m_leftClick = leftClick;
	if( setupViewRotation( twist, newCell, oldCell ) )
	{
		if( leftClick )
			settings.m_centerCell = newCellIndex;
		else
		{
			// We have to search for the new center.
			// This was so difficult to figure out!!!  God, it sucked.
			// I would have thought the oldCell would always be at 0,0,0,-1 after
			// the current view rotations, and that for this reason we wouldn't
			// have to apply the current view rotations below, but that wasn't true.
			CVector4D newCenter = m_cellsUnprojected[oldCellIndex].getCenter();
			CMatrix4D tempRot;
			twist.getViewRotationMatrix( twist.m_viewRotationMagnitude*-1, tempRot );
			CMatrix4D R = m_currentViewRotation * tempRot;
			newCenter.rotateFromMatrix( R.m );

			settings.m_centerCell = findCell( newCenter );
		}
		
		return true;
	}

	return false;
}

void 
CMagic120Cell::scramble( int numTwists ) 
{
	// If we are doing a full scramble, we load from a presaved file if we can.
	// This is because it was way too slow and avoid having to optimize.
	// Also, it was Sarah's idea!
	bool fullScramble = numTwists >= 1000;
	if( fullScramble )
	{
		// Always clear twists for a full scramble.
		m_twistHistory.clear();

		if( m_loader.loadScrambledFile( m_state, m_twistHistory.getAllTwists() ) )
		{
			m_state.setScrambled( true );
			puzzleChanged();
			return;
		}

		assert( false );
	}

	for( int i=0; i<numTwists; i++ )
	{
		bool left = getRandomInt( 1 ) == 1;
		__int8 cell = getRandomInt( m_nCells-1 );
		__int8 sticker = getRandomInt( m_nStickers-2 ) + 1;	// Avoids 0 sticker.
		m_currentTwist = STwist( getStickerHash( cell, sticker ), left );

		// Apply the twist.
		m_cells[m_currentTwist.m_cell].twist( m_currentTwist, getCurrentTwistMagnitude(), 
			m_settings, this, true, true, m_state, m_currentViewRotation.m );
		m_twistHistory.update( m_currentTwist );
	}
	
	// Cache the full scramble in case it disappeared for some reason.
	if( fullScramble )
	{
		assert( false );
		m_state.setScrambled( true );
		m_loader.saveScrambledFile( m_state, m_twistHistory.getAllTwists() );
	}

	// We do this so everything will be redrawn.
	// XXX - better name.
	settingsChanged();

	// Reset the fading information.
	for( int c=0; c<m_nCells; c++ )
		m_cells[c].m_changingFace = -1;
}

void 
CMagic120Cell::resetView()
{
	__super::resetView();
	m_settings.m_centerCell = 5;
	m_layerMapLogical = m_layerMap;
}

int 
CMagic120Cell::findCell( const CVector4D & center )
{
	for( int c=0; c<m_nCells; c++ )
	{
		CVector4D cellCenter = m_cellsUnprojected[c].getCenter();
		cellCenter.rotateFromMatrix( m_currentViewRotation.m );
		if( cellCenter.compare( center ) )
			return c;
	}

	assert( false );
	return -1;
}

void 
CMagic120Cell::generateCells() 
{
	// Track which dodecahedral face we are generating.
	int cell = 0;

	// There are 2 main sets of rings.
	for( int s=0; s<2; s++ )
	{
		bool ringSet2 = s % 2 ? true : false;

		// Generate one set of six rings.
		for( int r=0; r<6; r++ )
		{
			// Generate 1 ring.
			for( int i=0; i<10; i++ )
			{
				m_cells[cell].setupCellTransforms( !ringSet2, r, i );
				m_cells[cell].applyCellTransforms();

				m_cells[cell].m_physicalCell = cell;
				cell++;
			}
		}
	}

	assert( m_nCells == cell );
}

void 
CMagic120Cell::applyViewAndSettingTransforms()
{
	// Calc the view transforms.
	CMatrix4D R;
	if( m_currentTwist.m_viewRotation )
	{
		const double rotation = getSmoothedRotation( m_rotation, getCurrentTwistMagnitude() );
		CMatrix4D tempRot;
		m_currentTwist.getViewRotationMatrix( rotation, tempRot );
		R = m_currentViewRotation * tempRot;
	}

	for( int i=0; i<m_nCells; i++ )
	{
		if( m_currentTwist.m_viewRotation )
		{
			m_cells[i].applyViewTransforms( R.m );

			// We need to update the hypercube points as well.
			for( int i=0; i<32; i++ )
			{
				m_hypercubePoints[i] = m_hypercubePointsCopy[i];
				m_hypercubePoints[i].rotateFromMatrix( R.m );
			}
		}
		else
			m_cells[i].applyViewTransforms( m_currentViewRotation.m );
	}

	fillLayerMap( m_layerMapLogical );
	logicalVisibilityChanged();

	// Now the setting transforms.
	for( int i=0; i<m_nCells; i++ )
		m_cells[i].applySettingTransforms( m_settings );
}

typedef std::pair<__int8,__int8> int_pair;

void 
CMagic120Cell::fillLayerMap( std::map<__int8,__int8> & layerMap ) const
{
	layerMap.clear();

	// The way we generated the 120 cell, we are organized by rings.
	// This is how we track what layers cells belong to.
	//
	// We can track layers by distance to a given face.
	// The magSquared values come out to:
	//
	//	 0
	//	 6.199
	//	16.231
	//	22.430
	//	32.461
	//	42.493
	//	48.692
	//	58.723
	//	64.923

	CVector4D top( 0, 0, 0, -standardFaceOffset );

	for( int i=0; i<m_nCells; i++ )
	{
		double magSquared = ( m_cells[i].getCenter() - top ).magSquared();

		if( magSquared < 1 )
			layerMap.insert( int_pair( i, 0 ) );
		else if( magSquared < 7 )
			layerMap.insert( int_pair( i, 1 ) );
		else if( magSquared < 17 )
			layerMap.insert( int_pair( i, 2 ) );
		else if( magSquared < 23 )
			layerMap.insert( int_pair( i, 3 ) );
		else if( magSquared < 33 )
			layerMap.insert( int_pair( i, 4 ) );
		else if( magSquared < 43 )
			layerMap.insert( int_pair( i, 5 ) );
		else if( magSquared < 49 )
			layerMap.insert( int_pair( i, 6 ) );
		else if( magSquared < 59 )
			layerMap.insert( int_pair( i, 7 ) );
		else if( magSquared < 65 )
			layerMap.insert( int_pair( i, 8 ) );
		else
			assert( false );
	}
}

void 
CMagic120Cell::fillHypercubeMap() 
{
	// Generate the 8 cube face centers.
	// The last rotation is required because of how we initially orient our 120-cell.
	double angle = -dihedral/2;
	CVector4D faces[8];
	faces[0] = CVector4D(  standardFaceOffset, 0, 0, 0 ); faces[0].rotate( 0, 2, angle );
	faces[1] = CVector4D( -standardFaceOffset, 0, 0, 0 ); faces[1].rotate( 0, 2, angle );
	faces[2] = CVector4D( 0,  standardFaceOffset, 0, 0 ); faces[2].rotate( 0, 2, angle );
	faces[3] = CVector4D( 0, -standardFaceOffset, 0, 0 ); faces[3].rotate( 0, 2, angle );
	faces[4] = CVector4D( 0, 0,  standardFaceOffset, 0 ); faces[4].rotate( 0, 2, angle );
	faces[5] = CVector4D( 0, 0, -standardFaceOffset, 0 ); faces[5].rotate( 0, 2, angle );
	faces[6] = CVector4D( 0, 0, 0,  standardFaceOffset ); faces[6].rotate( 0, 2, angle );
	faces[7] = CVector4D( 0, 0, 0, -standardFaceOffset ); faces[7].rotate( 0, 2, angle );

	int count = 0;
	for( int f=0; f<8; f++ )
	{
		for( int i=0; i<m_nCells; i++ )
		{
			double magSquared = ( m_cells[i].getCenter() - faces[f] ).magSquared();
			if( magSquared < 7 )
			{
				m_hypercubeMap.insert( int_pair( i, f ) );
				count++;
			}
		}
	}
	assert( count == m_nCells - 16 );

	// There are 16 left.  Put them in the last layer.
	for( int i=0; i<m_nCells; i++ )
	{
		std::map<__int8,__int8>::iterator map_iterator;
		map_iterator = m_hypercubeMap.find( i );
		if( map_iterator == m_hypercubeMap.end() )
			m_hypercubeMap.insert( int_pair( i, 8 ) );
	}
}

void 
CMagic120Cell::fillAdjacentInfo() 
{
	// Adjacent cells.
	for( int c1=0; c1<m_nCells; c1++ )
	{
		for( int c2=c1+1; c2<m_nCells; c2++ )
		{
			double magSquared = ( m_cells[c1].getCenter() - m_cells[c2].getCenter() ).magSquared();
			if( magSquared < 7 )
			{
				m_cells[c1].addAdjacentCell( m_cells[c2] );
				m_cells[c2].addAdjacentCell( m_cells[c1] );
			}
		}
	}

	// Adjacent info.
	for( int c=0; c<m_nCells; c++ )
	{
		m_cells[c].findAdjacentFaces( m_cells );
		m_cells[c].findAdjacentStickers( m_cells );
	}

	// We can now save a copy.
	std::copy( &m_cells[0], &m_cells[m_nCells], &m_cellsUnprojected[0] );
}