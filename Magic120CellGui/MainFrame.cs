using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace Magic120Cell
{
	public enum CubeParameter
	{
		Puzzle,
		Perspective4d,
		CellDistance,
		CellSize,
		StickerSize,
		RotationRate,
		LogicalVisibility,
		Symmetry,
		DrawInvertedCells,
		ReverseInvertedCellColor,
		DrawWireframe,
	}

	public interface ICube
	{
		// Basic commands.
		bool EnableRedraw { get; set; }
		void SetParameter( CubeParameter param, int value );
		void SetupStickerRatios( double r1, double r2, double r3 );
		void EnableFace( int face, bool enable );
		void ColorFace( int face, Color c );
        void CycleStickerAccent( int colors );
		void Save( bool saveas );
		void Load();
		void Scramble( int number );
		void Solve();
		void Undo();
		void Redo();
		void ResetState();
		void ResetView();
		void KeyUp();
	}

	public partial class MainFrame : Form
	{
		private DrawSurface surface;
		private ICube cube;

		public MainFrame( ICube cube )
		{
			InitializeComponent();

			// setup the drawing surface
			this.surface = new DrawSurface();
			this.surface.Location = this.panelMain.Location;
			this.surface.Size = this.panelMain.Size;
			this.surface.Anchor = this.panelMain.Anchor;
			this.Controls.Add( this.surface );

			// setup tags for sliders
			this.track4dDist.Tag = CubeParameter.Perspective4d;
			this.trackCellDistance.Tag = CubeParameter.CellDistance;
			this.trackCellSize.Tag = CubeParameter.CellSize;
			this.trackStickerSize.Tag = CubeParameter.StickerSize;
			this.trackRotationRate.Tag = CubeParameter.RotationRate;
			this.trackVis.Tag = CubeParameter.LogicalVisibility;

			// setup tags for face checkboxes
			this.checkVis1.Tag = 0;
			this.checkVis2.Tag = 1;
			this.checkVis3.Tag = 2;
			this.checkVis4.Tag = 3;
			this.checkVis5.Tag = 4;
			this.checkVis6.Tag = 5;
			this.checkVis7.Tag = 6;
			this.checkVis8.Tag = 7;
			this.checkVis9.Tag = 8;
			this.checkVis10.Tag = 9;
			this.checkVis11.Tag = 10;
			this.checkVis12.Tag = 11;

			// setup symmetry combo
			this.comboSymmetry.Items.Add( "Tori" );
			this.comboSymmetry.Items.Add( "4-cube cells" );
            this.comboSymmetry.Items.Add( "Layers" );
            this.comboSymmetry.Items.Add( "Rings" );

			//
			// Menu items tags.
			//

			// setup tags for puzzle type.
			// NOTE: These are persisted so don't change them.
			this.menuPuzzleTori.Tag = 2;
			this.menuPuzzle4Cube.Tag = 3;
			this.menuPuzzleLayers.Tag = 1;
			this.menuPuzzleRings.Tag = 0;
			this.menuPuzzleAntipodal.Tag = 5;
			this.menuPuzzleFull.Tag = 4;

            // setup tags for hilight items.
            this.menuHilight1.Tag = 1;
            this.menuHilight2.Tag = 2;
            this.menuHilight3.Tag = 3;
            this.menuHilight4.Tag = 4;

			// setup tags for scramble menu items.
			this.menuScramble1.Tag = 5;
			this.menuScramble2.Tag = 10;
			this.menuScramble3.Tag = 20;
			this.menuScramble4.Tag = 40;
			this.menuScramble5.Tag = 80;
			this.menuScrambleFull.Tag = 1000;

			this.cube = cube;
		}

		public DrawSurface DrawSurface
		{
			get { return this.surface; }
		}

		private void MainFrame_Load( object sender, EventArgs e )
		{
            // setup initial values
            this.cube.EnableRedraw = false;
            this.menuHilight1.CheckState = CheckState.Checked;
            this.menuHilight2.CheckState = CheckState.Checked;
            this.menuHilight3.CheckState = CheckState.Checked;
            this.menuHilight4.CheckState = CheckState.Checked;
            this.menuPuzzleFull.Checked = true;
            this.cube.EnableRedraw = true;

			// load all of our saved settings
			GuiSettings defSettings = null;
			foreach( string s in Directory.GetFiles( ".", "*.settings" ) )
			{
				GuiSettings settings = new GuiSettings();
				settings.Load( s );
				this.comboSettings.Items.Add( settings );
				if( settings.Name == DefaultSettingsName )
					defSettings = settings;
			}

			// make sure the "default" settings exist
			if( defSettings == null )
			{
				defSettings = new GuiSettings();
				defSettings.Name = DefaultSettingsName;
				this.comboSettings.Items.Add( defSettings );
			}

			// select the default settings
			this.comboSettings.SelectedItem = defSettings;

			// give the draw surface the focus
			this.DrawSurface.Select();
		}

		private const string DefaultSettingsName = "-- Default --";

		private struct Axes
		{
			public int a1, a2;

			public Axes( int a1, int a2 )
			{
				this.a1 = a1;
				this.a2 = a2;
			}
		}

		private void trackItem_ValueChanged( object sender, EventArgs e )
		{
			TrackBar control = (TrackBar)sender;
			if( this.cube != null )
				this.cube.SetParameter( (CubeParameter)control.Tag, control.Value );
		}

		private void checkFace_CheckedChanged( object sender, EventArgs e )
		{
			CheckBox control = (CheckBox)sender;
			if( this.cube != null )
				this.cube.EnableFace( (int)control.Tag, control.Checked );
		}

		// Get the axis index for a face.
		int getFaceAxisIndex( int face )
		{
			if( 10 == face )
				return -1;
			else
				return( face / 2 );
		}

		// Helper for enabling buttons.
		void enableButton( ref Button button, int face )
		{
			int invalidAxis = getFaceAxisIndex( face );
			Axes axes = (Axes)button.Tag;
			if( axes.a1 == invalidAxis ||
				axes.a2 == invalidAxis )
				button.Enabled = false;
			else
				button.Enabled = true;
		}

		static bool ControlDown() 
		{
			return (Form.ModifierKeys & Keys.Control) == Keys.Control;
		}

		private GuiSettings lastSettings = new GuiSettings();

		private GuiSettings Settings
		{
			get
			{
				// refresh from input controls
				this.lastSettings.Perspective4d = this.track4dDist.Value;
				this.lastSettings.CellDistance = this.trackCellDistance.Value;
				this.lastSettings.CellSize = this.trackCellSize.Value;
				this.lastSettings.StickerSize = this.trackStickerSize.Value;
				this.lastSettings.Symmetry = this.comboSymmetry.SelectedIndex;
				this.lastSettings.RotationRate = this.trackRotationRate.Value;
				this.lastSettings.LogicalVisibility = (short)this.trackVis.Value;
				this.lastSettings.DrawInvertedCells = this.checkDrawInverted.Checked;
				this.lastSettings.Visibility1 = this.checkVis1.Checked;
				this.lastSettings.Visibility2 = this.checkVis2.Checked;
				this.lastSettings.Visibility3 = this.checkVis3.Checked;
				this.lastSettings.Visibility4 = this.checkVis4.Checked;
				this.lastSettings.Visibility5 = this.checkVis5.Checked;
				this.lastSettings.Visibility6 = this.checkVis6.Checked;
				this.lastSettings.Visibility7 = this.checkVis7.Checked;
				this.lastSettings.Visibility8 = this.checkVis8.Checked;
				this.lastSettings.Visibility9 = this.checkVis9.Checked;
				this.lastSettings.Visibility10 = this.checkVis10.Checked;
				this.lastSettings.Visibility11 = this.checkVis11.Checked;
				this.lastSettings.Visibility12 = this.checkVis12.Checked;

				return this.lastSettings.Clone();
			}
			set
			{
				this.lastSettings = value.Clone();

				// update the input controls and the cube
				this.cube.EnableRedraw = false;
				this.track4dDist.Value = value.Perspective4d;
				this.trackCellDistance.Value = value.CellDistance;
				this.trackCellSize.Value = value.CellSize;
				this.trackStickerSize.Value = value.StickerSize;
				this.comboSymmetry.SelectedIndex = value.Symmetry;
				this.trackRotationRate.Value = value.RotationRate;
				this.trackVis.Value = value.LogicalVisibility;
                this.checkDrawInverted.Checked = value.DrawInvertedCells;
				this.checkVis1.Checked = value.Visibility1;
				this.checkVis2.Checked = value.Visibility2;
				this.checkVis3.Checked = value.Visibility3;
				this.checkVis4.Checked = value.Visibility4;
				this.checkVis5.Checked = value.Visibility5;
				this.checkVis6.Checked = value.Visibility6;
				this.checkVis7.Checked = value.Visibility7;
				this.checkVis8.Checked = value.Visibility8;
				this.checkVis9.Checked = value.Visibility9;
				this.checkVis10.Checked = value.Visibility10;
				this.checkVis11.Checked = value.Visibility11;
				this.checkVis12.Checked = value.Visibility12;
				this.cube.ColorFace( 0, value.Color1 );
				this.cube.ColorFace( 1, value.Color2 );
				this.cube.ColorFace( 2, value.Color3 );
				this.cube.ColorFace( 3, value.Color4 );
				this.cube.ColorFace( 4, value.Color5 );
				this.cube.ColorFace( 5, value.Color6 );
				this.cube.ColorFace( 6, value.Color7 );
				this.cube.ColorFace( 7, value.Color8 );
				this.cube.ColorFace( 8, value.Color9 );
				this.cube.ColorFace( 9, value.Color10 );
				this.cube.ColorFace( 10, value.Color11 );
				this.cube.ColorFace( 11, value.Color12 );
				this.cube.ColorFace( -1, value.ColorBg );
				this.cube.EnableRedraw = true;

				if( this.lastSettings.R1 == 0.67532889 &&
					this.lastSettings.R2 == 0.412664445 &&
					this.lastSettings.R3 == .15 )
					this.menuCutStyle.Checked = true;
				else
					this.menuCutStyle.Checked = false;
			}
		}

		private void btnSettingsSave_Click( object sender, EventArgs e )
		{
			using( InputDlg dlg = new InputDlg() )
			{
				// get a name for the settings
				dlg.Caption = "Save Settings";
				dlg.Prompt = "Please provide a name for the new saved settings.";
				if( this.comboSettings.SelectedItem != null )
					dlg.Value = ((GuiSettings)this.comboSettings.SelectedItem).Name;
				if( DialogResult.OK != dlg.ShowDialog( this ) )
					return;
				foreach( GuiSettings old in this.comboSettings.Items )
				{
					if( old.Name == dlg.Value )
					{
						this.comboSettings.Items.Remove( old );
						break;
					}
				}

				// the new settings they are saving
				GuiSettings s = this.Settings;
				s.Name = dlg.Value;

				// find where we can save it
				string baseFile = s.Name;
				foreach( char c in Path.GetInvalidFileNameChars() )
					baseFile = baseFile.Replace( c, '_' );
				string final = baseFile + ".settings";
				int next = 1;
				while( System.IO.File.Exists( final ) )
					final = baseFile + (next++) + ".settings";
				s.Save( final );

				// add it to the list
				this.comboSettings.Items.Add( s );
				this.comboSettings.SelectedItem = s;
			}
		}

		private void btnSettingsDelete_Click( object sender, EventArgs e )
		{
			if( this.comboSettings.SelectedItem == null )
				return;
			GuiSettings settings = (GuiSettings)this.comboSettings.SelectedItem;
			if( settings.Name == DefaultSettingsName )
				return;
			this.comboSettings.Items.Remove( settings );
			File.Delete( settings.Path );
		}

		private void comboSettings_SelectedIndexChanged( object sender, EventArgs e )
		{
			if( this.comboSettings.SelectedItem == null )
				return;
			GuiSettings settings = (GuiSettings)this.comboSettings.SelectedItem;
			this.btnSettingsDelete.Enabled = (settings.Name != DefaultSettingsName);
			this.Settings = settings;
			this.cube.ResetView();
		}

		private void comboSymmetry_SelectedIndexChanged( object sender, EventArgs e )
		{
			if( this.cube != null )
				this.cube.SetParameter( CubeParameter.Symmetry, this.comboSymmetry.SelectedIndex );
            
			switch( this.comboSymmetry.SelectedIndex )
			{
                case 0:

                    this.checkVis1.Enabled = true;
                    this.checkVis2.Enabled = true;
                    this.checkVis3.Enabled = false;
                    this.checkVis4.Enabled = false;
                    this.checkVis5.Enabled = false;
                    this.checkVis6.Enabled = false;
                    this.checkVis7.Enabled = false;
                    this.checkVis8.Enabled = false;
                    this.checkVis9.Enabled = false;
                    this.checkVis10.Enabled = false;
                    this.checkVis11.Enabled = false;
                    this.checkVis12.Enabled = false;
                    break;

				case 1:
				case 2:

					this.checkVis1.Enabled = true;
					this.checkVis2.Enabled = true;
					this.checkVis3.Enabled = true;
					this.checkVis4.Enabled = true;
					this.checkVis5.Enabled = true;
					this.checkVis6.Enabled = true;
					this.checkVis7.Enabled = true;
					this.checkVis8.Enabled = true;
					this.checkVis9.Enabled = true;
					this.checkVis10.Enabled = 1 == this.comboSymmetry.SelectedIndex ? true : false;
					this.checkVis11.Enabled = false;
					this.checkVis12.Enabled = false;
					break;

                case 3:

                    this.checkVis1.Enabled = true;
                    this.checkVis2.Enabled = true;
                    this.checkVis3.Enabled = true;
                    this.checkVis4.Enabled = true;
                    this.checkVis5.Enabled = true;
                    this.checkVis6.Enabled = true;
                    this.checkVis7.Enabled = true;
                    this.checkVis8.Enabled = true;
                    this.checkVis9.Enabled = true;
                    this.checkVis10.Enabled = true;
                    this.checkVis11.Enabled = true;
                    this.checkVis12.Enabled = true;
                    break;
			}
		}

		private void checkDrawInverted_CheckedChanged( object sender, EventArgs e )
		{
			if( this.cube != null )
				this.cube.SetParameter( CubeParameter.DrawInvertedCells, this.checkDrawInverted.Checked ? 1 : 0 );
		}

		private void menuPuzzle_Click( object sender, EventArgs e )
		{
			ToolStripMenuItem item = (ToolStripMenuItem)sender;
			int puzzleType = (int)item.Tag;
			this.cube.SetParameter( CubeParameter.Puzzle, puzzleType );

			this.menuPuzzleTori.Checked =
			this.menuPuzzle4Cube.Checked =
			this.menuPuzzleLayers.Checked =
			this.menuPuzzleRings.Checked =
			this.menuPuzzleAntipodal.Checked =
			this.menuPuzzleFull.Checked = false;
			item.Checked = true;
		}

		private void menuLoad_Click( object sender, EventArgs e )
		{
			this.cube.Load();
		}

		private void menuSave_Click( object sender, EventArgs e )
		{
			this.cube.Save( false );
		}

		private void menuSaveAs_Click( object sender, EventArgs e )
		{
			this.cube.Save( true );
		}

		private void scramble( int num )
		{
			// This can take a while so show a wait cursor.
			Cursor old = this.Cursor;
			this.Cursor = Cursors.WaitCursor;
			this.cube.Scramble(num);
			this.Cursor = old;
		}

		private void menuScramble_Click( object sender, EventArgs e )
		{
			ToolStripMenuItem item = (ToolStripMenuItem)sender;
			int scrambleNumber = (int)item.Tag;
			scramble( scrambleNumber );
		}

		private void menuScrambleCustom_Click(object sender, EventArgs e)
		{
			using (InputDlg dlg = new InputDlg())
			{
				dlg.Caption = "Custom Scramble";
				dlg.Prompt = "Please provide a number of twists to perform.";
				if( DialogResult.OK != dlg.ShowDialog(this) )
					return;

				string s = dlg.Value;
				int num = 0;
				try
				{
					num = System.Convert.ToInt32(s);
				}
				catch
				{
					MessageBox.Show( this, "Please retry with a valid number.   ", "Value is not a number", 
						MessageBoxButtons.OK, MessageBoxIcon.Error );
					return;
				}

				scramble( num );
			}
		}

		private void menuSolve_Click( object sender, EventArgs e )
		{
			this.cube.Solve();
		}

		private void menuUndo_Click( object sender, EventArgs e )
		{
			this.cube.Undo();
		}

		private void menuRedo_Click( object sender, EventArgs e )
		{
			this.cube.Redo();
		}

		private void menuReset_Click( object sender, EventArgs e )
		{
			this.cube.ResetState();
		}

		private void menuEditColors_Click( object sender, EventArgs e )
		{
			GuiSettings current = this.Settings;
			PropertyDlg dlg = new PropertyDlg();
			dlg.EditObject = current;
			if( DialogResult.OK != dlg.ShowDialog( this ) )
				return;
			this.Settings = current;
        }

        private void menuHilight_Click(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            this.cube.CycleStickerAccent((int)item.Tag);

            // Cycle the check state.
            item.Checked = !item.Checked;
        }

        private void menuResetView_Click(object sender, EventArgs e)
        {
			this.cube.ResetView();
        }

		private void menuMouseCommands_Click(object sender, EventArgs e)
		{
			String text =
				"Click any Sticker:\t\tTwists a cell (axis defined by the sticker and cell center; left/right clicking controls direction).   \n\n" +
				"Left Mouse Button Drag:\tRotates the view.\n\n" +
				"Right Mouse Button Drag:\tZooms the view.\n\n" +
				"Ctrl+Click any Cell:\t\t4D view rotation:\n\t\t\t" +
					"- left click moves clicked cell to puzzle \"center\".\n\t\t\t" +
					"- right click moves puzzle \"center\" to clicked cell.\n\n" +
				"Shift+Click any Sticker:\tHighlights all the stickers that are on the same piece, regardless of visibility settings.\n\n" +
				"Shift+Ctrl+Click any Sticker:\tPiece finding functionality:\n\t\t\t" +
					"- left click highlights the cell centers of the eventual home of the piece.\n\t\t\t" +
					"- right click highlights the piece that should live in the clicked location.\n\n";
			String caption = "Mouse Commands";
			MessageBox.Show( this, text, caption, MessageBoxButtons.OK, MessageBoxIcon.Information );
		}

		private void menuHowToSolve_Click(object sender, EventArgs e)
		{
			String text = "Sorry you're on your own!  :)\n\n" +
				"But you can get lots of help from the 4D cubing group,   \n" +
				"so please consider joining!\n\n" +
				"games.groups.yahoo.com/group/4D_Cubing/";
			String caption = "How to Solve";
			MessageBox.Show( this, text, caption, MessageBoxButtons.OK, MessageBoxIcon.Information );
		}

		private void menuAbout_Click(object sender, EventArgs e)
		{
			String text = string.Format("Magic120Cell Version (1+sqrt(5))/2   \n");
			text += "Copyright 2008 by Roice Nelson.\n" +
				"www.gravitation3d.com/magic120cell   ";
			String caption = "About";
			MessageBox.Show( this, text, caption, MessageBoxButtons.OK, MessageBoxIcon.Information );
		}

		private void menuCutStyle_Click(object sender, EventArgs e)
		{
			bool newState = !menuCutStyle.Checked;
			if( newState )
			{
				this.lastSettings.R1 = 0.67532889;
				this.lastSettings.R2 = 0.412664445;
				this.lastSettings.R3 = .15;
			}
			else
			{
				this.lastSettings.R1 = 0.5;
				this.lastSettings.R2 = 1.0 / 3;
				this.lastSettings.R3 = 1.0 / 3;
			}
			menuCutStyle.Checked = newState;
		}

		private void menuCutStyle_CheckedChanged(object sender, EventArgs e)
		{
			if( this.cube != null )
				this.cube.SetupStickerRatios( this.Settings.R1, this.Settings.R2, this.Settings.R3 );
		}

		private void MainFrame_KeyUp(object sender, KeyEventArgs e)
		{
			if( this.cube != null )
				this.cube.KeyUp();
		}
	}

	public class DrawSurface : Control
	{
		public DrawSurface()
		{
			SetStyle( ControlStyles.UserMouse, true );
			SetStyle( ControlStyles.UserPaint, true );
			SetStyle( ControlStyles.AllPaintingInWmPaint, true );
			SetStyle( ControlStyles.Selectable, true );
		}

		protected override bool IsInputKey( Keys keyData )
		{
			if( keyData == Keys.Left || keyData == Keys.Right || keyData == Keys.Up || keyData == Keys.Down )
				return true;
			return base.IsInputKey( keyData );
		}

		protected override void OnPaintBackground( PaintEventArgs pevent )
		{
			// do nothing, otherwise we get flicker-city
		}

	}
}
