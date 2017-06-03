#include <stdafx.h>
#include "twistHistory.h"


CTwistHistory::CTwistHistory() 
{
	m_undoMode = m_redoMode = false;
}

void 
CTwistHistory::clear() 
{
	m_twists.clear();
	m_redoTwists.clear();
}

void 
CTwistHistory::update( const STwist & twist ) 
{
	if( m_undoMode )
	{
		// Remove from twist list.
		m_twists.pop_back();
		
		// Save in our redo list.
		STwist temp( twist );
		temp.reverseTwist();
		m_redoTwists.push_back( temp );
		m_undoMode = false;
		return;
	}

	if( m_redoMode )
	{
		m_redoTwists.pop_back();
		m_redoMode = false;
	}
	else
	{
		m_redoTwists.clear();
	}

	// This block should apply to normal twists and redo twists.
	m_twists.push_back( twist );
}

bool 
CTwistHistory::getUndoTwist( STwist & twist, bool setUndoMode ) 
{
	if( ! m_twists.size() )
		return false;

	twist = m_twists[ m_twists.size()-1 ];
	twist.reverseTwist();

	if( setUndoMode )
		m_undoMode = true;
	return true;
}

bool 
CTwistHistory::getRedoTwist( STwist & twist ) 
{
	if( ! m_redoTwists.size() )
		return false;

	twist = m_redoTwists[ m_redoTwists.size()-1 ];

	m_redoMode = true;
	return true;
}