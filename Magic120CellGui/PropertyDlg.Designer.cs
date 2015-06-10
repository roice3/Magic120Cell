namespace Magic120Cell
{
	partial class PropertyDlg
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose( bool disposing )
		{
			if( disposing && (components != null) )
			{
				components.Dispose();
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.properties = new System.Windows.Forms.PropertyGrid();
			this.btnCancel = new System.Windows.Forms.Button();
			this.btnOK = new System.Windows.Forms.Button();
			this.label1 = new System.Windows.Forms.Label();
			this.SuspendLayout();
			// 
			// properties
			// 
			this.properties.Anchor = ( (System.Windows.Forms.AnchorStyles)( ( ( ( System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom ) 
            | System.Windows.Forms.AnchorStyles.Left ) 
            | System.Windows.Forms.AnchorStyles.Right ) ) );
			this.properties.Location = new System.Drawing.Point( 8, 64 );
			this.properties.Name = "properties";
			this.properties.PropertySort = System.Windows.Forms.PropertySort.NoSort;
			this.properties.Size = new System.Drawing.Size( 336, 416 );
			this.properties.TabIndex = 0;
			this.properties.ToolbarVisible = false;
			// 
			// btnCancel
			// 
			this.btnCancel.Anchor = ( (System.Windows.Forms.AnchorStyles)( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right ) ) );
			this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.btnCancel.Location = new System.Drawing.Point( 264, 488 );
			this.btnCancel.Name = "btnCancel";
			this.btnCancel.Size = new System.Drawing.Size( 80, 24 );
			this.btnCancel.TabIndex = 1;
			this.btnCancel.Text = "Cancel";
			this.btnCancel.UseVisualStyleBackColor = true;
			// 
			// btnOK
			// 
			this.btnOK.Anchor = ( (System.Windows.Forms.AnchorStyles)( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right ) ) );
			this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.btnOK.Location = new System.Drawing.Point( 176, 488 );
			this.btnOK.Name = "btnOK";
			this.btnOK.Size = new System.Drawing.Size( 80, 24 );
			this.btnOK.TabIndex = 2;
			this.btnOK.Text = "OK";
			this.btnOK.UseVisualStyleBackColor = true;
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point( 13, 13 );
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size( 327, 48 );
			this.label1.TabIndex = 3;
			this.label1.Text = "The meaning of the colors below depend on the puzzle type.  For example, Color 1 " +
    "applies to the first ring if the Rings puzzle is selected.  Colors currently can" +
    "\'t be set for the full puzzle.";
			// 
			// PropertyDlg
			// 
			this.AcceptButton = this.btnOK;
			this.AutoScaleDimensions = new System.Drawing.SizeF( 6F, 13F );
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.btnCancel;
			this.ClientSize = new System.Drawing.Size( 352, 521 );
			this.Controls.Add( this.label1 );
			this.Controls.Add( this.btnOK );
			this.Controls.Add( this.btnCancel );
			this.Controls.Add( this.properties );
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "PropertyDlg";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Properties";
			this.ResumeLayout( false );

		}

		#endregion

		private System.Windows.Forms.PropertyGrid properties;
		private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.Button btnOK;
		private System.Windows.Forms.Label label1;
	}
}