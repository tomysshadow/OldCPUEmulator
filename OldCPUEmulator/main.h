#pragma once
#include <string>
#include <vector>
#include <math.h>
#include <windows.h>
#include <ntstatus.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <Powerbase.h>
#include <PowrProf.h>

// we will be using some undocumented NtDll features if possible because they're faster
// we need this program to be super optimized
typedef NTSTATUS(NTAPI *NTQUERYSYSTEMINFORMATION)(__in SYSTEM_INFORMATION_CLASS SystemInformationClass, __inout PVOID SystemInformation, __in ULONG SystemInformationLength, __out_opt PULONG ReturnLength);
typedef NTSTATUS(NTAPI *NTSUSPENDPROCESS)(IN HANDLE ProcessHandle);
typedef NTSTATUS(NTAPI *NTRESUMEPROCESS)(IN HANDLE ProcessHandle);

typedef struct _PROCESSOR_POWER_INFORMATION {
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE hConsoleError = GetStdHandle(STD_ERROR_HANDLE);
const UINT UWM_EMULATE_OLD_CPUS_SYNC_PROCESS = RegisterWindowMessage("UWM_EMULATE_OLD_CPUS_SYNC_PROCESS");

void WriteConsole(const char *lpBuffer, int newline = true, int tab = false, bool handle = false) {
	HANDLE hConsole = INVALID_HANDLE_VALUE;
	if (!handle) {
		hConsole = hConsoleOutput;
	} else {
		hConsole = hConsoleError;
	}
	if (hConsole && hConsole != INVALID_HANDLE_VALUE) {
		for (int i = 0;i < tab;i++) {
			WriteConsole("\t", false);
		}
		// override to make the function more convenient and optionally add a newline
		WriteConsole(hConsole, lpBuffer, strlen(lpBuffer), NULL, NULL);
		for (int i = 0;i < newline;i++) {
			WriteConsole("\n", false);
		}
	}
}

void help() {
	 WriteConsole("This command line tool emulates running a process on a CPU with a", true, true);
	 WriteConsole("slower clock speed in order to make old games run at the correct speed", true, true);
	 WriteConsole("or underclock CPU intensive processes like video encoding.", 3, true);


	WriteConsole("Usage: OldCPUEmulator pathname.exe -t targetRate [options]", 2);

	WriteConsole("-t targetRate");
	 WriteConsole("The Target Rate (in MHz, from 1 to your CPU's current clock speed)", true, true);
	 WriteConsole("to emulate.", true, true);
	 WriteConsole("This argument is required.", 2, true);

	 WriteConsole("Try 233 to emulate an Intel Pentium II 233 MHz from the late 1990s.", true, true);
	 WriteConsole("Try 750 to emulate an Intel Pentium III 750 MHz from the early 2000s.", 2, true);

	 WriteConsole("Go to http://intel.com/pressroom/kits/quickrefyr.htm", true, true);
	 WriteConsole("for a quick reference of year to clock speed.", true, true);
	 WriteConsole("Note that many of the measurements in said reference", true, true);
	 WriteConsole("are in GHz. This tool uses MHz.", 2, true);

	WriteConsole("-r refreshRate");
	 WriteConsole("The Refresh Rate (in Hz, from 1 to 1000) at which to refresh.", true, true);
	 WriteConsole("This argument is not required.", 2, true);

	 WriteConsole("Effectively an accuracy meter.", true, true);
	 WriteConsole("Lower numbers are more accurate but result in choppier playback.", true, true);
	 WriteConsole("Higher numbers are less accurate but result in smoother playback.", true, true);
	 WriteConsole("If not specified, Old CPU Emulator will default to the", true, true);
	 WriteConsole("smoothest possible playback setting.", 2, true);

	 WriteConsole("Try 60, 30 or 15 for gaming, 1 to 6 for video encoding.", 2, true);

	WriteConsole("--set-process-priority-high");
	 WriteConsole("Set the process priority of Old CPU Emulator to High,", true, true);
	 WriteConsole("in order to potentially improve the accuracy of the emulation.", 2, true);

	WriteConsole("--set-synced-process-affinity-one");
	 WriteConsole("Set the process affinity of the synced process", true, true);
	 WriteConsole("to one, which may make the speed more consistent and prevent crashes.", true, true);
	 WriteConsole("May be unstable with newer games.", 2, true);

	WriteConsole("--synced-process-main-thread-only");
	 WriteConsole("Try enabling this if the process you're running", true, true);
	 WriteConsole("seems to be barely affected by Old CPU Emulator,", true, true);
	 WriteConsole("as it may increase accuracy on some Windows versions,", true, true);
	 WriteConsole("as well as reduce audio stutters,", true, true);
	 WriteConsole("but could also introduce instability with some games.", 2, true);

	WriteConsole("--refresh-rate-floor-fifteen");
	 WriteConsole("Rounds Refresh Rate to the nearest multiple of 15 if applicable.", true, true);
}

inline UINT clamp(UINT number, UINT min, UINT max) {
	return min(max(min, number), max);
}