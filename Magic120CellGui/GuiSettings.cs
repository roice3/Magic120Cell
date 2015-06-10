using System;
using System.ComponentModel;
using System.Drawing;

namespace Magic120Cell
{
	class GuiSettings : PersistableBase
	{
		private string name;
		private string path;

		[Persistable]
		[Browsable( false )]
		public string Name
		{
			get { return name; }
			set { name = value; }
		}

		[Browsable( false )]
		public string Path
		{
			get { return path; }
		}

		public override void Save( string file )
		{
			base.Save( file );
			this.path = file;
		}

		public override void Load( string file )
		{
			base.Load( file );
			this.path = file;
		}

		public override string ToString()
		{
			return this.name;
		}

		public GuiSettings Clone()
		{
			return (GuiSettings)MemberwiseClone();
		}

		private int perspective4d = 48;
		private int cellDistance = 0;
		private int cellSize = 60;
		private int stickerSize = 70;
		private bool drawInvertedCells = false;
		private bool reverseInvertedCellColor = false;
		private int symmetry = 2;
		private bool drawWireframe = false;
		private int rotationRate = 40;
		private short logicalVisibility = 9;
		private double r1 = 0.67532889;
		private double r2 = 0.412664445;
		private double r3 = 0.15;

		[Persistable]
		[Browsable( false )]
		public int Perspective4d
		{
			get { return perspective4d; }
			set { perspective4d = value; }
		}

		[Persistable]
		[Browsable( false )]
		public int CellDistance
		{
			get { return cellDistance; }
			set { cellDistance = value; }
		}

		[Persistable]
		[Browsable( false )]
		public int CellSize
		{
			get { return cellSize; }
			set { cellSize = value; }
		}

		[Persistable]
		[Browsable( false )]
		public int StickerSize
		{
			get { return stickerSize; }
			set { stickerSize = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool DrawInvertedCells
		{
			get { return drawInvertedCells; }
			set { drawInvertedCells = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool ReverseInvertedCellColor
		{
			get { return reverseInvertedCellColor; }
			set { reverseInvertedCellColor = value; }
		}

		[Persistable]
		[Browsable( false )]
		public int Symmetry
		{
			get { return symmetry; }
			set { symmetry = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool DrawWireframe
		{
			get { return drawWireframe; }
			set { drawWireframe = value; }
		}

		[Persistable]
		[Browsable( false )]
		public int RotationRate
		{
			get { return rotationRate; }
			set { rotationRate = value; }
		}

		[Persistable]
		[Browsable( false )]
		public short LogicalVisibility
		{
			get { return logicalVisibility; }
			set { logicalVisibility = value; }
		}

		[Persistable]
		[Browsable(false)]
		public double R1
		{
			get { return r1; }
			set { r1 = value; }
		}

		[Persistable]
		[Browsable(false)]
		public double R2
		{
			get { return r2; }
			set { r2 = value; }
		}

		[Persistable]
		[Browsable(false)]
		public double R3
		{
			get { return r3; }
			set { r3 = value; }
		}

		private bool visibility1 = true;
        private bool visibility2 = true;
        private bool visibility3 = true;
        private bool visibility4 = true;
		private bool visibility5 = true;
		private bool visibility6 = true;
        private bool visibility7 = true;
        private bool visibility8 = true;
        private bool visibility9 = true;
        private bool visibility10 = true;
        private bool visibility11 = true;
        private bool visibility12 = true;

		[Persistable]
		[Browsable( false )]
		public bool Visibility1
		{
			get { return visibility1; }
			set { visibility1 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility2
		{
			get { return visibility2; }
			set { visibility2 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility3
		{
			get { return visibility3; }
			set { visibility3 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility4
		{
			get { return visibility4; }
			set { visibility4 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility5
		{
			get { return visibility5; }
			set { visibility5 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility6
		{
			get { return visibility6; }
			set { visibility6 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility7
		{
			get { return visibility7; }
			set { visibility7 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility8
		{
			get { return visibility8; }
			set { visibility8 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility9
		{
			get { return visibility9; }
			set { visibility9 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility10
		{
			get { return visibility10; }
			set { visibility10 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility11
		{
			get { return visibility11; }
			set { visibility11 = value; }
		}

		[Persistable]
		[Browsable( false )]
		public bool Visibility12
		{
			get { return visibility12; }
			set { visibility12 = value; }
		}

        private Color color1 = ColorF( 1, 0, 0 );
        private Color color2 = ColorF( 0, 1, 0 );
        private Color color3 = ColorF( 0, 0, 1 );
		private Color color4 = ColorF( 1, 1, 0 );
        private Color color5 = ColorF( 0, 1, 1 );
        private Color color6 = ColorF( 1, 0, 1 );
		private Color color7 = ColorF( .5, .5, 1 );
		private Color color8 = ColorF( 1, 1, 1 );
		private Color color9 = ColorF( 1, 0, .5 );
		private Color color10 = ColorF( 1, .5, 0 );
		private Color color11 = Color.DarkSlateGray;
		private Color color12 = Color.SandyBrown;
		private Color colorBg = ColorF( 0, 0, 0 );

		// make a color from floats
		private static Color ColorF( double r, double g, double b )
		{
			return Color.FromArgb( (int)(r * 255), (int)(g * 255), (int)(b * 255) );
		}

		[Persistable]
		[DisplayName( "Color 1" )]
		[Category( "Coloring" )]
		public Color Color1
		{
			get { return color1; }
			set { color1 = value; }
		}

		[Persistable]
		[DisplayName( "Color 2" )]
		[Category( "Coloring" )]
		public Color Color2
		{
			get { return color2; }
			set { color2 = value; }
		}

		[Persistable]
		[DisplayName( "Color 3" )]
		[Category( "Coloring" )]
		public Color Color3
		{
			get { return color3; }
			set { color3 = value; }
		}

		[Persistable]
		[DisplayName( "Color 4" )]
		[Category( "Coloring" )]
		public Color Color4
		{
			get { return color4; }
			set { color4 = value; }
		}

		[Persistable]
		[DisplayName( "Color 5" )]
		[Category( "Coloring" )]
		public Color Color5
		{
			get { return color5; }
			set { color5 = value; }
		}

		[Persistable]
		[DisplayName( "Color 6" )]
		[Category( "Coloring" )]
		public Color Color6
		{
			get { return color6; }
			set { color6 = value; }
		}

		[Persistable]
		[DisplayName( "Color 7" )]
		[Category( "Coloring" )]
		public Color Color7
		{
			get { return color7; }
			set { color7 = value; }
		}

		[Persistable]
		[DisplayName( "Color 8" )]
		[Category( "Coloring" )]
		public Color Color8
		{
			get { return color8; }
			set { color8 = value; }
		}

		[Persistable]
		[DisplayName( "Color 9" )]
		[Category( "Coloring" )]
		public Color Color9
		{
			get { return color9; }
			set { color9 = value; }
		}

		[Persistable]
		[DisplayName( "Color 10" )]
		[Category( "Coloring" )]
		public Color Color10
		{
			get { return color10; }
			set { color10 = value; }
		}

		[Persistable]
		[DisplayName( "Color 11" )]
		[Category( "Coloring" )]
		public Color Color11
		{
			get { return color11; }
			set { color11 = value; }
		}

		[Persistable]
		[DisplayName( "Color 12" )]
		[Category( "Coloring" )]
		public Color Color12
		{
			get { return color12; }
			set { color12 = value; }
		}

		[Persistable]
		[DisplayName( "Background Color" )]
		[Category( "Coloring" )]
		public Color ColorBg
		{
			get { return colorBg; }
			set { colorBg = value; }
		}
	}


}
