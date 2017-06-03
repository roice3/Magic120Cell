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