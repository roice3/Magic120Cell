#include <stdafx.h>
#include "displayList.h"


CDisplayList::CDisplayList() 
{
	m_dl = (GLuint)-1;
}

CDisplayList::~CDisplayList() 
{
	clear();
}

// NOTE: We don't want to transfer control of our display list when copying or assigning.
CDisplayList::CDisplayList( const CDisplayList & ) 
{
	m_dl = (GLuint)-1;
}

CDisplayList &  
CDisplayList::operator = ( const CDisplayList & ) 
{
	m_dl = (GLuint)-1;
	return *this; 
}

void 
CDisplayList::start( bool execute ) 
{
	if( !isRecorded() )
		m_dl = glGenLists( 1 );
	glNewList( m_dl, execute ? GL_COMPILE_AND_EXECUTE : GL_COMPILE );
}

void 
CDisplayList::end() const
{
	glEndList();
}

bool 
CDisplayList::isRecorded() const
{
	return -1 != m_dl;
}

bool 
CDisplayList::call() const
{
	if( isRecorded() )
	{
		glCallList( m_dl );
		return true;
	}

	return false;
}

void 
CDisplayList::clear() 
{
	// Delete the existing list if we need to.
	if( isRecorded() )
	{
		glDeleteLists( m_dl, 1 );
		m_dl = (GLuint)-1;
	}
}