#pragma once
#pragma managed(push, off)

#include "cell.h"
#include "helper.h"
#include "loader.h"
#include "puzzle/twist.h"
#include "puzzle/twistHistory.h"
#include "renderer.h"
#include "settings.h"


class CPuzzle : public IRenderable
{
public:

	CPuzzle( int nCells, int nStickers );

	// Main rendering control.
	virtual void render( const CVector3D & lookFrom, bool forPicking );

	// Save/load.
	void save( bool saveas );
	void load();

	// Sticker highlighting.
	void highlightSticker( int stickerHash, bool shiftOnly, bool leftClick );
	void clearStickersToHighlight();

	//
	// Settings related methods.
	//

	CSettings & getSettings() { return m_settings; }

	void settingsChanged() { m_settingsChanged = true; }
	void viewChanged() { m_settingsChanged = true; }		// XXX - make a separate variable as an optimization?
	void shapeChanged() { m_shapeChanged = true; }
	virtual void regenStandardCell() = 0;
	virtual void physicalVisibilityChanged() = 0;
	virtual void logicalVisibilityChanged() = 0;

	// Set the puzzle type.
	virtual void setPuzzleType( int type ) = 0;

	// Set the colors.
	void setColor( int i, CColor c );

	//
	// Methods to control twists/rotations.
	//

	void startRotate( const STwist & twist );
	void iterateRotate();
	void finishRotate();
	const STwist & getCurrentTwist() const;
	double getCurrentTwistStep() const;

	// Access to twist history.
	CTwistHistory & twistHistory() { return m_twistHistory; }

	// The rotation angle magnitude to make this twist, in radians.
	virtual double getCurrentTwistMagnitude() const = 0;

	// Returns true if twist should be applied.
	virtual bool calcViewTwist( int clickedCell, bool leftClick, STwist & twist ) = 0;

	//
	// Other methods.
	//

	// Scramble/reset the puzzle.
	virtual void scramble( int numTwists ) = 0;
	void reset();

	// Mark us as not scrambled.
	void markUnscrambled() { m_state.setScrambled( false ); }

	// Reset the view since it can get all goofy looking.
	virtual void resetView();

	// Are we solved?
	bool isSolved() const;
	bool canShowSolvedMessage();

	// Cell access.
	virtual Cell & getCell( int index ) = 0;
	virtual Cell & getCellUnprojected( int index ) = 0;

protected:

	// Handle puzzle changes (colors or type )
	virtual void puzzleChanged() = 0;

	// View and setting transforms.
	virtual void applyViewAndSettingTransforms() = 0;

	// Lighting/color helpers.
	void setupLighting( const CVector3D & lookFrom, bool forPicking ) const;
	void ambientOnly() const;

private:

	// Find a piece.
	// This will just return the hash for one of the stickers on the piece.
	void findPiece( const std::vector<__int8> & colors, int & cell, int & sticker );
	bool checkPiece( const std::vector<__int8> & colors, int c, int s );

protected:

	// Our settings.
	CSettings m_settings;

	// Puzzle info.
	int m_nCells;
	int m_nStickers;

	// Our puzzle state.
	CState m_state;

	// A loader.
	CLoader m_loader;

	// Rotation variables.
	STwist m_currentTwist;
	bool m_rotating;
	double m_rotation;

	// Our twist history.
	CTwistHistory m_twistHistory;

	// Track if our settings or shape have changed.
	bool m_settingsChanged;
	bool m_shapeChanged;

	// Whether or not we have shown the solved message already.
	bool m_solvedMessageShown;
	bool m_allowSolvedMessage;

	// Stickers to highlight.
	// XXX - make this depend on m_nCells.
	__int8 m_stickersToHighlight[120];

	// The current view rotation matrix.
	CMatrix4D m_currentViewRotation;
};

#pragma managed(pop)