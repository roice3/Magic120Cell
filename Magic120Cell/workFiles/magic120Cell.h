#pragma once
#pragma managed(push, off)

#include "puzzle.h"
#include "cellDodec.h"
#include <map>


class CMagic120Cell : public CPuzzle
{
public:

	CMagic120Cell();

	// Main rendering control.
	void render( const CVector3D & lookFrom, bool forPicking );

	//
	// Settings and related methods.
	//

	void regenStandardCell();
	void physicalVisibilityChanged();
	void logicalVisibilityChanged();

	// Set the puzzle type.
	void setPuzzleType( int type );

	//
	// Methods to control twists/rotations.
	//

	// Twist control.
	double getCurrentTwistMagnitude() const;
	bool calcViewTwist( int clickedCell, bool leftClick, STwist & twist );

	// Scramble the puzzle.
	void scramble( int numTwists );

	//
	// Other methods.
	//

	// Reset the view since it can get all goofy looking.
	void resetView();

	// This will calculate whether a cell is visible or not
	// depending on the current display settings.
	bool isCellVisible( int cellIndex );

	// Cell access.
	Cell & getCell( int index ) { return m_cells[index]; }
	Cell & getCellUnprojected( int index ) { return m_cellsUnprojected[index]; }

	// Find a cell index given a center.
	// NOTE: function searches view transformed centers.
	int findCell( const CVector4D & center );

private:

	// Generate all our faces.
	void generateCells();
	void applyViewAndSettingTransforms();

	// Fill out our symmetry maps.
	void fillLayerMap( std::map<__int8,__int8> & layerMap ) const;
	void fillHypercubeMap();
	void fillAdjacentInfo();

	// Additional helpers.
	void generate4Cube();
	void draw4Cube( CVector4D points[32] ) const;

	// Handle puzzle changes (colors or type )
	void puzzleChanged();

private:

	// Our cells.
	// These are organized in 12 rings of 10 cells.
	// There are 2 main sets of rings (cells 1-60 and cells 61-120).
	CellDodec m_cells[120];
	CellDodec m_cellsUnprojected[120];

	// Maps that help us display cells by layer.
	std::map<__int8,__int8> m_layerMap;
	std::map<__int8,__int8> m_layerMapLogical;

	// A map that helps us display cells organized by hypercube cells.
	std::map<__int8,__int8> m_hypercubeMap;
	CVector4D m_hypercubePoints[32];
	CVector4D m_hypercubePointsCopy[32];
};

#pragma managed(pop)