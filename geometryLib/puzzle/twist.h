#pragma once
#include "../matrix4d.h"
#include "../vectorND.h"
#include <assert.h>
#include <string>
#include <vector>


// Slicemask defines.
#define SLICEMASK_1 0x0001
#define SLICEMASK_2 0x0002
#define SLICEMASK_3 0x0004
#define SLICEMASK_4 0x0008
#define SLICEMASK_5 0x0010
#define SLICEMASK_6 0x0020
#define SLICEMASK_7 0x0040
#define SLICEMASK_8 0x0080
#define SLICEMASK_9 0x0100

namespace
{
	// XXX - move to a better shared location
	//		 maybe the class for loading saving state.
	// These versions only support up to 64 stickers per cell.
	int getStickerHash( int physicalCell, int sticker )
	{
		int ret = (physicalCell<<6) + sticker;
		return( ret );
	}

	void decodeStickerHash( int hash, int & cell, int & sticker )
	{
		cell = (hash>>6);
		sticker = hash - (cell<<6);
	}

	//
	// These versions are for puzzles a very large number of stickers per cell.
	// XXX - This was done for 'Magic Tile', and there is work debt here 
	//		 related to twist hashes below.
	//
	int getStickerHashEx( int cell, int sticker )
	{
		int ret = (cell<<16) + sticker;
		return( ret );
	}

	void decodeStickerHashEx( int hash, int & cell, int & sticker )
	{
		cell = (hash>>16);
		sticker = hash - (cell<<16);
	}

	int sliceToMask( int slice )
	{
		switch( slice )
		{
		case 1:
			return SLICEMASK_1;
		case 2:
			return SLICEMASK_2;
		case 3:
			return SLICEMASK_3;
		case 4:
			return SLICEMASK_4;
		case 5:
			return SLICEMASK_5;
		case 6:
			return SLICEMASK_6;
		case 7:
			return SLICEMASK_7;
		case 8:
			return SLICEMASK_8;
		case 9:
			return SLICEMASK_9;
		default:
			assert( false );
			return 0;
		}
	}
}

// Simple struct for a twist (also used for view rotations).
struct STwist
{
	STwist()
	{
		m_cell = -1;
		m_sticker = -1;
		m_leftClick = true;
		m_viewRotation = false;
		m_viewRotationMagnitude = 0;
		m_slicemask = 1;
	}

	STwist( int hash, bool left )
	{
		decodeStickerHash( hash, m_cell, m_sticker );
		m_leftClick = left;
		m_viewRotation = false;
		m_viewRotationMagnitude = 0;
	}

	bool operator == ( const STwist & rhs ) const
	{
		return( 
			m_cell == rhs.m_cell &&
			m_sticker == rhs.m_sticker &&
			m_leftClick == rhs.m_leftClick &&
			m_viewRotation == rhs.m_viewRotation &&
			m_viewRotationMagnitude == rhs.m_viewRotationMagnitude &&
			m_viewRotationP1 == rhs.m_viewRotationP1 &&
			m_viewRotationP2 == rhs.m_viewRotationP2 &&
			m_slicemask == rhs.m_slicemask );
	}

	// The rotation cell refers to physical cells.
	int m_cell;
	int m_sticker;
	bool m_leftClick;
	bool m_viewRotation;
	int m_slicemask;		// XXX - not yet part of the twist hash, i.e. not persisted.

	// View rotations are special.
	// The don't fit the standard twist mold.
	// XXX - Perhaps view rotations should be handled in a separate class.
	double m_viewRotationMagnitude;
	CVector4D m_viewRotationP1, m_viewRotationP2;

public:

	// View rotation calculation.
	void getViewRotationMatrix( double angle, CMatrix4D & R );

	// Helper to reverse this twist.
	void reverseTwist()
	{
		m_leftClick = !m_leftClick;
	}

	// XXX - move to a better shared location
	//		 maybe the class for loading saving state.
	// These versions should only be used for Magic120Cell
	int getTwistHash() const
	{
		// 63 stickers, 120 cells, 1 bit for twist direction, 1 bit to specify a view rotation.
		int stickerHash = getStickerHash( m_cell, m_sticker );
		int leftClick = m_leftClick == true ? 1 : 0;
		int viewRot = m_viewRotation == true ? 1 : 0;
		int ret = (stickerHash<<2) + (leftClick<<1) + viewRot;
		return( ret );
	}

	void decodeTwistHash( int hash )
	{
		int stickerHash = (hash>>2);
		decodeStickerHash( stickerHash, m_cell, m_sticker );
		int remain = hash - (stickerHash<<2);
		int leftClick = (remain>>1);
		int viewRot = remain - (leftClick<<1);
		m_leftClick = 1 == leftClick;
		m_viewRotation = 1 == viewRot;
	}
};