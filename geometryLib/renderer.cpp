#include <stdafx.h>
#include "renderer.h"

#include "atlstr.h"
#include "atlimage.h"
#include "puzzle/twist.h"


CRenderer::CRenderer( IRenderable * renderable ) :
	m_renderable( renderable )
{
	m_viewLookfrom = CVector3D( 30, 45, 25 );
	m_viewLookfrom *= .42;
	m_viewLookat = CVector3D();
	m_up = CVector3D( 0, 0, 1 );
	m_step = 0;

	// A higher clip near was working better for 120 cell.
	setupClippingPlanes( .5 );
}

void 
CRenderer::setupClippingPlanes( GLdouble clipNear, GLdouble clipFar ) 
{
	m_clipNear = clipNear;
	m_clipFar = clipFar;
}

void 
CRenderer::renderScene( int width, int height ) 
{
	renderSceneInternal( width, height, false, false );
}

int 
CRenderer::renderForPicking( int width, int height, int x, int y ) 
{
	renderSceneInternal( width, height, false, true );
	
	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	BYTE cell = 0;
	BYTE sticker = 0;
	BYTE background = 0;
	glReadPixels( x, viewport[3] - y,  1, 1, GL_RED, GL_BYTE, &cell );
	glReadPixels( x, viewport[3] - y,  1, 1, GL_GREEN, GL_BYTE, &sticker );
	glReadPixels( x, viewport[3] - y,  1, 1, GL_BLUE, GL_BYTE, &background );
	int val = background ? -1 : getStickerHash( cell, sticker );
	return val;
}

void CRenderer::renderSceneInternal( int width, int height, bool stereo, bool forPicking ) 
{
	/* This block was used to manually create frames for an output video.
	
	m_step--;
	if( 0 != m_step %10 )
		return;

	if( m_step < 0 )
		return;

	m_viewLookfrom.rotateAboutAxis( CVector3D(0,0,1), M_PI / 1500 );*/

	//
	// GL Drawing setup.
	//

	// Setup the projection.
	setupProjection( width, height );

	// Clear the color.
	{
		if( forPicking )
		{
			// We must clear to black or there could be problems.
			glClearColor( 0, 0, 1, 1 );
		}
		else
			glClearColor( (float)m_bgColor.m_r, (float)m_bgColor.m_g, (float)m_bgColor.m_b, 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	// Enable depth testing.
	glEnable( GL_DEPTH_TEST );

	// Enable alpha blending.
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	// Smooth shading.
	glShadeModel( GL_SMOOTH );

	// Anitaliasing.
	// Stereo doesn't like this.
	if( ! stereo )
	{
		glEnable( GL_LINE_SMOOTH );
		glEnable( GL_POINT_SMOOTH );
		//glEnable( GL_POLYGON_SMOOTH );	// Mades the polygons look bad (GeForce4 implemented this).
	}

	// Setup the view.
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );
	setupView( );

	//
	// Render everything here.
	//
	m_renderable->render( m_viewLookfrom, forPicking );

	// Setup for text drawing.
	/*
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluOrtho2D( 0, (GLfloat)width, 0,  (GLfloat)height );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );
	*/

	//
	// Render all the text here.
	//

	glFinish();

	//m_step++;
	//if( !forPicking )
	//	screenShot( width, height );
}

void 
CRenderer::screenShot( int width, int height ) 
{
	//assert( glutExtensionSupported( "GL_EXT_packed_pixels" );

	// Setup our output image.
	// We have to do the negative height to make it 
	// a top-down DIB (so we can call getBits).
	CImage image;
	if( !image.Create( width, height*-1, 32 ) )
	{
		assert( false );
	}

	// Read from the back buffer before swapping.
	glReadBuffer( GL_BACK );
	glReadPixels( 0, 0, 
		width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, image.GetBits() );

	// Setup an output image (we need to flip the result from above vertically.
	CImage imageOut;
	if( !imageOut.Create( width, height*-1, 32 ) )
	{
		assert( false );
	}
	HDC hdc = imageOut.GetDC(); 
	image.StretchBlt( hdc, 0, height-1, width, -1*height ); 
	imageOut.ReleaseDC();
	
	CString outName;
	outName.Format( _T("C:\\p4\\video\\scratch\\%05d.png"), m_step );
	imageOut.Save( outName, Gdiplus::ImageFormatPNG );
}

void CRenderer::setupProjection( int cx, int cy ) 
{
	// Calculate the clipping plane based on the current view.
	//
	// This may need to be revisited now that there are many different viewing possibilities.
	// Seem ok though.
	double abs = m_viewLookfrom.abs( );
	m_clipFar  = abs + 100.0f;
	m_clipFar *= 1.1;

	if( cy > 0 )
	{
		glViewport( 0, 0, cx, cy );
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		gluPerspective( 30, (GLdouble)cx/cy, m_clipNear, m_clipFar );
		glMatrixMode( GL_MODELVIEW );
	}
}

void CRenderer::setupView( ) 
{
	gluLookAt( 
		m_viewLookfrom.m_components[0], 
		m_viewLookfrom.m_components[1], 
		m_viewLookfrom.m_components[2], 
		m_viewLookat.m_components[0], 
		m_viewLookat.m_components[1], 
		m_viewLookat.m_components[2], 
		m_up.m_components[0], 
		m_up.m_components[1],
		m_up.m_components[2] );
}