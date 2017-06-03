#pragma once

#include "windows.h"
#include "gl\gl.h"
#include "gl\glu.h"


class CDisplayList
{
public:

	CDisplayList();
	~CDisplayList();

	CDisplayList( const CDisplayList & );
	CDisplayList & operator = ( const CDisplayList & );

	void start( bool execute = true );
	void end() const;
	bool isRecorded() const;
	bool call() const;
	void clear();

private:

	GLuint m_dl;
};