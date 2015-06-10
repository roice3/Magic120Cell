#pragma once
#pragma managed(push, off)

#include "cell.h"


class CellDodec : public Cell
{
public:

	CellDodec();
	CellDodec & operator = ( const CellDodec & rhs );

	// Setup methods.
	static void shapeChanged();
	void setupCellTransforms( bool ringSet, __int8 ring, __int8 ringItem );
	void addAdjacentCell( const Cell & aCell );
	void findAdjacentFaces( CellDodec cells[] );
	void findAdjacentStickers( CellDodec cells[] );

	//
	// Overrides.
	//

	void generateStickers( const CSettings & settings );
	void getConnectedStickers( int sticker, std::vector<short> & connected ) const;

private:

	Cell & getStandardCell();
	void regenStickers( __int8 face );
	bool isInverted();
	const SPrimitive3D & getBounds() const { return m_bounds; }
	const SPrimitive3D & getSticker( int sticker ) const;
	int getStickerIndex( int face, int indexOnFace );

	// Other internal helpers.
	void setConnectedSticker( int sticker, int num, short hash );
	static void generateStaticItems( const CSettings & settings );

private:

	// The bounds of this cell.
	SDodecahedron m_bounds;

	/*
	We have 63 stickers in a cell.  Here is a breakdown.

	type    number    description
	1C	     1	      12 sides (dodecahedron)
	2C	    12	       7 sides (pentagonal prism)
	3C	    30	       6 sides (hexahedron)
	4C	    20	       6 sides (hexahedron)
	*/

	// Our stickers.
	SDodecahedron m_1C;
	SShape2CSticker m_2C[12];
	SHexaHedron m_3C[30];
	SHexaHedron m_4C[20];

	// Our adjacent stickers (stores are of full sticker hashes).
	short m_2Ca[12];
	short m_3Ca[30*2];
	short m_4Ca[20*3];

	// The cell transforms.
	// This is data that identifies our position in the 120-cell.
	// This will be used to calculate/apply the transforms from the origin to
	// the correct location in the puzzle.
	bool m_ringSet;
	__int8 m_ring;
	__int8 m_ringItem;
};

#pragma managed(pop)