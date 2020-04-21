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

        private bool GetCurrentMhz(out long currentMhz) {
            currentMhz = 0;

            // create the Get Current Mhz Process to get the Current Rate
            ProcessStartInfo oldCPUSimulatorProcessStartInfo = new ProcessStartInfo("OldCPUSimulator.exe", "--dev-get-current-mhz") {
                UseShellExecute = false,
                RedirectStandardError = false,
                RedirectStandardOutput = true,
                RedirectStandardInput = false,
                WindowStyle = ProcessWindowStyle.Hidden,
                CreateNoWindow = true,
                ErrorDialog = false
            };

            try {
                Process oldCPUSimulatorProcess = new Process {
                    StartInfo = oldCPUSimulatorProcessStartInfo
                };

                oldCPUSimulatorProcess.Start();
                string oldCPUSimulatorProcessStandardOutput = oldCPUSimulatorProcess.StandardOutput.ReadToEnd();

                if (!oldCPUSimulatorProcess.HasExited) {
                    oldCPUSimulatorProcess.WaitForExit();
                }

                if (oldCPUSimulatorProcess.ExitCode != 0 || !long.TryParse(oldCPUSimulatorProcessStandardOutput.Split('\n').Last(), out currentMhz)) {
                    MessageBox.Show("Failed to Get Current Rate");
                    return false;
                }

                // set the Current Rate Value Label's Text to the Current Rate String
                currentMhzValueLabel.Text = currentMhz.ToString();
            } catch {
                MessageBox.Show("Failed to Get Current Rate");
                return false;
            }
            return true;
        }

        private bool GetCurrentMhz() {
            long currentMhz = 0;
            return GetCurrentMhz(out currentMhz);
        }

        private bool GetMhz(out long targetMhz, out long currentMhz) {
            targetMhz = 0;
            currentMhz = 0;

            if (!GetCurrentMhz(out currentMhz)) {
                Application.Exit();
                return false;
            }

            // ensure the Target Rate Combo Box's Selected Item is less than the Current Rate
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
                    //targetMhzComboBox.Text = (currentMhz - 1).ToString();
                    return false;
                }
                break;
            }

            if (targetMhz < 1) {
                MessageBox.Show("The Target Rate cannot be less than one.");
                return false;
            }

            if (currentMhz <= targetMhz) {
                MessageBox.Show("The Target Rate cannot exceed or equal the Current Rate of " + currentMhzValueLabel.Text + ".");
                //targetMhzComboBox.SelectedIndex = 2;
                //targetMhzComboBox.Text = currentMhzValueLabel.Text;
                return false;
            }
            return true;
        }

        private void CreateOldCPUSimulatorProcess() {
            // create Arguments for the Old CPU Simulator Process Start Info
            string oldCPUSimulatorProcessStartInfoArguments = "";

            long targetMhz = 0;
            long currentMhz = 0;

            if (!GetMhz(out targetMhz, out currentMhz)) {
                return;
            }

            oldCPUSimulatorProcessStartInfoArguments += " -t " + targetMhz;
            oldCPUSimulatorProcessStartInfoArguments += " -r " + refreshHzNumericUpDown.Value;

            if (setProcessPriorityHighCheckBox.Checked) {
                oldCPUSimulatorProcessStartInfoArguments += " --set-process-priority-high";
            }

            if (setSyncedProcessAffinityOneCheckBox.Checked) {
                oldCPUSimulatorProcessStartInfoArguments += " --set-synced-process-affinity-one";
            }

            if (syncedProcessMainThreadOnlyCheckBox.Checked) {
                oldCPUSimulatorProcessStartInfoArguments += " --synced-process-main-thread-only";
            }

            if (refreshRateFloorFifteenCheckBox.Checked) {
                oldCPUSimulatorProcessStartInfoArguments += " --refresh-rate-floor-fifteen";
            }

            try {
                string fullPath = Path.GetFullPath(recentFilesListBox.GetItemText(recentFilesListBox.SelectedItem));

                // create the Old CPU Simulator Process Start Info
                oldCPUSimulatorProcessStartInfoArguments += " -sw \"" + fullPath + "\"";

                ProcessStartInfo oldCPUSimulatorProcessStartInfo = new ProcessStartInfo("OldCPUSimulator.exe", oldCPUSimulatorProcessStartInfoArguments) {
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
                Process oldCPUSimulatorProcess = new Process {
                    StartInfo = oldCPUSimulatorProcessStartInfo
                };

                // hide... our laziness with not being async
                Hide();
                oldCPUSimulatorProcess.Start();

                if (!oldCPUSimulatorProcess.HasExited) {
                    oldCPUSimulatorProcess.WaitForExit();
                }

                Show();

                switch (oldCPUSimulatorProcess.ExitCode) {
                    case 0:
                    break;
                    case -1:
                    string oldCPUSimulatorProcessStandardError = oldCPUSimulatorProcess.StandardError.ReadToEnd();
                    string[] lastOldCPUSimulatorProcessStandardErrors = oldCPUSimulatorProcessStandardError.Split('\n');
                    string lastOldCPUSimulatorProcessStandardError = lastOldCPUSimulatorProcessStandardErrors.Length < 2 ? null : lastOldCPUSimulatorProcessStandardErrors[lastOldCPUSimulatorProcessStandardErrors.Length - 2];

                    if (!string.IsNullOrEmpty(lastOldCPUSimulatorProcessStandardError)) {
                        MessageBox.Show(lastOldCPUSimulatorProcessStandardError);
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
            long currentMhz = 0;
            long targetMhz = 0;

            if (!GetMhz(out targetMhz, out currentMhz)) {
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
