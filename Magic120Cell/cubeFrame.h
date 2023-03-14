
#pragma once

#include "workFiles/puzzle.h"

using namespace System;
using namespace System::Windows::Forms;

namespace Magic120Cell
{

	public ref class CubeFrame : ICube
	{
	public:

		CubeFrame();
		~CubeFrame();

		// run the application
		void Run();

	protected:

		// event handers
		void FrameLoad( Object^ sender, EventArgs^ args );
		void Paint( Object^ sender, System::Windows::Forms::PaintEventArgs^ args );
		void TimerTick( System::Object^ sender, System::EventArgs^ e );
		void MouseDown( Object^ sender, System::Windows::Forms::MouseEventArgs^ args );
		void MouseMove( Object^ sender, System::Windows::Forms::MouseEventArgs^ args );
		void MouseUp( Object^ sender, System::Windows::Forms::MouseEventArgs^ args );
		void KeyDown( Object^ sender, System::Windows::Forms::KeyEventArgs^ args );
		void LostFocus( Object^ sender, System::EventArgs^ args );

		// drag helpers
		void StartDrag();
		void PerformDrag( int x, int y, MouseButtons btn );
		void FinishDrag();

		// paint helpers
		void Redraw( bool force );
		void DoRender( System::Drawing::Graphics^ g );
		void DoRender();

		// keyboard helpers
		static bool ShiftDown();
		static bool CtrlDown();
		static bool AltDown();

	public:

		// ICube methods
		virtual void SetParameter( CubeParameter param, int value );
		virtual void SetupStickerRatios( double r0, double r1, double r2 );
		virtual void EnableFace( int face, bool enable );
		virtual void ColorFace( int face, System::Drawing::Color c );
		virtual void CycleStickerAccent( int colors );
		virtual void Save( bool saveas );
		virtual void Load();
		virtual void Scramble( int number );
		virtual void Solve();
		virtual void Undo();
		virtual void Redo();
		virtual void ResetState();
		virtual void ResetView();
		virtual void KeyUp();
		virtual property bool EnableRedraw
		{
			bool get();
			void set( bool );
		}

	private:

		// Internal rotation method
		// NOTE: This will always rotate, even if the puzzle is in the animating state.
		//		 You'll want to consider using the ICube rotate method instead.
		void internalRotate( const STwist & twist );

		// Show a congratulatory solved message.
		void showSolvedMessage();

	private:

		// The main frame of our UI
		MainFrame^ m_frame;

		// Additonal managed component bits.
		System::ComponentModel::IContainer^ m_components;
		Timer^	m_timer;

		// The cube renderer.
		CRenderer* m_renderer;
		CPuzzle* m_puzzle;
		bool	m_enableRedraw;

		// Whether or not we are solving.
		bool	m_solving;

		// Rotation stuff.
		double	m_currentAngle;

		// Animating?
		bool	m_animating;

		// Mouse moving.
		int		m_mouseDownX;
		int		m_mouseDownY;
		bool	m_dragging;
		int		m_mouseLastX;
		int		m_mouseLastY;
	};

}
