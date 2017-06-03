#pragma once


class CGlInit
{

public:

	CGlInit();
	HGLRC initializeGL( HWND hwnd );
	HGLRC initializeGL( HDC hContext );

private:

	BOOL setupPixelFormat( HDC hContext );
};

class CDisplayList
{
public:

	CDisplayList()
	{
		m_dl = (GLuint)-1;
	}

	~CDisplayList()
	{
		clear();
	}

	void start( bool execute = true )
	{
		clear();
		m_dl = glGenLists( 1 );
		glNewList( m_dl, execute ? GL_COMPILE_AND_EXECUTE : GL_COMPILE );
	}

	void end() const
	{
		glEndList();
	}

	bool isRecorded() const
	{
		return -1 != m_dl;
	}

	bool call() const
	{
		if( isRecorded() )
		{
			glCallList( m_dl );
			return true;
		}

		return false;
	}

	void clear()
	{
		// Delete the existing list if we need to.
		if( isRecorded() )
		{
			glDeleteLists( m_dl, 1 );
			m_dl = (GLuint)-1;
		}
	}

private:

	GLuint m_dl;
};