// {{{ClassName=CubeFrame

#include <stdafx.h>
#include "cubeFrame.h"

#include "gl/glInit.h"
#include "helper.h"
#include "workFiles/magic120Cell.h"

#define _USE_MATH_DEFINES
#include <math.h>

using namespace System::Drawing;

namespace Magic120Cell
{

CubeFrame::CubeFrame() 
{
	m_solving = false;
	m_enableRedraw = true;
	m_animating	= false;

	m_dragging = false;
	m_mouseDownX = m_mouseDownY = -1;
	m_mouseLastX = m_mouseLastY = 0;
}

CubeFrame::~CubeFrame() 
{
	if( m_components )
		delete m_components;

	if( m_puzzle )
		delete( m_puzzle );

	if( m_renderer )
		delete( m_renderer );
}

void 
CubeFrame::Run() 
{
	m_frame = gcnew MainFrame( this );

	m_components = gcnew System::ComponentModel::Container();
	m_timer = gcnew Timer( m_components );
	m_timer->Interval = 20;
	m_timer->Tick += gcnew EventHandler( this, &CubeFrame::TimerTick );

	m_frame->Load += gcnew EventHandler( this, &CubeFrame::FrameLoad );

	m_frame->DrawSurface->Paint += gcnew PaintEventHandler( this, &CubeFrame::Paint );
	m_frame->DrawSurface->KeyDown += gcnew KeyEventHandler( this, &CubeFrame::KeyDown );
	m_frame->DrawSurface->LostFocus += gcnew EventHandler( this, &CubeFrame::LostFocus );
	m_frame->DrawSurface->MouseDown += gcnew MouseEventHandler( this, &CubeFrame::MouseDown );
	m_frame->DrawSurface->MouseMove += gcnew MouseEventHandler( this, &CubeFrame::MouseMove );
	m_frame->DrawSurface->MouseUp += gcnew MouseEventHandler( this, &CubeFrame::MouseUp );

	//m_puzzle = new CMagicSimplex4D();
	m_puzzle = new CMagic120Cell();
	m_renderer = new CRenderer( m_puzzle );

	Application::Run( m_frame );
}

void 
CubeFrame::FrameLoad( Object^ sender, EventArgs^ args ) 
{
	CGlInit glInit;
	glInit.initializeGL( (HWND)m_frame->DrawSurface->Handle.ToInt32() );
}

void 
CubeFrame::Paint( Object^ sender, System::Windows::Forms::PaintEventArgs^ args ) 
{
	DoRender( args->Graphics );
}

void 
CubeFrame::TimerTick( System::Object^ sender, System::EventArgs^ e )
{
	if( m_currentAngle + m_puzzle->getCurrentTwistStep() >= rad2deg( m_puzzle->getCurrentTwistMagnitude() ) )
	{
		m_puzzle->finishRotate();
		DoRender();
		bool showMessage = false;

		// Special handling if we are solving.
		if( m_solving )
		{
			// Start the next undo.
			STwist undo;
			if( m_puzzle->twistHistory().getUndoTwist( undo ) )
			{
				internalRotate( undo );
				return;
			}
			else
			{
				m_solving = false;
			}
		}
		else
		{
			// NOTE: Don't do solution checks after view rotations.
			if( !m_puzzle->getCurrentTwist().m_viewRotation && m_puzzle->isSolved() )
			{
				// Beep!
				System::Media::SystemSounds::Asterisk::get()->Play();

				// See if we should show a message.
				showMessage = m_puzzle->canShowSolvedMessage();
			}
		}

		m_timer->Enabled = false;
		m_animating = false;

		Redraw( false );

		// Show a congratulatory message if they've solved it.
		if( showMessage )
			showSolvedMessage();

		return;
	}

	// Do the drawing.
	m_puzzle->iterateRotate();
	DoRender();

	m_currentAngle += m_puzzle->getCurrentTwistStep();
}

void 
CubeFrame::MouseMove(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) 
{
	// Make sure we have the focus.
	m_frame->DrawSurface->Select();

	// Are we starting a drag?
	// NOTE: The mousedown checks make sure we had a mouse down call and fixes a problem I was seeing
	//		 where the view would reset when you loaded in a log file.
	if( ! m_dragging && e->Button != MouseButtons::None &&
		-1 != m_mouseDownX && -1 != m_mouseDownY &&
		((Math::Abs( e->X - m_mouseDownX ) > SystemInformation::DragSize.Width / 2) ||
		(Math::Abs( e->Y - m_mouseDownY ) > SystemInformation::DragSize.Height / 2)) )
	{
		StartDrag();

		// Fake the original mouse position so we will get some drag motion immediately.
		m_mouseLastX = m_mouseDownX;
		m_mouseLastY = m_mouseDownY;
	}

	// Are we dragging?
	if( m_dragging )
		PerformDrag( e->X, e->Y, e->Button );

	m_mouseLastX = e->X;
	m_mouseLastY = e->Y;

	// We don't need to continue past here if we are animating.
	if( m_animating )
		return;

	Redraw( true );
}

void
CubeFrame::StartDrag() 
{
	m_dragging = true;
	m_frame->DrawSurface->Capture = true;
}

void 
CubeFrame::PerformDrag( int x, int y, MouseButtons btn ) 
{
	// This is increment we moved, scaled to the window size.
	float xIncrement = (float)( x - m_mouseLastX ) / (float)m_frame->DrawSurface->ClientRectangle.Width;
	float yIncrement = (float)( y - m_mouseLastY ) / (float)m_frame->DrawSurface->ClientRectangle.Height;

	// A measure of the magnitude of the change.
	float magnitude = (float)( sqrt( xIncrement*xIncrement + yIncrement*yIncrement ) * 100 );

	if( btn == System::Windows::Forms::MouseButtons::Left )
	{
		// The spherical coordinate radius.
		CVector3D & view = m_renderer->m_viewLookfrom;
		double radius = view.abs();
		if( ! IS_ZERO( radius ) )
		{
			CVector3D & up = m_renderer->m_up;
			CVector3D v1 = up.cross( view );
			CVector3D v2 = view.cross( v1 );
			v1.normalize();
			v2.normalize();

			view.rotateAboutAxis( v2, -xIncrement*magnitude );
			view.rotateAboutAxis( v1, -yIncrement*magnitude );
			up.rotateAboutAxis( v2, -xIncrement*magnitude );
			up.rotateAboutAxis( v1, -yIncrement*magnitude );

			/* OLD way
			// The spherical coordinate angles.
			double theta = atan2( viewVector.m_components[1], viewVector.m_components[0] );
			double phi = acos( viewVector.m_components[2] / radius );

			// Increment the angles.
			theta -= magnitude * xIncrement;
			phi   -= magnitude * yIncrement;

			// Check the bounds.
			// So we don't have to worry about the upVector, don't allow these to be exactly 0 or Pi.
			if( phi <= 0 )
				phi  = 0.0000001;
			if( phi >= CONSTANT_PI )
				phi  = CONSTANT_PI - 0.0000001;

			// Calculate the new position.
			m_renderer->m_viewLookfrom.m_components[0] = m_renderer->m_viewLookat.m_components[0] + radius * sin( phi ) * cos( theta );
			m_renderer->m_viewLookfrom.m_components[1] = m_renderer->m_viewLookat.m_components[1] + radius * sin( phi ) * sin( theta );
			m_renderer->m_viewLookfrom.m_components[2] = m_renderer->m_viewLookat.m_components[2] + radius * cos( phi );
			*/
		}
	}

	if( btn == System::Windows::Forms::MouseButtons::Right )
	{
		// The view vector magnitude.
		CVector3D viewVector = m_renderer->m_viewLookfrom - m_renderer->m_viewLookat;
		double abs = viewVector.abs();
	
		// Increment it.
		abs += 5 * abs * yIncrement;
		viewVector.normalize();
		viewVector *= abs;

		double smallestRadius = .02;
		if( viewVector.abs() < smallestRadius )
		{
			viewVector.normalize( );
			viewVector *= smallestRadius;
		}

		// Set the new position.
		m_renderer->m_viewLookfrom = viewVector;
	}
}

void 
CubeFrame::FinishDrag() 
{
	m_frame->DrawSurface->Capture = false;
	m_dragging = false;
}

void 
CubeFrame::MouseDown(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) 
{
	m_mouseDownX = e->X;
	m_mouseDownY = e->Y;
}

void 
CubeFrame::MouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) 
{
	// NOTE: The mousedown checks make sure we had a mouse down call and fixes a problem I was seeing
	//		 where where unintended sticker clicks could happen when loading a log file.
	if( -1 == m_mouseDownX || -1 == m_mouseDownY )
		return;

	m_mouseDownX = m_mouseDownY = -1;

	// Figure out if we were dragging, and if the drag is done.
	if( m_dragging )
	{
		if( Form::MouseButtons == MouseButtons::None )
			FinishDrag();
		return;
	}

	if( m_animating )
		return;

	// Past here, the mouse-up represents a click.

	if( e->Button == System::Windows::Forms::MouseButtons::Left || 
		e->Button == System::Windows::Forms::MouseButtons::Right )
	{
		if( ShiftDown() && CtrlDown() && AltDown() )
		{
			m_puzzle->regenColors();
			return;
		}

		bool left = e->Button == System::Windows::Forms::MouseButtons::Left;
		
		int stickerHash = m_renderer->renderForPicking( 
			m_frame->DrawSurface->ClientRectangle.Width, 
			m_frame->DrawSurface->ClientRectangle.Height,
			e->X, e->Y );

		if( -1 != stickerHash )
		{
			// What we do depends on ctrl, shift, left/right.
			if( ShiftDown() && CtrlDown() )
			{
				m_puzzle->highlightSticker( stickerHash, false, left );
			}
			else if( ShiftDown() )
			{
				m_puzzle->highlightSticker( stickerHash, true, left );
			}
			else
			{
				if( CtrlDown() )
				{
					int clickedCell, dummy;
					decodeStickerHash( stickerHash, clickedCell, dummy );
					
					STwist twist;
					if( m_puzzle->calcViewTwist( clickedCell, left, twist ) )
						internalRotate( twist );
				}
				else
				{
					STwist twist( stickerHash, left );

					// Detect if this twist was just the reverse of the last twist.
					STwist temp;
					if( m_puzzle->twistHistory().getUndoTwist( temp, false ) && temp == twist )
						m_puzzle->twistHistory().getUndoTwist( temp );

					// Don't try to twist if a 1C was clicked.
					if( 0 != twist.m_sticker )
						internalRotate( twist );
				}
			}
		}	
	}
}

void 
CubeFrame::KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) 
{
	if( m_solving && e->KeyCode == Keys::Escape )
		m_solving = false;
}

void 
CubeFrame::LostFocus( Object^ sender, System::EventArgs^ args ) 
{
	/*
	// Reset click handling.
	m_puzzle->clearStickersToHighlight();

	Redraw( true );
	*/
}

void 
CubeFrame::SetParameter( CubeParameter param, int value ) 
{
	CSettings & settings = m_puzzle->getSettings();
	switch( param )
	{
	case CubeParameter::Puzzle:
		m_puzzle->setPuzzleType( (int)value );
		break;
	case CubeParameter::Perspective4d:
		settings.m_projection4Distance = 3.0 + pow( (double)value / 25, 2 );
		break;
	case CubeParameter::CellDistance:
		settings.m_cellDistance = 1.0 + (double)value / 25;
		break;
	case CubeParameter::CellSize:
		settings.m_cellSize = (double)value / 100;
		break;
	case CubeParameter::StickerSize:
		settings.m_stickerSize = (double)value / 100;
		break;
	case CubeParameter::RotationRate:
		settings.m_rotationStep = 0.1 + (double)value / 5;
		
		// Make extremely large for disco ball mode.
		if( (int)value == 100 )
			settings.m_rotationStep = 200;
		break;
	case CubeParameter::LogicalVisibility:
		settings.m_logicalVisibility = (short)value;
		break;
	case CubeParameter::Symmetry:
		settings.m_symmetry = value;
		m_puzzle->physicalVisibilityChanged();
		break;
	case CubeParameter::DrawInvertedCells:
		settings.m_drawInvertedCells = 1 == value ? true : false;
		break;
	case CubeParameter::ReverseInvertedCellColor:
		settings.m_reverseInvertedCellColor = 1 == value ? true : false;
		break;
	case CubeParameter::DrawWireframe:
		settings.m_drawWireframe = 1 == value ? true : false;
		break;
	}

	m_puzzle->settingsChanged();
	Redraw( true );
}

void 
CubeFrame::SetupStickerRatios( double r1, double r2, double r3 )
{
	CSettings & settings = m_puzzle->getSettings();
	settings.m_r1 = r1;
	settings.m_r2 = r2;
	settings.m_r3 = r3;
	m_puzzle->shapeChanged();
	Redraw( true );
}

bool
CubeFrame::EnableRedraw::get()
{
	return m_enableRedraw;
}

void
CubeFrame::EnableRedraw::set( bool val )
{
	m_enableRedraw = val;
	if( val )
		Redraw( false );
}

void 
CubeFrame::EnableFace( int face, bool enable ) 
{
	m_puzzle->getSettings().m_visibility[face] = enable;
	m_puzzle->physicalVisibilityChanged();
	Redraw( true );
}

void 
CubeFrame::ColorFace( int face, System::Drawing::Color c ) 
{
	// Setting background color?
	CColor color( (float)c.R / 255, (float)c.G / 255, (float)c.B / 255, (float)c.A / 255 );
	if( -1 == face )
		m_renderer->setBackgroundColor( color );
	else
		m_puzzle->setColor( face, color );

}

void 
CubeFrame::CycleStickerAccent( int colors ) 
{
	CSettings & settings = m_puzzle->getSettings();
	switch( colors )
	{
	case 1:
		settings.m_highlight1C = !settings.m_highlight1C;
		break;
	case 2:
		settings.m_highlight2C = !settings.m_highlight2C;
		break;
	case 3:
		settings.m_highlight3C = !settings.m_highlight3C;
		break;
	case 4:
		settings.m_highlight4C = !settings.m_highlight4C;
		break;
	}

	Redraw( true );
}

void 
CubeFrame::Save( bool saveas ) 
{
	if( m_animating )
		return;

	m_puzzle->save( saveas );
}

void 
CubeFrame::Load() 
{
	if( m_animating )
		return;

	m_puzzle->load();
	Redraw( true );
}

void 
CubeFrame::Scramble( int number ) 
{
	if( m_animating )
		return;

	m_puzzle->scramble( number );
	Redraw( true );
}

void 
CubeFrame::Solve() 
{
	if( m_animating )
		return;

	// Start the first undo.
	STwist undo;
	if( m_puzzle->twistHistory().getUndoTwist( undo ) )
	{
		m_solving = true;
		internalRotate( undo );
	}

	// Clicking the solve button means the puzzle shouldn't be counted as scrambled.
	m_puzzle->markUnscrambled();
}

void 
CubeFrame::Undo() 
{
	if( m_animating )
		return;

	STwist undo;
	if( m_puzzle->twistHistory().getUndoTwist( undo ) )
		internalRotate( undo );
}

void 
CubeFrame::Redo() 
{
	if( m_animating )
		return;

	STwist redo;
	if( m_puzzle->twistHistory().getRedoTwist( redo ) )
		internalRotate( redo );
}

void 
CubeFrame::ResetState() 
{
	if( m_animating )
		return;

	m_puzzle->reset();
}

void 
CubeFrame::ResetView()
{
	if( m_animating )
		return;

	m_puzzle->resetView();
	DoRender();
}

void 
CubeFrame::KeyUp() 
{
	// Reset click handling.
	if( !ShiftDown() || !CtrlDown() )
	{
		m_puzzle->clearStickersToHighlight();
		Redraw( true );
	}
}

void 
CubeFrame::internalRotate( const STwist & twist ) 
{
	m_currentAngle = 0;
	m_puzzle->startRotate( twist );
	m_animating = true;
	m_timer->Enabled = true;
}

void 
CubeFrame::showSolvedMessage()
{
	String ^ text = "You are a true permutation puzzle master and now one with the universe.\n\n"
		"Be sure to send your logfile to roice@gravitation3d.com\n"
		"to get your name listed in the Magic120Cell Hall of Insanity!";
	String ^ caption = "Magic120Cell Unraveled!";
	System::Windows::Forms::MessageBox::Show( text, caption, MessageBoxButtons::OK, MessageBoxIcon::Exclamation );
}

void 
CubeFrame::Redraw( bool force ) 
{
	if( force || m_enableRedraw )
		m_frame->DrawSurface->Invalidate();
}

void 
CubeFrame::DoRender() 
{
	Graphics^ g = Graphics::FromHwnd( m_frame->DrawSurface->Handle );
	DoRender( g );
	g->~Graphics();
}

void 
CubeFrame::DoRender( Graphics^ g )
{
	m_renderer->renderScene( m_frame->DrawSurface->ClientRectangle.Width, m_frame->DrawSurface->ClientRectangle.Height );

	IntPtr hdc = g->GetHdc();
	SwapBuffers( (HDC)hdc.ToPointer() );
	g->ReleaseHdc( hdc );
}

bool 
CubeFrame::ShiftDown() 
{
	return (Form::ModifierKeys & Keys::Shift) == Keys::Shift;
}

bool 
CubeFrame::CtrlDown() 
{
	return (Form::ModifierKeys & Keys::Control) == Keys::Control;
}

bool
CubeFrame::AltDown()
{
	return (Form::ModifierKeys & Keys::Alt) == Keys::Alt;
}

}
