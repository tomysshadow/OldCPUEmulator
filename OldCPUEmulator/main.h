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

NTQUERYSYSTEMINFORMATION _NtQuerySystemInformation = NULL;
NTSUSPENDPROCESS _NtSuspendProcess = NULL;
NTRESUMEPROCESS _NtResumeProcess = NULL;
const UINT UWM_EMULATE_OLD_CPUS_SYNC_PROCESS = RegisterWindowMessage("UWM_EMULATE_OLD_CPUS_SYNC_PROCESS");
HANDLE syncedProcess = INVALID_HANDLE_VALUE;
HANDLE syncedProcessMainThread = INVALID_HANDLE_VALUE;
HANDLE oldCPUEmulatorMutex = INVALID_HANDLE_VALUE;
BOOL suspended = FALSE;
UINT ms = 1;
UINT s = 1000;

void help() {
	OutputDebugString("\tThis command line tool emulates running a process on a CPU with a");
	OutputDebugString("\tslower clock speed in order to make old games run at the correct speed");
	OutputDebugString("\tor underclock CPU intensive processes like video encoding.\n\n");
	OutputDebugString("Usage: OldCPUEmulator pathname.exe -t targetRate [options]\n");
	OutputDebugString("-t targetRate");
	OutputDebugString("\tThe Target Rate (in MHz, from 1 to your CPU's clock speed) to emulate.");
	OutputDebugString("\tThis argument is required.");
	OutputDebugString("\tTry 233 to emulate an Intel Pentium II 233 MHz from the late 1990s.\n");
	OutputDebugString("\tTry 750 to emulate an Intel Pentium III 750 MHz from the early 2000s.\n");
	OutputDebugString("\tGo to http://intel.com/pressroom/kits/quickrefyr.htm");
	OutputDebugString("\tfor a quick reference of year to clock speed.");
	OutputDebugString("\tNote that many of the measurements in said reference");
	OutputDebugString("\tare in GHz. This tool uses MHz.\n");
	OutputDebugString("-r refreshRate");
	OutputDebugString("\tThe Refresh Rate (in Hz, from 1 to 1000) at which to refresh.");
	OutputDebugString("\tThis argument is not required.");
	OutputDebugString("\tEffectively an accuracy meter.");
	OutputDebugString("\tLower numbers are more accurate but result in choppier playback.");
	OutputDebugString("\tHigher numbers are less accurate but result in smoother playback.");
	OutputDebugString("\tIf not specified, Old CPU Emulator will default to the smoothest possible playback setting.");
	OutputDebugString("\tTry 60, 30 or 15 for gaming, 1 to 6 for video encoding.\n");
	OutputDebugString("--set-process-priority-high");
	OutputDebugString("\tSet the process priority of Old CPU Emulator to High,");
	OutputDebugString("\tin order to potentially improve the accuracy of the emulation.\n");
	OutputDebugString("--set-synced-process-affinity-one");
	OutputDebugString("\tSet the process affinity of the synced process");
	OutputDebugString("\tto one, which may make the speed more consistent and prevent crashes.");
	OutputDebugString("\tMay be unstable with newer games.\n");
	OutputDebugString("--synced-process-main-thread-only");
	OutputDebugString("\tOnly throttle the speed of the synced process's main thread,");
	OutputDebugString("\twhich may reduce audio stutters, and improve the accuracy of");
	OutputDebugString("\tthe emulation on some Windows versions,");
	OutputDebugString("\tbut could also introduce instability with some games.\n");
	OutputDebugString("--refresh-rate-floor-fifteen");
	OutputDebugString("\tRounds Refresh Rate to the nearest multiple of 15 if applicable.");
}

inline int clamp(UINT number, UINT min, UINT max) {
	return min(max(min, number), max);
}