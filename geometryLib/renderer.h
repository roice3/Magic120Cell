#pragma once
#include <atltypes.h>
#include "vectorND.h"

#include "gl/glInit.h"

class IRenderable
{
public:

	virtual void render( const CVector3D & lookFrom, bool forPicking ) = 0;
};


class CRenderer
{
public:

	CRenderer( IRenderable * renderable );

	void setupClippingPlanes( GLdouble clipNear, GLdouble clipFar = -1 );

	// This will do the actual rendering.
	void renderScene( int width, int height );

	void screenShot( int width, int height );

	// This will do rendering for picking.
	// The return value is a hash integer representing a sticker.
	int renderForPicking( int width, int height, int x, int y );

	// Set the background color.
	void setBackgroundColor( CColor c ) { m_bgColor = c; }

private:

	void renderSceneInternal( int width, int height, bool stereo, bool forPicking );

	// GL helper methods.
	void setupProjection( int cx, int cy );
	void setupView( );

public:

	CVector3D m_viewLookat;
	CVector3D m_viewLookfrom;
	CVector3D m_up;

	CColor m_bgColor;

private:

	GLdouble m_clipNear, m_clipFar;

	IRenderable * m_renderable;
	int m_step;
};