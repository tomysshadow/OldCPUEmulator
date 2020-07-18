using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace OldCPUSimulatorGUI {
    public partial class OldCPUSimulatorGUI : Form {
        public OldCPUSimulatorGUI() {
            InitializeComponent();
        }

        private void OldCPUSimulatorGUI_Load(object sender, EventArgs e) {
            try {
                Directory.SetCurrentDirectory(Application.StartupPath);
            } catch {
                // Fail silently.
            }

            targetMhzComboBox.SelectedIndex = 0;
            FloorRefreshRateFifteen();
            recentFilesListBox.Items.Insert(0, Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString0);
            recentFilesListBox.Items.Insert(1, Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString1);
            recentFilesListBox.Items.Insert(2, Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString2);
            recentFilesListBox.Items.Insert(3, Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString3);
            recentFilesListBox.Items.Insert(4, Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString4);
            recentFilesListBox.Items.Insert(5, Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString5);
            recentFilesListBox.Items.Insert(6, Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString6);
            recentFilesListBox.Items.Insert(7, Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString7);
            recentFilesListBox.Items.Insert(8, Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString8);
            recentFilesListBox.Items.Insert(9, Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString9);
        }

        protected override bool ProcessCmdKey(ref Message msg, Keys keyData) {
            if (keyData == (Keys.Control | Keys.N)) {
                New();
                return true;
            }

            if (keyData == (Keys.G)) {
                Go();
                return true;
            }
            return base.ProcessCmdKey(ref msg, keyData);
        }

        private bool GetMhzLimit(out long mhzLimit) {
            mhzLimit = 0;

            // create the Get Rate Limit Process to get the Rate Limit
            ProcessStartInfo oldCPUSimulatorProcessStartInfo = new ProcessStartInfo("OldCPUSimulator.exe", "--dev-get-mhz-limit") {
                UseShellExecute = false,
                RedirectStandardError = false,
                RedirectStandardOutput = true,
                RedirectStandardInput = false,
                WindowStyle = ProcessWindowStyle.Hidden,
                CreateNoWindow = true,
                ErrorDialog = false
            };

            try {
                Process oldCPUSimulatorProcess = Process.Start(oldCPUSimulatorProcessStartInfo);

                if (!oldCPUSimulatorProcess.HasExited) {
                    oldCPUSimulatorProcess.WaitForExit();
                }

                string oldCPUSimulatorProcessStandardError = null;
                string oldCPUSimulatorProcessStandardOutput = null;

                if (oldCPUSimulatorProcessStartInfo.RedirectStandardError) {
                    oldCPUSimulatorProcessStandardError = oldCPUSimulatorProcess.StandardError.ReadToEnd();
                }

                if (oldCPUSimulatorProcessStartInfo.RedirectStandardOutput) {
                    oldCPUSimulatorProcessStandardOutput = oldCPUSimulatorProcess.StandardOutput.ReadToEnd();
                }

                if (oldCPUSimulatorProcess.ExitCode != 0 || !long.TryParse(oldCPUSimulatorProcessStandardOutput.Split('\n').Last(), out mhzLimit)) {
                    MessageBox.Show("Failed to Get Rate Limit");
                    return false;
                }

                // set the Rate Limit Value Label's Text to the Rate Limit String
                mhzLimitValueLabel.Text = mhzLimit.ToString();
            } catch {
                MessageBox.Show("Failed to Get Rate Limit");
                return false;
            }
            return true;
        }

        private bool GetMhzLimit() {
            long mhzLimit = 0;
            return GetMhzLimit(out mhzLimit);
        }

        private bool GetMhz(out long targetMhz, out long mhzLimit) {
            targetMhz = 0;
            mhzLimit = 0;

            if (!GetMhzLimit(out mhzLimit)) {
                Application.Exit();
                return false;
            }

            // ensure the Target Rate Combo Box's Selected Item is less than the Rate Limit
            switch (targetMhzComboBox.SelectedIndex) {
                case 0:
                targetMhz = 233;
                break;
                case 1:
                targetMhz = 350;
                break;
                default:
                if (!long.TryParse(targetMhzComboBox.Text, out targetMhz)) {
                    MessageBox.Show("The Target Rate must be a number.");
                    //targetMhzComboBox.Text = (mhzLimit - 1).ToString();
                    return false;
                }
                break;
            }

            if (targetMhz < 1) {
                MessageBox.Show("The Target Rate cannot be less than one.");
                return false;
            }

            if (mhzLimit <= targetMhz) {
                MessageBox.Show("The Target Rate cannot exceed or equal the Rate Limit of " + mhzLimitValueLabel.Text + ".");
                //targetMhzComboBox.SelectedIndex = 2;
                //targetMhzComboBox.Text = mhzLimitValueLabel.Text;
                return false;
            }
            return true;
        }

        private void CreateOldCPUSimulatorProcess() {
            // create Arguments for the Old CPU Simulator Process Start Info
            StringBuilder oldCPUSimulatorProcessStartInfoArguments = new StringBuilder();

            long targetMhz = 0;
            long mhzLimit = 0;

            if (!GetMhz(out targetMhz, out mhzLimit)) {
                return;
            }

            oldCPUSimulatorProcessStartInfoArguments.Append(" -t ");
            oldCPUSimulatorProcessStartInfoArguments.Append(targetMhz);
            oldCPUSimulatorProcessStartInfoArguments.Append(" -r ");
            oldCPUSimulatorProcessStartInfoArguments.Append(refreshHzNumericUpDown.Value);

            if (setProcessPriorityHighCheckBox.Checked) {
                oldCPUSimulatorProcessStartInfoArguments.Append(" --set-process-priority-high");
            }

            if (setSyncedProcessAffinityOneCheckBox.Checked) {
                oldCPUSimulatorProcessStartInfoArguments.Append(" --set-synced-process-affinity-one");
            }

            if (syncedProcessMainThreadOnlyCheckBox.Checked) {
                oldCPUSimulatorProcessStartInfoArguments.Append(" --synced-process-main-thread-only");
            }

            if (refreshRateFloorFifteenCheckBox.Checked) {
                oldCPUSimulatorProcessStartInfoArguments.Append(" --refresh-rate-floor-fifteen");
            }

            try {
                string fullPath = Path.GetFullPath(recentFilesListBox.GetItemText(recentFilesListBox.SelectedItem));

                // create the Old CPU Simulator Process Start Info
                oldCPUSimulatorProcessStartInfoArguments.Append(" -sw \"");
                oldCPUSimulatorProcessStartInfoArguments.Append(fullPath);
                oldCPUSimulatorProcessStartInfoArguments.Append("\"");

                ProcessStartInfo oldCPUSimulatorProcessStartInfo = new ProcessStartInfo("OldCPUSimulator.exe", oldCPUSimulatorProcessStartInfoArguments.ToString()) {
                    UseShellExecute = false,
                    RedirectStandardError = true,
                    RedirectStandardOutput = false,
                    RedirectStandardInput = false,
                    WindowStyle = ProcessWindowStyle.Hidden,
                    CreateNoWindow = true,
                    ErrorDialog = false,
                    WorkingDirectory = Path.GetDirectoryName(fullPath)
                };

                // create the Old CPU Simulator Process
                // hide... our laziness with not being async
                Hide();
                Process oldCPUSimulatorProcess = Process.Start(oldCPUSimulatorProcessStartInfo);

                if (!oldCPUSimulatorProcess.HasExited) {
                    oldCPUSimulatorProcess.WaitForExit();
                }

                Show();

                string oldCPUSimulatorProcessStandardError = null;
                string oldCPUSimulatorProcessStandardOutput = null;

                if (oldCPUSimulatorProcessStartInfo.RedirectStandardError) {
                    oldCPUSimulatorProcessStandardError = oldCPUSimulatorProcess.StandardError.ReadToEnd();
                }

                if (oldCPUSimulatorProcessStartInfo.RedirectStandardOutput) {
                    oldCPUSimulatorProcessStandardOutput = oldCPUSimulatorProcess.StandardOutput.ReadToEnd();
                }

                switch (oldCPUSimulatorProcess.ExitCode) {
                    case 0:
                    break;
                    case -1:
                    if (!String.IsNullOrEmpty(oldCPUSimulatorProcessStandardError)) {
                        string[] lastOldCPUSimulatorProcessStandardErrors = oldCPUSimulatorProcessStandardError.Split('\n');
                        string lastOldCPUSimulatorProcessStandardError = null;

                        if (lastOldCPUSimulatorProcessStandardErrors.Length > 1) {
                            lastOldCPUSimulatorProcessStandardError = lastOldCPUSimulatorProcessStandardErrors[lastOldCPUSimulatorProcessStandardErrors.Length - 2];
                        }

                        if (!String.IsNullOrEmpty(lastOldCPUSimulatorProcessStandardError)) {
                            MessageBox.Show(lastOldCPUSimulatorProcessStandardError);
                        }
                    }
                    break;
                    case -2:
                    MessageBox.Show("You cannot run multiple instances of Old CPU Simulator.");
                    break;
                    case -3:
                    MessageBox.Show("Failed to Create New String");
                    break;
                    case -4:
                    MessageBox.Show("Failed to Set String");
                    break;
                    default:
                    MessageBox.Show("Failed to Simulate Old CPU");
                    break;
                }
            } catch {
                Show();
                MessageBox.Show("Failed to Create Old CPU Simulator Process");
            }
        }

        void ShowRefreshRateMinimumMaximum() {
            if (refreshHzNumericUpDown.Value == refreshHzNumericUpDown.Maximum) {
                refreshHzMaximumNumericUpDown.Controls[0].Enabled = false;
                refreshHzMaximumGroupBox.Visible = true;
            } else {
                refreshHzMaximumGroupBox.Visible = false;
            }

            if (refreshHzNumericUpDown.Value == refreshHzNumericUpDown.Minimum) {
                refreshHzMinimumNumericUpDown.Controls[0].Enabled = false;
                refreshHzMinimumGroupBox.Visible = true;
            } else {
                refreshHzMinimumGroupBox.Visible = false;
            }
        }

        void FloorRefreshRateFifteen() {
            long mhzLimit = 0;
            long targetMhz = 0;

            if (!GetMhz(out targetMhz, out mhzLimit)) {
                return;
            }

            double suspend = ((double)(mhzLimit - targetMhz) / (double)mhzLimit);
            double resume = ((double)targetMhz / (double)mhzLimit);

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
            if (refreshRateFloorFifteenCheckBox.Checked) {
                refreshHzNumericUpDown.Minimum = 15;
                refreshHzNumericUpDown.Increment = 15;
                refreshHzNumericUpDown.Value = MathUtils.clamp((uint)Math.Floor(refreshHzNumericUpDown.Value / 15) * 15, (uint)refreshHzNumericUpDown.Minimum, (uint)refreshHzNumericUpDown.Maximum);
                refreshHzNumericUpDown.Maximum = MathUtils.clamp((uint)Math.Floor(refreshHzNumericUpDown.Maximum / 15) * 15, (uint)refreshHzNumericUpDown.Minimum, (uint)refreshHzNumericUpDown.Maximum);
            }

            ShowRefreshRateMinimumMaximum();
        }

        private void New() {
            if (newOpenFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK) {
                for (byte i = 0;i < 10;i++) {
                    if (newOpenFileDialog.FileName == recentFilesListBox.Items[i].ToString()) {
                        recentFilesListBox.Items.RemoveAt(i);
                        break;
                    }

                    if (i == 9) {
                        recentFilesListBox.Items.RemoveAt(9);
                    }
                }

                recentFilesListBox.Items.Insert(0, newOpenFileDialog.FileName);
                recentFilesListBox.SelectedIndex = 0;
                Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString0 = recentFilesListBox.Items[0].ToString();
                Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString1 = recentFilesListBox.Items[1].ToString();
                Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString2 = recentFilesListBox.Items[2].ToString();
                Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString3 = recentFilesListBox.Items[3].ToString();
                Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString4 = recentFilesListBox.Items[4].ToString();
                Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString5 = recentFilesListBox.Items[5].ToString();
                Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString6 = recentFilesListBox.Items[6].ToString();
                Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString7 = recentFilesListBox.Items[7].ToString();
                Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString8 = recentFilesListBox.Items[8].ToString();
                Properties.Settings.Default.oldCPUSimulatorSaveDataRecentFilesListBoxItemString9 = recentFilesListBox.Items[9].ToString();
                Properties.Settings.Default.Save();
                CreateOldCPUSimulatorProcess();
            }
        }

        private void Go() {
            CreateOldCPUSimulatorProcess();
        }

        private void newButton_Click(object sender, EventArgs e) {
            New();
        }

        private void goButton_Click(object sender, EventArgs e) {
            Go();
        }

        private void targetMhzComboBox_SelectedIndexChanged(object sender, EventArgs e) {
            targetMhzComboBox.DropDownStyle = ComboBoxStyle.DropDownList;

            if (targetMhzComboBox.SelectedIndex == targetMhzComboBox.Items.IndexOf("Custom Target Rate...")) {
                targetMhzComboBox.DropDownStyle = ComboBoxStyle.DropDown;
                targetMhzComboBox.Text = "1";
            }

            FloorRefreshRateFifteen();
            refreshHzNumericUpDown.Value = refreshHzNumericUpDown.Maximum;
        }

        private void targetMhzComboBox_TextUpdate(object sender, EventArgs e) {
            FloorRefreshRateFifteen();
            refreshHzNumericUpDown.Value = refreshHzNumericUpDown.Maximum;
        }

        private void refreshRateFloorFifteenCheckBox_CheckedChanged(object sender, EventArgs e) {
            FloorRefreshRateFifteen();
        }

        private void refreshHzNumericUpDown_ValueChanged(object sender, EventArgs e) {
            ShowRefreshRateMinimumMaximum();
        }
    }
}
