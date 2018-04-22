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

const UINT UWM_EMULATE_OLD_CPUS_SYNC_PROCESS = RegisterWindowMessage("UWM_EMULATE_OLD_CPUS_SYNC_PROCESS");

inline void OutputDebugStringNewline(LPCSTR lpOutputString) {
	OutputDebugString(lpOutputString);
	OutputDebugString("\n");
}

void help() {
	OutputDebugStringNewline("\tThis command line tool emulates running a process on a CPU with a");
	OutputDebugStringNewline("\tslower clock speed in order to make old games run at the correct speed");
	OutputDebugStringNewline("\tor underclock CPU intensive processes like video encoding.\n\n");
	OutputDebugStringNewline("Usage: OldCPUEmulator pathname.exe -t targetRate [options]\n");
	OutputDebugStringNewline("-t targetRate");
	OutputDebugStringNewline("\tThe Target Rate (in MHz, from 1 to your CPU's current clock speed) to emulate.");
	OutputDebugStringNewline("\tThis argument is required.\n");
	OutputDebugStringNewline("\tTry 233 to emulate an Intel Pentium II 233 MHz from the late 1990s.");
	OutputDebugStringNewline("\tTry 750 to emulate an Intel Pentium III 750 MHz from the early 2000s.\n");
	OutputDebugStringNewline("\tGo to http://intel.com/pressroom/kits/quickrefyr.htm");
	OutputDebugStringNewline("\tfor a quick reference of year to clock speed.");
	OutputDebugStringNewline("\tNote that many of the measurements in said reference");
	OutputDebugStringNewline("\tare in GHz. This tool uses MHz.\n");
	OutputDebugStringNewline("-r refreshRate");
	OutputDebugStringNewline("\tThe Refresh Rate (in Hz, from 1 to 1000) at which to refresh.");
	OutputDebugStringNewline("\tThis argument is not required.\n");
	OutputDebugStringNewline("\tEffectively an accuracy meter.");
	OutputDebugStringNewline("\tLower numbers are more accurate but result in choppier playback.");
	OutputDebugStringNewline("\tHigher numbers are less accurate but result in smoother playback.");
	OutputDebugStringNewline("\tIf not specified, Old CPU Emulator will default to the\n");
	OutputDebugStringNewline("\tsmoothest possible playback setting.\n");
	OutputDebugStringNewline("\tTry 60, 30 or 15 for gaming, 1 to 6 for video encoding.\n");
	OutputDebugStringNewline("--set-process-priority-high");
	OutputDebugStringNewline("\tSet the process priority of Old CPU Emulator to High,");
	OutputDebugStringNewline("\tin order to potentially improve the accuracy of the emulation.\n");
	OutputDebugStringNewline("--set-synced-process-affinity-one");
	OutputDebugStringNewline("\tSet the process affinity of the synced process");
	OutputDebugStringNewline("\tto one, which may make the speed more consistent and prevent crashes.");
	OutputDebugStringNewline("\tMay be unstable with newer games.\n");
	OutputDebugStringNewline("--synced-process-main-thread-only");
	OutputDebugStringNewline("\tTry enabling this if the process you're running");
	OutputDebugStringNewline("\tseems to be barely affected by Old CPU Emulator,");
	OutputDebugStringNewline("\tas it may increase accuracy on some Windows versions,");
	OutputDebugStringNewline("\tas well as reduce audio stutters,");
	OutputDebugStringNewline("\tbut could also introduce instability with some games.\n");
	OutputDebugStringNewline("--refresh-rate-floor-fifteen");
	OutputDebugStringNewline("\tRounds Refresh Rate to the nearest multiple of 15 if applicable.");
}

inline int clamp(UINT number, UINT min, UINT max) {
	return min(max(min, number), max);
}