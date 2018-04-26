using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace OldCPUEmulatorGUI
{
	public partial class Form1 : Form
	{
		public Form1()
		{
			InitializeComponent();
		}

		private void Form1_Load(object sender, EventArgs e)
		{
			targetMhzComboBox.SelectedIndex = 0;
			floorRefreshRateFifteen();
			recentFilesListBox.Items.Insert(0, Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString0);
			recentFilesListBox.Items.Insert(1, Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString1);
			recentFilesListBox.Items.Insert(2, Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString2);
			recentFilesListBox.Items.Insert(3, Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString3);
			recentFilesListBox.Items.Insert(4, Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString4);
			recentFilesListBox.Items.Insert(5, Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString5);
			recentFilesListBox.Items.Insert(6, Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString6);
			recentFilesListBox.Items.Insert(7, Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString7);
			recentFilesListBox.Items.Insert(8, Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString8);
			recentFilesListBox.Items.Insert(9, Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString9);
		}

		protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
		{
			if (keyData == (Keys.Control | Keys.N))
			{
				newButton_Click();
				return true;
			}
			if (keyData == (Keys.G))
			{
				goButton_Click();
				return true;
			}
			return base.ProcessCmdKey(ref msg, keyData);
		}

		private bool getCurrentMhz(out long currentMhz)
		{
			currentMhz = 0;
			// create the Get Current Mhz Process to get the Current Rate
			ProcessStartInfo oldCPUEmulatorProcessStartInfo = new ProcessStartInfo("OldCPUEmulator.exe", "--dev-get-current-mhz");
			oldCPUEmulatorProcessStartInfo.UseShellExecute = false;
			oldCPUEmulatorProcessStartInfo.RedirectStandardError = false;
			oldCPUEmulatorProcessStartInfo.RedirectStandardOutput = true;
			oldCPUEmulatorProcessStartInfo.RedirectStandardInput = false;
			oldCPUEmulatorProcessStartInfo.WindowStyle = ProcessWindowStyle.Hidden;
			oldCPUEmulatorProcessStartInfo.CreateNoWindow = true;
			oldCPUEmulatorProcessStartInfo.ErrorDialog = false;
			oldCPUEmulatorProcessStartInfo.WorkingDirectory = Environment.CurrentDirectory;
			try
			{
				Process oldCPUEmulatorProcess = new Process();
				oldCPUEmulatorProcess.StartInfo = oldCPUEmulatorProcessStartInfo;
				oldCPUEmulatorProcess.Start();
				string oldCPUEmulatorProcessStandardOutput = oldCPUEmulatorProcess.StandardOutput.ReadToEnd();
				oldCPUEmulatorProcess.WaitForExit();
				if (oldCPUEmulatorProcess.ExitCode != 0 || !long.TryParse(oldCPUEmulatorProcessStandardOutput.Split('\n').Last(), out currentMhz))
				{
					MessageBox.Show("Failed to get the Current Rate");
					return false;
				}

				// set the Current Rate Value Label's Text to the Current Rate String
				currentMhzValueLabel.Text = currentMhz.ToString();
			}
			catch (Exception e)
			{
				MessageBox.Show("Failed to get the Current Rate");
				return false;
			}
			return true;
		}

		private bool getCurrentMhz()
		{
			long currentMhz = 0;
			return getCurrentMhz(out currentMhz);
		}

		private bool getMhz(out long targetMhz, out long currentMhz)
		{
			targetMhz = 0;
			currentMhz = 0;
			if (!getCurrentMhz(out currentMhz))
			{
				Application.Exit();
				return false;
			}
			// ensure the Target Rate Combo Box's Selected Item is less than the Current Rate
			switch (targetMhzComboBox.SelectedIndex)
			{
				case 0:
					targetMhz = 233;
					break;
				case 1:
					targetMhz = 750;
					break;
				default:
					if (!long.TryParse(targetMhzComboBox.Text, out targetMhz))
					{
						MessageBox.Show("The Target Rate must be a number.");
						//targetMhzComboBox.Text = (currentMhz - 1).ToString();
						return false;
					}
					break;
			}
			if (currentMhz <= targetMhz)
			{
				MessageBox.Show("The Target Rate cannot exceed or equal the Current Rate of " + currentMhzValueLabel.Text + ".");
				//targetMhzComboBox.SelectedIndex = 2;
				//targetMhzComboBox.Text = currentMhzValueLabel.Text;
				return false;
			}
			return true;
		}

		private bool createOldCPUEmulatorProcess()
		{
			// create Arguments for the Old CPU Emulator Process Start Info
			string oldCPUEmulatorProcessStartInfoArguments = "\"" + recentFilesListBox.GetItemText(recentFilesListBox.SelectedItem) + "\"";

			long targetMhz = 0;
			long currentMhz = 0;
			if (!getMhz(out targetMhz, out currentMhz))
			{
				return false;
			}
			oldCPUEmulatorProcessStartInfoArguments += " -t " + targetMhz;

			oldCPUEmulatorProcessStartInfoArguments += " -r " + refreshHzNumericUpDown.Value;
			if (setProcessPriorityHighCheckBox.Checked)
			{
				oldCPUEmulatorProcessStartInfoArguments += " --set-process-priority-high";
			}
			if (setSyncedProcessAffinityOneCheckBox.Checked)
			{
				oldCPUEmulatorProcessStartInfoArguments += " --set-synced-process-affinity-one";
			}
			if (syncedProcessMainThreadOnlyCheckBox.Checked)
			{
				oldCPUEmulatorProcessStartInfoArguments += " --synced-process-main-thread-only";
			}
			if (refreshRateFloorFifteenCheckBox.Checked)
			{
				oldCPUEmulatorProcessStartInfoArguments += " --refresh-rate-floor-fifteen";
			}

			// create the Old CPU Emulator Process Start Info
			ProcessStartInfo oldCPUEmulatorProcessStartInfo = new ProcessStartInfo("OldCPUEmulator.exe", oldCPUEmulatorProcessStartInfoArguments);
			oldCPUEmulatorProcessStartInfo.UseShellExecute = false;
			oldCPUEmulatorProcessStartInfo.RedirectStandardError = true;
			oldCPUEmulatorProcessStartInfo.RedirectStandardOutput = false;
			oldCPUEmulatorProcessStartInfo.RedirectStandardInput = false;
			oldCPUEmulatorProcessStartInfo.WindowStyle = ProcessWindowStyle.Hidden;
			oldCPUEmulatorProcessStartInfo.CreateNoWindow = true;
			oldCPUEmulatorProcessStartInfo.ErrorDialog = false;
			oldCPUEmulatorProcessStartInfo.WorkingDirectory = Environment.CurrentDirectory;
			try
			{
				// create the Old CPU Emulator Process
				Process oldCPUEmulatorProcess = new Process();
				oldCPUEmulatorProcess.StartInfo = oldCPUEmulatorProcessStartInfo;
				oldCPUEmulatorProcess.Start();
				string oldCPUEmulatorProcessStandardError = oldCPUEmulatorProcess.StandardError.ReadToEnd();
				oldCPUEmulatorProcess.WaitForExit();
				switch (oldCPUEmulatorProcess.ExitCode)
				{
					case 0:
						break;
					case -1:
						MessageBox.Show(oldCPUEmulatorProcessStandardError.Split('\n').Last());
						return false;
					case -2:
						MessageBox.Show("You cannot run multiple instances of Old CPU Emulator.");
						return false;
					case -3:
						MessageBox.Show("Failed to create a new C String");
						return false;
					case -4:
						MessageBox.Show("Failed to set a C String");
						return false;
					default:
						MessageBox.Show("Failed to emulate the old CPU");
						return false;
				}
			}
			catch (Exception e)
			{
				MessageBox.Show("Failed to create the Old CPU Emulator Process");
				return false;
			}
			return true;
		}

		void floorRefreshRateFifteen()
		{
			long currentMhz = 0;
			long targetMhz = 0;
			if (!getMhz(out targetMhz, out currentMhz))
			{
				return;
			}

			double suspend = ((double)(currentMhz - targetMhz) / (double)currentMhz);
			double resume = ((double)targetMhz / (double)currentMhz);

			refreshHzNumericUpDown.Increment = 1;
			refreshHzNumericUpDown.Minimum = 1;
			refreshHzNumericUpDown.Maximum = 1000;

			// if we're suspended 3 / 4
			// and resumed 1 / 4
			// we'll be suspended a minimum of 3 Ms
			// and resumed a minimum of 1 Ms
			// (3 / 4) / (1 / 4) = 3 Ms
			// 3 Ms + 1 Ms = 4 Ms, our minRefreshMs
			double minRefreshMs = ((double)(Math.Max(suspend, resume)) / (double)(Math.Min(suspend, resume)) * (double)refreshHzNumericUpDown.Minimum) + (double)refreshHzNumericUpDown.Minimum;
			double maxRefreshHz = (double)refreshHzNumericUpDown.Maximum / Math.Ceiling(minRefreshMs);
			refreshHzNumericUpDown.Value = MathUtils.clamp(Math.Min((uint)refreshHzNumericUpDown.Value, (uint)maxRefreshHz), (uint)refreshHzNumericUpDown.Minimum, (uint)refreshHzNumericUpDown.Maximum);
			refreshHzNumericUpDown.Maximum = Math.Max((uint)maxRefreshHz, refreshHzNumericUpDown.Minimum);
			// we do this after in case the Refresh Rate before was well above the maximum
			if (refreshRateFloorFifteenCheckBox.Checked)
			{
				refreshHzNumericUpDown.Increment = 15;
				refreshHzNumericUpDown.Value = MathUtils.clamp((uint)Math.Floor(refreshHzNumericUpDown.Value / 15) * 15, (uint)refreshHzNumericUpDown.Minimum, (uint)refreshHzNumericUpDown.Maximum);
				refreshHzNumericUpDown.Maximum = MathUtils.clamp((uint)Math.Floor(refreshHzNumericUpDown.Maximum / 15) * 15, (uint)refreshHzNumericUpDown.Minimum, (uint)refreshHzNumericUpDown.Maximum);
			}
		}

		private void newButton_Click()
		{
			if (newOpenFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
			{
				for (byte i = 0; i < 10; i++)
				{
					if (newOpenFileDialog.FileName == recentFilesListBox.Items[i].ToString())
					{
						recentFilesListBox.Items.RemoveAt(i);
						break;
					}
					if (i == 9)
					{
						recentFilesListBox.Items.RemoveAt(9);
					}
				}
				recentFilesListBox.Items.Insert(0, newOpenFileDialog.FileName);
				recentFilesListBox.SelectedIndex = 0;
				Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString0 = recentFilesListBox.Items[0].ToString();
				Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString1 = recentFilesListBox.Items[1].ToString();
				Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString2 = recentFilesListBox.Items[2].ToString();
				Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString3 = recentFilesListBox.Items[3].ToString();
				Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString4 = recentFilesListBox.Items[4].ToString();
				Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString5 = recentFilesListBox.Items[5].ToString();
				Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString6 = recentFilesListBox.Items[6].ToString();
				Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString7 = recentFilesListBox.Items[7].ToString();
				Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString8 = recentFilesListBox.Items[8].ToString();
				Properties.Settings.Default.oldCPUEmulatorSaveDataRecentFilesListBoxItemString9 = recentFilesListBox.Items[9].ToString();
				Properties.Settings.Default.Save();
				createOldCPUEmulatorProcess();
			}
		}

		private void newButton_Click(object sender, EventArgs e)
		{
			newButton_Click();
		}

		private void goButton_Click()
		{
			createOldCPUEmulatorProcess();
		}

		private void goButton_Click(object sender, EventArgs e)
		{
			goButton_Click();
		}

		private void targetMhzComboBox_SelectedIndexChanged(object sender, EventArgs e)
		{
			targetMhzComboBox.DropDownStyle = ComboBoxStyle.DropDownList;
			if (targetMhzComboBox.SelectedIndex == targetMhzComboBox.Items.Count - 1)
			{
				targetMhzComboBox.DropDownStyle = ComboBoxStyle.DropDown;
			}
			floorRefreshRateFifteen();
			refreshHzNumericUpDown.Value = refreshHzNumericUpDown.Maximum;
		}

		private void targetMhzComboBox_TextUpdate(object sender, EventArgs e)
		{
			floorRefreshRateFifteen();
		}

		private void refreshRateFloorFifteenCheckBox_CheckedChanged(object sender, EventArgs e)
		{
			floorRefreshRateFifteen();
		}
	}
}
