namespace OldCPUEmulatorGUI
{
	partial class Form1
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.recentFilesListBox = new System.Windows.Forms.ListBox();
			this.titleLabel = new System.Windows.Forms.Label();
			this.subtitleLabel = new System.Windows.Forms.Label();
			this.recentFilesGroupBox = new System.Windows.Forms.GroupBox();
			this.optionsGroupBox = new System.Windows.Forms.GroupBox();
			this.refreshRateFloorFifteenCheckBox = new System.Windows.Forms.CheckBox();
			this.setSyncedProcessAffinityOneCheckBox = new System.Windows.Forms.CheckBox();
			this.setProcessPriorityHighCheckBox = new System.Windows.Forms.CheckBox();
			this.refreshHzLabel = new System.Windows.Forms.Label();
			this.refreshHzNumericUpDown = new System.Windows.Forms.NumericUpDown();
			this.targetMhzComboBox = new System.Windows.Forms.ComboBox();
			this.syncedProcessMainThreadOnlyCheckBox = new System.Windows.Forms.CheckBox();
			this.newButton = new System.Windows.Forms.Button();
			this.goButton = new System.Windows.Forms.Button();
			this.newOpenFileDialog = new System.Windows.Forms.OpenFileDialog();
			this.currentMhzLabel = new System.Windows.Forms.Label();
			this.targetMhzLabel = new System.Windows.Forms.Label();
			this.currentMhzValueLabel = new System.Windows.Forms.Label();
			this.recentFilesGroupBox.SuspendLayout();
			this.optionsGroupBox.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.refreshHzNumericUpDown)).BeginInit();
			this.SuspendLayout();
			// 
			// recentFilesListBox
			// 
			this.recentFilesListBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.recentFilesListBox.FormattingEnabled = true;
			this.recentFilesListBox.Location = new System.Drawing.Point(6, 19);
			this.recentFilesListBox.Name = "recentFilesListBox";
			this.recentFilesListBox.Size = new System.Drawing.Size(248, 69);
			this.recentFilesListBox.TabIndex = 0;
			// 
			// titleLabel
			// 
			this.titleLabel.AutoSize = true;
			this.titleLabel.Font = new System.Drawing.Font("Trebuchet MS", 16F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.titleLabel.Location = new System.Drawing.Point(12, 12);
			this.titleLabel.Name = "titleLabel";
			this.titleLabel.Size = new System.Drawing.Size(256, 27);
			this.titleLabel.TabIndex = 2;
			this.titleLabel.Text = "Old CPU Emulator 1.4.0";
			// 
			// subtitleLabel
			// 
			this.subtitleLabel.AutoSize = true;
			this.subtitleLabel.Font = new System.Drawing.Font("Georgia", 12F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.subtitleLabel.ForeColor = System.Drawing.Color.DimGray;
			this.subtitleLabel.Location = new System.Drawing.Point(12, 44);
			this.subtitleLabel.Name = "subtitleLabel";
			this.subtitleLabel.Size = new System.Drawing.Size(165, 18);
			this.subtitleLabel.TabIndex = 3;
			this.subtitleLabel.Text = "By Anthony Kleine";
			// 
			// recentFilesGroupBox
			// 
			this.recentFilesGroupBox.Controls.Add(this.recentFilesListBox);
			this.recentFilesGroupBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.recentFilesGroupBox.Location = new System.Drawing.Point(12, 77);
			this.recentFilesGroupBox.Name = "recentFilesGroupBox";
			this.recentFilesGroupBox.Size = new System.Drawing.Size(260, 100);
			this.recentFilesGroupBox.TabIndex = 4;
			this.recentFilesGroupBox.TabStop = false;
			this.recentFilesGroupBox.Text = "Recent Files";
			// 
			// optionsGroupBox
			// 
			this.optionsGroupBox.Controls.Add(this.refreshRateFloorFifteenCheckBox);
			this.optionsGroupBox.Controls.Add(this.setSyncedProcessAffinityOneCheckBox);
			this.optionsGroupBox.Controls.Add(this.setProcessPriorityHighCheckBox);
			this.optionsGroupBox.Controls.Add(this.refreshHzLabel);
			this.optionsGroupBox.Controls.Add(this.refreshHzNumericUpDown);
			this.optionsGroupBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.optionsGroupBox.Location = new System.Drawing.Point(12, 277);
			this.optionsGroupBox.Name = "optionsGroupBox";
			this.optionsGroupBox.Size = new System.Drawing.Size(259, 134);
			this.optionsGroupBox.TabIndex = 5;
			this.optionsGroupBox.TabStop = false;
			this.optionsGroupBox.Text = "Options";
			// 
			// refreshRateFloorFifteenCheckBox
			// 
			this.refreshRateFloorFifteenCheckBox.AutoSize = true;
			this.refreshRateFloorFifteenCheckBox.Checked = true;
			this.refreshRateFloorFifteenCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
			this.refreshRateFloorFifteenCheckBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.refreshRateFloorFifteenCheckBox.Location = new System.Drawing.Point(126, 79);
			this.refreshRateFloorFifteenCheckBox.Name = "refreshRateFloorFifteenCheckBox";
			this.refreshRateFloorFifteenCheckBox.Size = new System.Drawing.Size(106, 43);
			this.refreshRateFloorFifteenCheckBox.TabIndex = 4;
			this.refreshRateFloorFifteenCheckBox.Text = "Round Down\r\nRefresh Rate\r\nto Nearest 15 Hz";
			this.refreshRateFloorFifteenCheckBox.UseVisualStyleBackColor = true;
			this.refreshRateFloorFifteenCheckBox.CheckedChanged += new System.EventHandler(this.refreshRateFloorFifteenCheckBox_CheckedChanged);
			// 
			// setSyncedProcessAffinityOneCheckBox
			// 
			this.setSyncedProcessAffinityOneCheckBox.AutoSize = true;
			this.setSyncedProcessAffinityOneCheckBox.Checked = true;
			this.setSyncedProcessAffinityOneCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
			this.setSyncedProcessAffinityOneCheckBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.setSyncedProcessAffinityOneCheckBox.Location = new System.Drawing.Point(127, 43);
			this.setSyncedProcessAffinityOneCheckBox.Name = "setSyncedProcessAffinityOneCheckBox";
			this.setSyncedProcessAffinityOneCheckBox.Size = new System.Drawing.Size(122, 30);
			this.setSyncedProcessAffinityOneCheckBox.TabIndex = 3;
			this.setSyncedProcessAffinityOneCheckBox.Text = "Set Synced Process\r\nAffinity to One";
			this.setSyncedProcessAffinityOneCheckBox.UseVisualStyleBackColor = true;
			// 
			// setProcessPriorityHighCheckBox
			// 
			this.setProcessPriorityHighCheckBox.AutoSize = true;
			this.setProcessPriorityHighCheckBox.Checked = true;
			this.setProcessPriorityHighCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
			this.setProcessPriorityHighCheckBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.setProcessPriorityHighCheckBox.Location = new System.Drawing.Point(10, 43);
			this.setProcessPriorityHighCheckBox.Name = "setProcessPriorityHighCheckBox";
			this.setProcessPriorityHighCheckBox.Size = new System.Drawing.Size(117, 30);
			this.setProcessPriorityHighCheckBox.TabIndex = 2;
			this.setProcessPriorityHighCheckBox.Text = "Set Process Priority\r\nto High";
			this.setProcessPriorityHighCheckBox.UseVisualStyleBackColor = true;
			// 
			// refreshHzLabel
			// 
			this.refreshHzLabel.AutoSize = true;
			this.refreshHzLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.refreshHzLabel.Location = new System.Drawing.Point(7, 20);
			this.refreshHzLabel.Name = "refreshHzLabel";
			this.refreshHzLabel.Size = new System.Drawing.Size(95, 13);
			this.refreshHzLabel.TabIndex = 0;
			this.refreshHzLabel.Text = "Refresh Rate (Hz:)";
			// 
			// refreshHzNumericUpDown
			// 
			this.refreshHzNumericUpDown.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.refreshHzNumericUpDown.Location = new System.Drawing.Point(126, 17);
			this.refreshHzNumericUpDown.Maximum = new decimal(new int[] {
            1000,
            0,
            0,
            0});
			this.refreshHzNumericUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
			this.refreshHzNumericUpDown.Name = "refreshHzNumericUpDown";
			this.refreshHzNumericUpDown.Size = new System.Drawing.Size(127, 20);
			this.refreshHzNumericUpDown.TabIndex = 5;
			this.refreshHzNumericUpDown.Value = new decimal(new int[] {
            1000,
            0,
            0,
            0});
			// 
			// targetMhzComboBox
			// 
			this.targetMhzComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.targetMhzComboBox.FormattingEnabled = true;
			this.targetMhzComboBox.Items.AddRange(new object[] {
            "Intel Pentium II 233 MHz (late 1990s)",
            "Intel Pentium III 750 MHz (early 2000s)",
            "Custom Target Rate..."});
			this.targetMhzComboBox.Location = new System.Drawing.Point(12, 250);
			this.targetMhzComboBox.Name = "targetMhzComboBox";
			this.targetMhzComboBox.Size = new System.Drawing.Size(260, 21);
			this.targetMhzComboBox.TabIndex = 0;
			this.targetMhzComboBox.SelectedIndexChanged += new System.EventHandler(this.targetMhzComboBox_SelectedIndexChanged);
			this.targetMhzComboBox.TextUpdate += new System.EventHandler(this.targetMhzComboBox_TextUpdate);
			// 
			// syncedProcessMainThreadOnlyCheckBox
			// 
			this.syncedProcessMainThreadOnlyCheckBox.AutoSize = true;
			this.syncedProcessMainThreadOnlyCheckBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.syncedProcessMainThreadOnlyCheckBox.Location = new System.Drawing.Point(22, 356);
			this.syncedProcessMainThreadOnlyCheckBox.Name = "syncedProcessMainThreadOnlyCheckBox";
			this.syncedProcessMainThreadOnlyCheckBox.Size = new System.Drawing.Size(110, 30);
			this.syncedProcessMainThreadOnlyCheckBox.TabIndex = 4;
			this.syncedProcessMainThreadOnlyCheckBox.Text = "Synced Process\r\nMain Thread Only";
			this.syncedProcessMainThreadOnlyCheckBox.UseVisualStyleBackColor = true;
			// 
			// newButton
			// 
			this.newButton.Location = new System.Drawing.Point(12, 183);
			this.newButton.Name = "newButton";
			this.newButton.Size = new System.Drawing.Size(127, 23);
			this.newButton.TabIndex = 6;
			this.newButton.Text = "New (Ctrl + N)";
			this.newButton.UseVisualStyleBackColor = true;
			this.newButton.Click += new System.EventHandler(this.newButton_Click);
			// 
			// goButton
			// 
			this.goButton.Location = new System.Drawing.Point(145, 183);
			this.goButton.Name = "goButton";
			this.goButton.Size = new System.Drawing.Size(127, 23);
			this.goButton.TabIndex = 7;
			this.goButton.Text = "Go (G)";
			this.goButton.UseVisualStyleBackColor = true;
			this.goButton.Click += new System.EventHandler(this.goButton_Click);
			// 
			// newOpenFileDialog
			// 
			this.newOpenFileDialog.Filter = "Executable Files (*.exe)|*.exe|All Files (*.*)|*.*";
			// 
			// currentMhzLabel
			// 
			this.currentMhzLabel.AutoSize = true;
			this.currentMhzLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.currentMhzLabel.Location = new System.Drawing.Point(12, 215);
			this.currentMhzLabel.Name = "currentMhzLabel";
			this.currentMhzLabel.Size = new System.Drawing.Size(101, 13);
			this.currentMhzLabel.TabIndex = 6;
			this.currentMhzLabel.Text = "Current Rate (MHz:)";
			// 
			// targetMhzLabel
			// 
			this.targetMhzLabel.AutoSize = true;
			this.targetMhzLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.targetMhzLabel.Location = new System.Drawing.Point(12, 234);
			this.targetMhzLabel.Name = "targetMhzLabel";
			this.targetMhzLabel.Size = new System.Drawing.Size(116, 13);
			this.targetMhzLabel.TabIndex = 8;
			this.targetMhzLabel.Text = "Target Rate (MHz:)";
			// 
			// currentMhzValueLabel
			// 
			this.currentMhzValueLabel.AutoSize = true;
			this.currentMhzValueLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.currentMhzValueLabel.Location = new System.Drawing.Point(145, 215);
			this.currentMhzValueLabel.Name = "currentMhzValueLabel";
			this.currentMhzValueLabel.Size = new System.Drawing.Size(116, 13);
			this.currentMhzValueLabel.TabIndex = 9;
			this.currentMhzValueLabel.Text = "currentRateValueLabel";
			// 
			// Form1
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(284, 423);
			this.Controls.Add(this.currentMhzValueLabel);
			this.Controls.Add(this.targetMhzLabel);
			this.Controls.Add(this.currentMhzLabel);
			this.Controls.Add(this.goButton);
			this.Controls.Add(this.newButton);
			this.Controls.Add(this.syncedProcessMainThreadOnlyCheckBox);
			this.Controls.Add(this.optionsGroupBox);
			this.Controls.Add(this.subtitleLabel);
			this.Controls.Add(this.titleLabel);
			this.Controls.Add(this.recentFilesGroupBox);
			this.Controls.Add(this.targetMhzComboBox);
			this.KeyPreview = true;
			this.MaximizeBox = false;
			this.Name = "Form1";
			this.Text = "Old CPU Emulator 1.4.0";
			this.Load += new System.EventHandler(this.Form1_Load);
			this.recentFilesGroupBox.ResumeLayout(false);
			this.optionsGroupBox.ResumeLayout(false);
			this.optionsGroupBox.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.refreshHzNumericUpDown)).EndInit();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.ListBox recentFilesListBox;
		private System.Windows.Forms.Label titleLabel;
		private System.Windows.Forms.Label subtitleLabel;
		private System.Windows.Forms.GroupBox recentFilesGroupBox;
		private System.Windows.Forms.GroupBox optionsGroupBox;
		private System.Windows.Forms.CheckBox setProcessPriorityHighCheckBox;
		private System.Windows.Forms.Label refreshHzLabel;
		private System.Windows.Forms.ComboBox targetMhzComboBox;
		private System.Windows.Forms.CheckBox setSyncedProcessAffinityOneCheckBox;
		private System.Windows.Forms.CheckBox syncedProcessMainThreadOnlyCheckBox;
		private System.Windows.Forms.CheckBox refreshRateFloorFifteenCheckBox;
		private System.Windows.Forms.Button newButton;
		private System.Windows.Forms.Button goButton;
		private System.Windows.Forms.NumericUpDown refreshHzNumericUpDown;
		private System.Windows.Forms.OpenFileDialog newOpenFileDialog;
		private System.Windows.Forms.Label currentMhzLabel;
		private System.Windows.Forms.Label targetMhzLabel;
		private System.Windows.Forms.Label currentMhzValueLabel;
	}
}

