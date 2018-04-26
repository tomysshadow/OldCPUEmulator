#pragma once
#include <iostream>
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

/*
HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE hConsoleError = GetStdHandle(STD_ERROR_HANDLE);
*/
const UINT UWM_EMULATE_OLD_CPUS_SYNC_PROCESS = RegisterWindowMessage("UWM_EMULATE_OLD_CPUS_SYNC_PROCESS");

void consoleLog(const char *buffer, int newline = true, int tab = false, bool err = false) {
	for (int i = 0;i < tab;i++) {
		consoleLog("\t", false, false, err);
	}
	if (!err) {
		std::cout << buffer;
		for (int i = 0;i < newline;i++) {
			std::cout << std::endl;
		}
	} else {
		std::cerr << buffer;
		for (int i = 0;i < newline;i++) {
			std::cerr << std::endl;
		}
	}
}

void help() {
	 consoleLog("This command line tool emulates running a process on a CPU with a", true, true);
	 consoleLog("slower clock speed in order to make old games run at the correct speed", true, true);
	 consoleLog("or underclock CPU intensive processes like video encoding.", 3, true);


	consoleLog("Usage: OldCPUEmulator pathname.exe -t targetRate [options]", 2);

	consoleLog("-t targetRate");
	 consoleLog("The Target Rate (in MHz, from 1 to your CPU's current clock speed)", true, true);
	 consoleLog("to emulate.", true, true);
	 consoleLog("This argument is required.", 2, true);

	 consoleLog("Try 233 to emulate an Intel Pentium II 233 MHz from the late 1990s.", true, true);
	 consoleLog("Try 750 to emulate an Intel Pentium III 750 MHz from the early 2000s.", 2, true);

	 consoleLog("Go to http://intel.com/pressroom/kits/quickrefyr.htm", true, true);
	 consoleLog("for a quick reference of year to clock speed.", true, true);
	 consoleLog("Note that many of the measurements in said reference", true, true);
	 consoleLog("are in GHz. This tool uses MHz.", 2, true);

	consoleLog("-r refreshRate");
	 consoleLog("The Refresh Rate (in Hz, from 1 to 1000) at which to refresh.", true, true);
	 consoleLog("This argument is not required.", 2, true);

	 consoleLog("Effectively an accuracy meter.", true, true);
	 consoleLog("Lower numbers are more accurate but result in choppier playback.", true, true);
	 consoleLog("Higher numbers are less accurate but result in smoother playback.", true, true);
	 consoleLog("If not specified, Old CPU Emulator will default to the", true, true);
	 consoleLog("smoothest possible playback setting.", 2, true);

	 consoleLog("Try 60, 30 or 15 for gaming, 1 to 6 for video encoding.", 2, true);

	consoleLog("--set-process-priority-high");
	 consoleLog("Set the process priority of Old CPU Emulator to High,", true, true);
	 consoleLog("in order to potentially improve the accuracy of the emulation.", 2, true);

	consoleLog("--set-synced-process-affinity-one");
	 consoleLog("Set the process affinity of the synced process", true, true);
	 consoleLog("to one, which may make the speed more consistent and prevent crashes.", true, true);
	 consoleLog("May be unstable with newer games.", 2, true);

	consoleLog("--synced-process-main-thread-only");
	 consoleLog("Try enabling this if the process you're running", true, true);
	 consoleLog("seems to be barely affected by Old CPU Emulator,", true, true);
	 consoleLog("as it may increase accuracy on some Windows versions,", true, true);
	 consoleLog("as well as reduce audio stutters,", true, true);
	 consoleLog("but could also introduce instability with some games.", 2, true);

	consoleLog("--refresh-rate-floor-fifteen");
	 consoleLog("Rounds Refresh Rate to the nearest multiple of 15 if applicable.", true, true);
}

inline UINT clamp(UINT number, UINT min, UINT max) {
	return min(max(min, number), max);
}