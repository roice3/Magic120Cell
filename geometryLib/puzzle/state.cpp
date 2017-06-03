#include <stdafx.h>
#include "state.h"


CState::CState( int nCells, int nStickers )
{
	m_nCells = nCells;
	m_nStickers = nStickers;
	initializeState();
	setupFullColors();
}

void 
CState::reset() 
{
	initializeState();
}

void 
CState::setCellColor( int cell, const CColor & color ) 
{
	assert( cell < m_nCells );
	m_colors[cell] = color;
}


const CColor & 
CState::getStickerColor( int cell, int sticker ) const
{
	assert( cell < m_nCells && sticker < m_nStickers );
	return m_colors[ getStickerColorIndex( cell, sticker ) ];
}

int 
CState::getStickerColorIndex( int cell, int sticker ) const
{
	assert( cell < m_nCells && sticker < m_nStickers);
	return m_state[cell][sticker];
}

void 
CState::setStickerColorIndex( int cell, int sticker, int color ) 
{
	m_copy[cell][sticker] = color;
}

void 
CState::commitChanges() 
{
	m_state = m_copy;
}

void 
CState::commitChanges( int changedCell ) 
{
	for( int s=0; s<m_nStickers; s++ )
		m_state[changedCell][s] = m_copy[changedCell][s];
}

void 
CState::commitChanges( int changedCells[], int num ) 
{
	for( int i=0; i<num; i++ )
		commitChanges( changedCells[i] );
}

void 
CState::initializeState()
{
	m_state.resize( m_nCells );
	m_copy.resize( m_nCells );

	for( int c=0; c<m_nCells; c++ )
	{
		m_state[c].resize( m_nStickers );
		m_copy[c].resize( m_nStickers );

		for( int s=0; s<m_nStickers; s++ )
			m_state[c][s] = m_copy[c][s] = c;
	}
	m_scrambled = false;
}

void 
CState::setupFullColors() 
{
	m_colors.resize( m_nCells );
	if( m_nCells <= 0 )
		return;

	srand( 0 );

	// First cell white.
	m_colors[0] = CColor();

	// Fill in the color array.
	for( int i=1; i<m_nCells; ++i )
	{
		double hue = getRandomDouble( 1.0 );
		double luminance = .25 + getRandomDouble( .5 );

		// Make most high saturation.
		double saturation = .8 + getRandomDouble( .2 );
		double randomTest = getRandomDouble( 1.0 );
		if( randomTest < .2 )
			saturation = randomTest;

		m_colors[i].setColorHLS( hue, luminance, saturation, 1.0 );
	}
}

void 
CState::setupAntipodalColors()
{
	setupFullColors();

	// Now edit half of them.
	for( int r=0; r<12; r++ )
		for( int c=0; c<5; c++ )
			m_colors[10*r + (c+5)] = m_colors[10*r + c];
}

bool 
CState::isSolved() const
{
	// XXX - performance could be better if we compared ints
	//		 instead of CColors, but that would involve bigger changes.
	for( int c=0; c<m_nCells; c++ )
	{
		CColor cellColor = getStickerColor( c, 0 );
		for( int s=1; s<m_nStickers; s++ )
		{
			if( ! ( getStickerColor( c, s ) == cellColor ) )
				return false;
		}
	}

	return true;
}