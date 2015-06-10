#include <stdafx.h>
#include "settings.h"

#pragma unmanaged


CSettings::CSettings() 
{
	m_puzzle = 4;  // Full puzzle.
	m_cellDistance = 1.0;
	m_cellSize = 1.0;
	m_stickerSize = .75;
	m_r1 = 0.67532889;
	m_r2 = 0.412664445;
	m_r3 = 0.15;
	m_drawInvertedCells = false;
	m_reverseInvertedCellColor = false;
	m_symmetry = 2;	// Layer display
	m_drawWireframe = false;

	m_logicalVisibility = 9;

	m_highlight1C = true;
	m_highlight2C = true;
	m_highlight3C = true;
	m_highlight4C = true;
	m_centerCell = 5;

	for( int i=0; i<12; i++ )
		m_visibility.push_back( true );
}