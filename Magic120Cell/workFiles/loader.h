#pragma once

#include "puzzle/state.h"
#include "puzzle/twist.h"


class CLoader
{
public:

	CLoader();

	// Save/Load the entire puzzle.
	void saveToFile( int puzzle, const CState & state, const std::vector<STwist> & twists, bool saveas );
	void saveScrambledFile( const CState & state, const std::vector<STwist> & twists );
	bool loadFromFile( int & puzzle, CState & state, std::vector<STwist> & twists );
	bool loadScrambledFile( CState & state, std::vector<STwist> & twists );

private:

	// Prompt for filenames.
	System::String ^ getSaveFileName( bool forcePrompt );
	System::String ^ getLoadFileName();

	// The last filename we saved.
	std::string m_filename;
};