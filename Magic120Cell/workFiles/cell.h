#pragma once
#pragma managed(push, off)

#include "primitives.h"
#include "puzzle/state.h"
#include "puzzle/twist.h"
#include "settings.h"

class CPuzzle;


class Cell
{
public:

	Cell();
	Cell & operator = ( const Cell & rhs );

	// A physical integer assigned to this cell.
	__int8 m_physicalCell;

	// Visibility variables.
	bool m_visiblePhysical;
	bool m_visibleLogical;
	bool isVisible() { return m_visibleLogical && m_visiblePhysical; }
	int m_changingFace;		// Means latest twist will fade visibility of stickers on this face.

	// The cell center.
	const CVector4D & getCenter() const;

	// Render.
	void render( const CSettings & settings, const CState & state, bool forceVisible, 
		int highlightSticker, bool forPicking, double percentTwisted );

	// Twist control.
	// This is probably THE most complicated method.
	void twist( const STwist & twist, double rotation, 
		const CSettings & settings, CPuzzle * pPuzzle, bool calculateNewState,
		bool updateState, CState & state, double mViewRot[][4] );

	//
	// Transformation helpers.
	//

	// Full cell transforms.
	void applyCellTransforms();
	void applyCellTransforms( const CVector4D & cellTransforms, bool undo );
	void applyViewTransforms( double mRot[][4] );
	void applySettingTransforms( const CSettings & settings );

private:

	// These are all meant to be called on adjacent cells during a twist.
	void applyCellTransforms( __int8 face, const CVector4D & transforms, bool undo );
	void applyViewTransforms( __int8 face, double mRot[][4] );
	void applySettingTransforms( __int8 face, const CVector4D & cellCenter, const CSettings & settings );
	void rotateAboutAxis( __int8 face, CVector3D axis, double rotation );

public:

	//
	// Required implementation.
	//

	// Generate all the stickers on this cell.
	virtual void generateStickers( const CSettings & settings ) = 0;

	// Get the connected stickers (returns full hash of stickers).
	virtual void getConnectedStickers( int sticker, std::vector<short> & connected ) const = 0;

protected:

	virtual Cell & getStandardCell() = 0;
	virtual void regenStickers( __int8 face ) = 0;
	virtual bool isInverted() = 0;
	virtual const SPrimitive3D & getBounds() const = 0;
	virtual const SPrimitive3D & getSticker( int sticker ) const = 0;
	virtual int getStickerIndex( int face, int indexOnFace ) = 0;

protected:

	// Other internal helpers.
	void copyGeometryInfo( const Cell & source ); // XXX - should this go away?
	SPrimitive3D & getBounds();
	SPrimitive3D & getSticker( int sticker );

protected:

	int m_nStickers;
	int m_n2DFaces;
	int m_nStickersPer2DFace;

	// The encoding for these will be different depending on the cell type.
	CVector4D m_cellTransforms;

	// Our adjacent cells/faces.
	int m_adjacentCells[12];
	int m_adjacentFaces[12];
	VecToIntMap m_adjacentCellPositions;

	// Our sticker position map (transformed sticker constant points to sticker indices).
	VecToIntMap m_stickerPositionMap;
};

#pragma managed(pop)