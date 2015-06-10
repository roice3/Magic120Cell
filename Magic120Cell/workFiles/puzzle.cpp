#include <stdafx.h>
#include "puzzle.h"


#pragma unmanaged


CPuzzle::CPuzzle( int nCells, int nStickers ) :
	m_state( nCells, nStickers )
{
	m_nCells = nCells;
	m_nStickers = nStickers;
	m_rotating = false;
	m_rotation = 0;
	m_settingsChanged = false;
	m_shapeChanged = false;
	m_solvedMessageShown = false;
	m_allowSolvedMessage = true;
	m_currentViewRotation.setIdentity();

	clearStickersToHighlight();
}

void 
CPuzzle::render( const CVector3D & lookFrom, bool forPicking ) 
{
	if( m_shapeChanged )
	{
		// We need to regen the stickers.
		regenStandardCell();
		for( int c=0; c<m_nCells; c++ )
		{
			getCellUnprojected( c ).generateStickers( m_settings );
			getCellUnprojected( c ).applyCellTransforms();
		}

		// XXX - better name!
		// This means we need to reproject and all that.
		settingsChanged();
		m_shapeChanged = false;
	}

	if( m_settingsChanged )
	{
		// Use the copy of the cells that haven't been transformed by settings.
		for( int c=0; c<m_nCells; c++ )
			getCell( c ) = getCellUnprojected( c );

		applyViewAndSettingTransforms();
		m_settingsChanged = false;
	}

	// Setup our lighting.
	setupLighting( lookFrom, forPicking );

	// Calclate how much we're twisted for fading.
	double percentTwisted = 0;
	if( !m_currentTwist.m_viewRotation && -1 != m_currentTwist.m_cell )
		percentTwisted = m_rotation / getCurrentTwistMagnitude();

	//
	// Now draw all the cells.
	// We do this in two passes - a poor man's handling of alpha ordering which is still not perfect
	//

	// Draw all our faces except those not visible or being blended.
	for( int i=0; i<m_nCells; i++ )
	{
		Cell & cell = getCell( i );

		// Force visible?
		bool forceVisible = -1 != m_stickersToHighlight[i] && -2 != m_stickersToHighlight[i];
		if( !forceVisible )
		{
			if( !cell.isVisible() || -1 != cell.m_changingFace )
				continue;
		}

		cell.render( m_settings, m_state, forceVisible, m_stickersToHighlight[i], 
			forPicking, percentTwisted );
	}

	// Draw ones with changing faces now.
	for( int i=0; i<m_nCells; i++ )
	{
		Cell & cell = getCell( i );

		if( -1 == cell.m_changingFace )
			continue;

		cell.render( m_settings, m_state, false, m_stickersToHighlight[i], 
			forPicking, percentTwisted );
	}
}

void 
CPuzzle::save( bool saveas ) 
{
	m_loader.saveToFile( m_settings.m_puzzle, m_state, m_twistHistory.getAllTwists(), saveas );
}

void 
CPuzzle::load() 
{
	m_loader.loadFromFile( m_settings.m_puzzle, m_state, m_twistHistory.getAllTwists() );

	// Don't allow a solved message to be shown if the user is just loading a file with a completed solution!
	if( m_state.getScrambled() && m_state.isSolved() )
		m_allowSolvedMessage = false;
	else
		m_allowSolvedMessage = true;

	// Just in case this happened.
	puzzleChanged();

	// XXX - we need to update the selected puzzle type in the UI
	// (This is also a bug in MC5D, so fix there too).
}

void 
CPuzzle::highlightSticker( int stickerHash, bool shiftOnly, bool leftClick ) 
{
	std::vector<int> stickersToHighlight;
	int cell, sticker;
	decodeStickerHash( stickerHash, cell, sticker );
	std::vector<short> connected;
	getCell( cell ).getConnectedStickers( sticker, connected );

	if( shiftOnly )
	{
		stickersToHighlight.push_back( stickerHash );
		for( uint i=0; i<connected.size(); i++ )
			stickersToHighlight.push_back( connected[i] );
	}
	else if( leftClick )
	{
		// This code highlights the centers of the eventual home.
		int colorIndex = m_state.getStickerColorIndex( cell, sticker );
		stickersToHighlight.push_back( getStickerHash( colorIndex, 0 ) );

		for( uint i=0; i<connected.size(); i++ )
		{
			int tCell, tSticker;
			decodeStickerHash( connected[i], tCell, tSticker );
			colorIndex = m_state.getStickerColorIndex( tCell, tSticker );
			stickersToHighlight.push_back( getStickerHash( colorIndex, 0 ) );
		}
	}
	else
	{
		std::vector<__int8> colors;
		for( uint i=0; i<connected.size(); i++ )
		{
			int tCell, tSticker;
			decodeStickerHash( connected[i], tCell, tSticker );
			colors.push_back( m_state.getStickerColorIndex( tCell, 0 ) );
		}

		// Add in ourself.
		colors.push_back( m_state.getStickerColorIndex( cell, 0 ) );

		// Now we need to go find the piece with these colors.
		// It could be anywhere (we have to search the whole puzzle,
		// though we can be smart about searching by piece type).
		int pCell, pSticker;
		findPiece( colors, pCell, pSticker );
		stickersToHighlight.push_back( getStickerHash( pCell, pSticker ) );
		getCell( pCell ).getConnectedStickers( pSticker, connected );
		for( uint i=0; i<connected.size(); i++ )
			stickersToHighlight.push_back( connected[i] );
	}

	for( int i=0; i<m_nCells; i++ )
		m_stickersToHighlight[i] = -2;

	for( uint i=0; i<stickersToHighlight.size(); i++ )
	{
		int cell, sticker;
		decodeStickerHash( stickersToHighlight[i], cell, sticker );
		m_stickersToHighlight[cell] = sticker;
	}
}

void 
CPuzzle::clearStickersToHighlight() 
{
	for( int i=0; i<m_nCells; i++ )
		m_stickersToHighlight[i] = -1;
}

void 
CPuzzle::setColor( int i, CColor c ) 
{
	m_settings.m_colors[i] = c;
	puzzleChanged();
}

void 
CPuzzle::startRotate( const STwist & twist ) 
{
	m_currentTwist = twist;
	m_rotation = 0;
	m_rotating = true;

	// I'm not 100% sure I like doing this, (I think inverted cells should
	// perhaps just rotate backwards!) but I also don't want to get questions about it.
	// XXX - took this out because it was causing troubles with auto-detecting undos.
	//if( !m_undoMode && !m_redoMode && m_cells[m_currentTwist.m_cell].isInverted() )
	//	m_currentTwist.reverseTwist();

	// Go ahead and calculate the new state, but don't update it yet.
	if( !m_currentTwist.m_viewRotation )
	{
		Cell & cell = getCell( m_currentTwist.m_cell );
		cell.twist( m_currentTwist, getCurrentTwistMagnitude(), 
			m_settings, this, true, false, m_state, m_currentViewRotation.m );
		cell.twist( m_currentTwist, 0, 
			m_settings, this, false, false, m_state, m_currentViewRotation.m );
	}
}

void 
CPuzzle::iterateRotate() 
{
	if( ! m_rotating )
		return;

	m_rotation += deg2rad( getCurrentTwistStep() );
	double twistMag = getCurrentTwistMagnitude();
	if( m_rotation > twistMag )
		m_rotation = twistMag;

	if( m_currentTwist.m_viewRotation )
	{
		viewChanged();
		return;
	}
	else
	{
		// For smooth rotation start/ends.
		const double rotation = getSmoothedRotation( m_rotation, twistMag );

		// Rotate.
		Cell & cell = getCell( m_currentTwist.m_cell );
		cell.twist( m_currentTwist, rotation, 
			m_settings, this, false, false, m_state, m_currentViewRotation.m );
	}
}

void 
CPuzzle::finishRotate() 
{
	m_rotating = false;
	m_rotation = 0;

	if( m_currentTwist.m_viewRotation )
	{
		// Save the current view rotation matrix.
		CMatrix4D tempRot;
		m_currentTwist.getViewRotationMatrix( getCurrentTwistMagnitude(), tempRot );
		m_currentViewRotation *= tempRot;

		// XXX - it'd be nice to be able to undo view rotations.
		viewChanged();
		return;
	}
	else
	{
		// Once to update the state and once for the drawing.
		// XXX - the later call can go away when the fading gets done.
		Cell & cell = getCell( m_currentTwist.m_cell );
		cell.twist( m_currentTwist, getCurrentTwistMagnitude(), 
			m_settings, this, false, true, m_state, m_currentViewRotation.m );
		cell.twist( m_currentTwist, 0.0, 
			m_settings, this, false, false, m_state, m_currentViewRotation.m );

		// Reset the fading information.
		for( int c=0; c<m_nCells; c++ )
			getCell( c ).m_changingFace = -1;
	}

	m_twistHistory.update( m_currentTwist );
}

const STwist & 
CPuzzle::getCurrentTwist() const
{
	return m_currentTwist;
}

double 
CPuzzle::getCurrentTwistStep() const
{
	// This function is here to slow down view rotations.
	// But we also don't want to do that in disco mode.

	if( 200 == m_settings.m_rotationStep )
		return m_settings.m_rotationStep;

	if( m_currentTwist.m_viewRotation )
		return m_settings.m_rotationStep / 3;

	return m_settings.m_rotationStep;
}

void 
CPuzzle::reset() 
{
	m_state.reset();
	m_twistHistory.clear();
}

void 
CPuzzle::resetView() 
{
	m_currentViewRotation.setIdentity();
	viewChanged();
}

bool 
CPuzzle::isSolved() const
{
	return m_state.isSolved();
}

bool 
CPuzzle::canShowSolvedMessage() 
{
	// NOTE: We shouldn't make it here unless we've been solved.
	assert( isSolved() );

	// We only want to show a message once and if we've been scrambled.
	// We also don't want to allow people just loading a solved file to see this.
	if( m_solvedMessageShown || !m_state.getScrambled() || !m_allowSolvedMessage )
		return false;

	m_solvedMessageShown = true;
	return true;
}

void 
CPuzzle::setupLighting( const CVector3D & lookFrom, bool forPicking ) const
{
	if( forPicking )
	{
		glDisable( GL_LIGHTING );
		glDisable( GL_NORMALIZE );
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		return;
	}

	// Lighting.
	glEnable( GL_LIGHTING );
	glEnable( GL_NORMALIZE );

	// Ambient lighting.
	GLfloat ambient_light[] = { .25, .25, .25, 1 };
	glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE );
	glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );
	glLightModelfv( GL_LIGHT_MODEL_AMBIENT, ambient_light );

	// Create a light at the viewer.
	GLfloat light_color[] = { 0.8f, 0.8f, 0.8f, 1 };
	GLfloat light_position[] = { 		
		(float)lookFrom.m_components[0], 
		(float)lookFrom.m_components[1], 
		(float)lookFrom.m_components[2], 1 };
	glLightfv( GL_LIGHT1, GL_DIFFUSE, light_color );
	glLightfv( GL_LIGHT1, GL_SPECULAR, light_color );
	glLightfv( GL_LIGHT1, GL_POSITION, light_position );
	glEnable( GL_LIGHT1 );

	// Go ahead and setup some stuff here depending on the wireframe setting.
	if( m_settings.m_drawWireframe )
	{
		glCullFace( GL_FRONT );
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		ambientOnly();
	}
	else
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	};
}

void 
CPuzzle::ambientOnly() const
{
	glLineWidth( 2.0 );

	// We need to change up the lighting so lines look good.
	glDisable( GL_LIGHTING );
	glDisable( GL_LIGHT1 );
	GLfloat ambient_light[] = { 1, 1, 1, 1 };
	glLightModelfv( GL_LIGHT_MODEL_AMBIENT, ambient_light );
}

void 
CPuzzle::findPiece( const std::vector<__int8> & colors, int & cell, int & sticker ) 
{
	__int8 numColors = colors.size();
	for( int c=0; c<m_nCells; c++ )
	{
		switch( numColors )
		{
		case 1:
			{
				if( checkPiece( colors, c, 0 ) )
				{
					cell = c;
					sticker = 0;
					return;
				}
				break;
			}
		case 2:
			{
				for( int s=1; s<13; s++ )
					if( checkPiece( colors, c, s ) )
					{
						cell = c;
						sticker = s;
						return;
					}
				break;
			}
		case 3:
			{
				for( int s=13; s<43; s++ )
					if( checkPiece( colors, c, s ) )
					{
						cell = c;
						sticker = s;
						return;
					}
				break;
			}
		case 4:
			{
				for( int s=43; s<63; s++ )
					if( checkPiece( colors, c, s ) )
					{
						cell = c;
						sticker = s;
						return;
					}
				break;
			}
		default:
			assert( false );
			return;
		}
	}
}

bool 
CPuzzle::checkPiece( const std::vector<__int8> & colors, int c, int s ) 
{
	if( m_state.getStickerColorIndex( c, s ) != colors[0] )
		return false;

	// XXX - getConnectedStickerColors method might be better.
	std::vector<__int8> pieceColors;
	pieceColors.push_back( colors[0] );
	std::vector<short> connected;
	getCell( c ).getConnectedStickers( s, connected );
	for( uint i=0; i<connected.size(); i++ )
	{
		int tCell, tSticker;
		decodeStickerHash( connected[i], tCell, tSticker );
		pieceColors.push_back( m_state.getStickerColorIndex( tCell, tSticker ) );
	}

	switch( colors.size() )
	{
	case 1:
		return true;
	case 2:
		return colors[1] == pieceColors[1];
	case 3:
		return
			( colors[1] == pieceColors[1] &&
			  colors[2] == pieceColors[2] ) ||
			( colors[1] == pieceColors[2] &&
			  colors[2] == pieceColors[1] );
	case 4:
		return
			( colors[1] == pieceColors[1] &&
			  colors[2] == pieceColors[2] &&
			  colors[3] == pieceColors[3] ) ||
			( colors[1] == pieceColors[1] &&
			  colors[2] == pieceColors[3] &&
			  colors[3] == pieceColors[2] ) ||
			( colors[1] == pieceColors[2] &&
			  colors[2] == pieceColors[1] &&
			  colors[3] == pieceColors[3] ) ||
			( colors[1] == pieceColors[2] &&
			  colors[2] == pieceColors[3] &&
			  colors[3] == pieceColors[1] ) ||
			( colors[1] == pieceColors[3] &&
			  colors[2] == pieceColors[2] &&
			  colors[3] == pieceColors[1] ) ||
			( colors[1] == pieceColors[3] &&
			  colors[2] == pieceColors[1] &&
			  colors[3] == pieceColors[2] );
	default:
		break;
	}

	assert( false );
	return false;
}