#pragma once
#include "../helper.h"
#include <vector>


class CState
{
public:

	CState( int nCells, int nStickers );
	void reset();

	// Setup one of our cell colors (This doesn't change the state).
	void setCellColor( int cell, const CColor & color );

	// Get the color for a sticker.
	const CColor & getStickerColor( int cell, int sticker ) const;

	// Get the color index for a sticker.
	int getStickerColorIndex( int cell, int sticker ) const;

	// State changes.
	void setStickerColorIndex( int cell, int sticker, int color );
	void commitChanges();
	void commitChanges( int changedCell );
	void commitChanges( int changedCells[], int num );

	// Setup full colors.
	void setupFullColors( unsigned int seed = 0 );
	void setupAntipodalColors();

	bool getScrambled() const { return m_scrambled; }
	void setScrambled( bool scrambled ) { m_scrambled = scrambled; }
	bool isSolved() const;

	// Size access.
	int nCells() const { return m_nCells; }
	int nStickers() const { return m_nStickers; }

private:

	void initializeState();

private:

	int m_nCells;
	int m_nStickers;

	// This matrix will hold a representation of the puzzle state.
	// The left index cycles through cells.
	// The right index cycles through stickers on those cells.
	// The matrix integers represent the sticker colors.
	std::vector<std::vector<int>> m_state;
	std::vector<std::vector<int>> m_copy;

	// Our colors.
	std::vector<CColor> m_colors;

	// Whether or not we have had a full scramble.
	// (Used to control congratulatory message).
	bool m_scrambled;
};