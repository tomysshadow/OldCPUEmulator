using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace OldCPUEmulatorGUI
{
	public static class MathUtils
	{
		public static uint clamp(uint number, uint min, uint max)
		{
			return Math.Min(Math.Max(min, number), max);
		}
	}
	static class Program
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main()
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			Application.Run(new Form1());
		}
	}
}
