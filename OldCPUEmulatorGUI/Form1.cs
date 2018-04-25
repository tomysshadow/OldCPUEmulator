using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace OldCPUEmulatorGUI
{
	public partial class Form1 : Form
	{
		public Form1()
		{
			InitializeComponent();
		}

		private void getCurrentRate()
		{
			// get the current rate value
			Process oldCPUEmulatorProcess = new Process();
			oldCPUEmulatorProcess.StartInfo.UseShellExecute = false;
			oldCPUEmulatorProcess.StartInfo.RedirectStandardOutput = true;
			oldCPUEmulatorProcess.StartInfo.RedirectStandardError = false;
			oldCPUEmulatorProcess.StartInfo.CreateNoWindow = true;
			oldCPUEmulatorProcess.StartInfo.FileName = "OldCPUEmulator.exe";
			oldCPUEmulatorProcess.StartInfo.Arguments = "--dev-get-current-mhz";
			try
			{
				oldCPUEmulatorProcess.Start();
				string oldCPUEmulatorProcessStandardOutput = oldCPUEmulatorProcess.StandardOutput.ReadToEnd();
				oldCPUEmulatorProcess.WaitForExit();
				if (oldCPUEmulatorProcess.ExitCode != 0)
				{
					MessageBox.Show("Failed to get Current Rate");
					Application.Exit();
				}
				currentMhzValueLabel.Text = oldCPUEmulatorProcessStandardOutput.Split('\n').Reverse().ElementAt(0);
			}
			catch (Exception e)
			{
				MessageBox.Show(e.Message);
				Application.Exit();
			}
		}

		private void runProcess()
		{
			// run process
			Process oldCPUEmulatorProcess = new Process();
			oldCPUEmulatorProcess.StartInfo.UseShellExecute = false;
			oldCPUEmulatorProcess.StartInfo.RedirectStandardOutput = false;
			oldCPUEmulatorProcess.StartInfo.RedirectStandardError = true;
			oldCPUEmulatorProcess.StartInfo.CreateNoWindow = true;
			oldCPUEmulatorProcess.StartInfo.FileName = "OldCPUEmulator.exe";
			oldCPUEmulatorProcess.StartInfo.Arguments = "\"" + recentFilesListBox.GetItemText(recentFilesListBox.SelectedItem) + "\" -t";
			switch (targetMhzComboBox.SelectedIndex)
			{
				case 0:
					oldCPUEmulatorProcess.StartInfo.Arguments += " 233";
					break;
				case 1:
					oldCPUEmulatorProcess.StartInfo.Arguments += " 750";
					break;
				case 2:
					long targetMhz = 0;
					if (long.TryParse(targetMhzComboBox.GetItemText(targetMhzComboBox.SelectedItem), out targetMhz))
					{
						oldCPUEmulatorProcess.StartInfo.Arguments += " " + targetMhz;
					}
					else
					{
						MessageBox.Show("Target Rate must be a number.");
						return;
					}
					break;
			}
			oldCPUEmulatorProcess.StartInfo.Arguments += " -r " + refreshRateNumericUpDown.Value;
			if (setProcessPriorityHighCheckBox.Checked)
			{
				oldCPUEmulatorProcess.StartInfo.Arguments += " --set-process-priority-high";
			}
			if (setSyncedProcessAffinityOneCheckBox.Checked)
			{
				oldCPUEmulatorProcess.StartInfo.Arguments += " --set-synced-process-affinity-one";
			}
			if (syncedProcessMainThreadOnlyCheckBox.Checked)
			{
				oldCPUEmulatorProcess.StartInfo.Arguments += " --synced-process-main-thread-only";
			}
			if (refreshRateFloorFifteenCheckBox.Checked)
			{
				oldCPUEmulatorProcess.StartInfo.Arguments += " --refresh-rate-floor-fifteen";
			}
			try
			{
				oldCPUEmulatorProcess.Start();
				string oldCPUEmulatorProcessStandardError = oldCPUEmulatorProcess.StandardError.ReadToEnd();
				oldCPUEmulatorProcess.WaitForExit();
				switch (oldCPUEmulatorProcess.ExitCode)
				{
					case 0:
					break;
					case -1:
					MessageBox.Show(oldCPUEmulatorProcessStandardError.Split('\n').Reverse().ElementAt(0));
					return;
					case -2:
					MessageBox.Show("You cannot run multiple instances of Old CPU Emulator.");
					return;
					case -3:
					MessageBox.Show("Failed to create a new C String");
					return;
					case -4:
					MessageBox.Show("Failed to set a C String");
					return;
					default:
					MessageBox.Show("Failed to emulate the old CPU");
					return;
				}
			}
			catch (Exception e)
			{
				MessageBox.Show(e.Message);
			}
		}

		private void newButton_Click(object sender, EventArgs e)
		{
			// open new file dialog
			if (newOpenFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
			{
				// insert recent file
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
				runProcess();
			}
		}

		private void goButton_Click(object sender, EventArgs e)
		{
			runProcess();
		}

		private void targetMhzComboBox_SelectedIndexChanged(object sender, EventArgs e)
		{
			// set the numeric up down to match the preset
			targetMhzComboBox.DropDownStyle = ComboBoxStyle.DropDownList;
			if (targetMhzComboBox.SelectedIndex == 2)
			{
				targetMhzComboBox.DropDownStyle = ComboBoxStyle.DropDown;
			}
		}

		private void Form1_Load(object sender, EventArgs e)
		{
			targetMhzComboBox.SelectedIndex = 0;
			getCurrentRate();
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
	}
}
