#pragma once

#include "twist.h"


class CTwistHistory
{
public:

	CTwistHistory();

	void clear();

	// Adds a twist to our history.
	void update( const STwist & twist );

	// Get undo rotation parameters.
	// Calling this will set us in the "undo" state for the next rotation.
	// Returns false if there are no more twists to undo.
	bool getUndoTwist( STwist & twist, bool setUndoMode = true );

	// Get redo rotation parameters.
	// Calling this will set us in the "redo" state for the next rotation.
	// Returns false if there are no more twists to redo.
	bool getRedoTwist( STwist & twist );

	// Access to the internal set of twists.
	const std::vector<STwist> & getAllTwists() const { return m_twists; }
	std::vector<STwist> & getAllTwists() { return m_twists; }

private:

	// Whether we are in undo/redo modes.
	bool m_undoMode, m_redoMode;

	// Our twist history.
	std::vector<STwist> m_twists;
	std::vector<STwist> m_redoTwists;
};