using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace OldCPUSimulatorGUI {
    public partial class Form1 : Form {
        ManualResetEvent ProcessExitedManualResetEvent = new ManualResetEvent(false);

        public Form1() {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e) {
            targetMhzComboBox.SelectedIndex = 0;
            floorRefreshRateFifteen();
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
                newButton_Click();
                return true;
            }

            if (keyData == (Keys.G)) {
                goButton_Click();
                return true;
            }
            return base.ProcessCmdKey(ref msg, keyData);
        }

        private void ProcessExited(object sender, EventArgs e) {
            ProcessExitedManualResetEvent.Set();
        }

        private bool getCurrentMhz(out long currentMhz) {
            currentMhz = 0;
            // create the Get Current Mhz Process to get the Current Rate
            ProcessStartInfo oldCPUSimulatorProcessStartInfo = new ProcessStartInfo("OldCPUSimulator.exe", "--dev-get-current-mhz");
            oldCPUSimulatorProcessStartInfo.UseShellExecute = false;
            oldCPUSimulatorProcessStartInfo.RedirectStandardError = false;
            oldCPUSimulatorProcessStartInfo.RedirectStandardOutput = true;
            oldCPUSimulatorProcessStartInfo.RedirectStandardInput = false;
            oldCPUSimulatorProcessStartInfo.WindowStyle = ProcessWindowStyle.Hidden;
            oldCPUSimulatorProcessStartInfo.CreateNoWindow = true;
            oldCPUSimulatorProcessStartInfo.ErrorDialog = false;
            oldCPUSimulatorProcessStartInfo.WorkingDirectory = Environment.CurrentDirectory;

            try {
                Process oldCPUSimulatorProcess = new Process();
                EventHandler processExitedEventHandler = new EventHandler(ProcessExited);
                oldCPUSimulatorProcess.Exited += processExitedEventHandler;
                oldCPUSimulatorProcess.StartInfo = oldCPUSimulatorProcessStartInfo;
                oldCPUSimulatorProcess.Start();
                string oldCPUSimulatorProcessStandardOutput = oldCPUSimulatorProcess.StandardOutput.ReadToEnd();

                if (!oldCPUSimulatorProcess.HasExited) {
                    ProcessExitedManualResetEvent.WaitOne();
                }

                oldCPUSimulatorProcess.Exited -= processExitedEventHandler;
                ProcessExitedManualResetEvent.Reset();

                if (oldCPUSimulatorProcess.ExitCode != 0 || !long.TryParse(oldCPUSimulatorProcessStandardOutput.Split('\n').Last(), out currentMhz)) {
                    MessageBox.Show("Failed to Get Current Rate");
                    return false;
                }

                // set the Current Rate Value Label's Text to the Current Rate String
                currentMhzValueLabel.Text = currentMhz.ToString();
            } catch (Exception) {
                MessageBox.Show("Failed to Get Current Rate");
                return false;
            }
            return true;
        }

        private bool getCurrentMhz() {
            long currentMhz = 0;
            return getCurrentMhz(out currentMhz);
        }

        private bool getMhz(out long targetMhz, out long currentMhz) {
            targetMhz = 0;
            currentMhz = 0;

            if (!getCurrentMhz(out currentMhz)) {
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

        private bool createOldCPUSimulatorProcess() {
            // create Arguments for the Old CPU Simulator Process Start Info
            string oldCPUSimulatorProcessStartInfoArguments = "\"" + recentFilesListBox.GetItemText(recentFilesListBox.SelectedItem) + "\"";

            long targetMhz = 0;
            long currentMhz = 0;

            if (!getMhz(out targetMhz, out currentMhz)) {
                return false;
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

            // create the Old CPU Simulator Process Start Info
            ProcessStartInfo oldCPUSimulatorProcessStartInfo = new ProcessStartInfo("OldCPUSimulator.exe", oldCPUSimulatorProcessStartInfoArguments);
            oldCPUSimulatorProcessStartInfo.UseShellExecute = false;
            oldCPUSimulatorProcessStartInfo.RedirectStandardError = true;
            oldCPUSimulatorProcessStartInfo.RedirectStandardOutput = false;
            oldCPUSimulatorProcessStartInfo.RedirectStandardInput = false;
            oldCPUSimulatorProcessStartInfo.WindowStyle = ProcessWindowStyle.Hidden;
            oldCPUSimulatorProcessStartInfo.CreateNoWindow = true;
            oldCPUSimulatorProcessStartInfo.ErrorDialog = false;
            oldCPUSimulatorProcessStartInfo.WorkingDirectory = Environment.CurrentDirectory;

            try {
                // create the Old CPU Simulator Process
                Process oldCPUSimulatorProcess = new Process();
                EventHandler processExitedEventHandler = new EventHandler(ProcessExited);
                oldCPUSimulatorProcess.Exited += processExitedEventHandler;
                oldCPUSimulatorProcess.StartInfo = oldCPUSimulatorProcessStartInfo;
                oldCPUSimulatorProcess.Start();
                string oldCPUSimulatorProcessStandardError = oldCPUSimulatorProcess.StandardError.ReadToEnd();

                if (!oldCPUSimulatorProcess.HasExited) {
                    ProcessExitedManualResetEvent.WaitOne();
                }

                oldCPUSimulatorProcess.Exited -= processExitedEventHandler;
                ProcessExitedManualResetEvent.Reset();

                switch (oldCPUSimulatorProcess.ExitCode) {
                    case 0:
                    break;
                    case -1:
                    string[] lastOldCPUSimulatorProcessStandardErrors = oldCPUSimulatorProcessStandardError.Split('\n');
                    string lastOldCPUSimulatorProcessStandardError = lastOldCPUSimulatorProcessStandardErrors.Length < 2 ? null : lastOldCPUSimulatorProcessStandardErrors[lastOldCPUSimulatorProcessStandardErrors.Length - 2];

                    if (!string.IsNullOrEmpty(lastOldCPUSimulatorProcessStandardError)) {
                        MessageBox.Show(lastOldCPUSimulatorProcessStandardError);
                    }
                    return false;
                    case -2:
                    MessageBox.Show("You cannot run multiple instances of Old CPU Simulator.");
                    return false;
                    case -3:
                    MessageBox.Show("Failed to Create New String");
                    return false;
                    case -4:
                    MessageBox.Show("Failed to Set String");
                    return false;
                    default:
                    MessageBox.Show("Failed to Simulate Old CPU");
                    return false;
                }
            } catch (Exception) {
                MessageBox.Show("Failed to Create Old CPU Simulator Process");
                return false;
            }
            return true;
        }

        void showRefreshRateMinimumMaximum() {
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

        void floorRefreshRateFifteen() {
            long currentMhz = 0;
            long targetMhz = 0;

            if (!getMhz(out targetMhz, out currentMhz)) {
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

            showRefreshRateMinimumMaximum();
        }

        private void newButton_Click() {
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
                createOldCPUSimulatorProcess();
            }
        }

        private void newButton_Click(object sender, EventArgs e) {
            newButton_Click();
        }

        private void goButton_Click() {
            createOldCPUSimulatorProcess();
        }

        private void goButton_Click(object sender, EventArgs e) {
            goButton_Click();
        }

        private void targetMhzComboBox_SelectedIndexChanged(object sender, EventArgs e) {
            targetMhzComboBox.DropDownStyle = ComboBoxStyle.DropDownList;

            if (targetMhzComboBox.SelectedIndex == targetMhzComboBox.Items.IndexOf("Custom Target Rate...")) {
                targetMhzComboBox.DropDownStyle = ComboBoxStyle.DropDown;
                targetMhzComboBox.Text = "1";
            }

            floorRefreshRateFifteen();
            refreshHzNumericUpDown.Value = refreshHzNumericUpDown.Maximum;
        }

        private void targetMhzComboBox_TextUpdate(object sender, EventArgs e) {
            floorRefreshRateFifteen();
            refreshHzNumericUpDown.Value = refreshHzNumericUpDown.Maximum;
        }

        private void refreshRateFloorFifteenCheckBox_CheckedChanged(object sender, EventArgs e) {
            floorRefreshRateFifteen();
        }

        private void refreshHzNumericUpDown_ValueChanged(object sender, EventArgs e) {
            showRefreshRateMinimumMaximum();
        }
    }
}
