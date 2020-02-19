#include "main.h"
#include <vector>
#include <string>
#include <regex>
#include <windows.h>
#include <atlbase.h>
#include <math.h>
#include <ntstatus.h>
#include <TlHelp32.h>
#include <PowrProf.h>
#include <Shlwapi.h>

void getOriginalNtDll(HMODULE &originalNtDll,
	NTQUERYSYSTEMINFORMATION &originalNtQuerySystemInformation,
	NTSUSPENDPROCESS &originalNtSuspendProcess,
	NTRESUMEPROCESS &originalNtResumeProcess) {
	consoleLog("Getting Original NtDll");

	originalNtDll = GetModuleHandle("ntdll.dll");

	if (originalNtDll) {
		originalNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(originalNtDll, "NtQuerySystemInformation");
		originalNtSuspendProcess = (NTSUSPENDPROCESS)GetProcAddress(originalNtDll, "NtSuspendProcess");
		originalNtResumeProcess = (NTRESUMEPROCESS)GetProcAddress(originalNtDll, "NtResumeProcess");
	}
}

bool getCurrentMhz(ULONG &currentMhz) {
	//consoleLog("Getting Current Rate");

	SYSTEM_INFO systemInfo = {};
	GetSystemInfo(&systemInfo);
	const int PROCESSOR_POWER_INFORMATION_SIZE = sizeof(PROCESSOR_POWER_INFORMATION) * systemInfo.dwNumberOfProcessors;
	PVOID processorPowerInformationOutputBufferPointer = new BYTE[PROCESSOR_POWER_INFORMATION_SIZE];

	if (!processorPowerInformationOutputBufferPointer) {
		consoleLog("Failed to Create Processor Power Information Output Buffer", true, false, true);
		return false;
	}

	// TODO: we assume all CPU cores have the same clock speed (it's not normal for anything else to be true right?)
	if (CallNtPowerInformation(ProcessorInformation, NULL, NULL, processorPowerInformationOutputBufferPointer, PROCESSOR_POWER_INFORMATION_SIZE) != STATUS_SUCCESS) {
		consoleLog("Failed to Call NtPowerInformation", true, false, true);
		delete[] processorPowerInformationOutputBufferPointer;
		processorPowerInformationOutputBufferPointer = NULL;
		return false;
	}

	PPROCESSOR_POWER_INFORMATION processorPowerInformationPointer = (PPROCESSOR_POWER_INFORMATION)processorPowerInformationOutputBufferPointer;

	currentMhz = processorPowerInformationPointer->CurrentMhz;

	delete[] processorPowerInformationOutputBufferPointer;
	processorPowerInformationOutputBufferPointer = NULL;
	processorPowerInformationPointer = NULL;
	return true;
}

bool setProcessAffinity(HANDLE processHandle, byte affinity) {
	consoleLog("Setting Process Affinity");

	if (!processHandle || processHandle == INVALID_HANDLE_VALUE) {
		consoleLog("processHandle cannot be NULL or INVALID_HANDLE_VALUE", true, false, true);
		return false;
	}

	// set synced processHandle's affinity
	DWORD_PTR processAffinityMask = NULL;
	DWORD_PTR systemAffinityMask = NULL;

	if (!GetProcessAffinityMask(processHandle, &processAffinityMask, &systemAffinityMask)) {
		consoleLog("Failed to Get Process Affinity Mask", true, false, true);
		processAffinityMask = NULL;
		systemAffinityMask = NULL;
		return false;
	}

	// loop through all the cores, leaving only one bit lit
	BYTE processAffinityMaskLit = 0;

	for (BYTE i = 0; i < 32; i++) {
		if (processAffinityMaskLit < affinity) {
			if (processAffinityMask & (1 << i)) {
				processAffinityMaskLit++;
			}
		} else {
			// clear the bit
			processAffinityMask &= ~(1 << i);
		}
	}

	if (!SetProcessAffinityMask(processHandle, processAffinityMask)) {
		consoleLog("Failed to Set Process Affinity Mask", true, false, true);
		processAffinityMask = NULL;
		systemAffinityMask = NULL;
		return false;
	}

	processAffinityMask = NULL;
	systemAffinityMask = NULL;
	return true;
}

bool getCommandLineArgument(std::string commandLine, std::string &commandLineArgument) {
	std::regex commandLineQuotes("^\\s*\"[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\"? ?");
	std::regex commandLineWords("^\\s*\\S+ ?");
	std::smatch matchResults = {};
	bool match = std::regex_search(commandLine, matchResults, commandLineQuotes);

	if (match && matchResults.length() > 0) {
		commandLineArgument = matchResults[0];
		return true;
	} else {
		matchResults = {};
		match = std::regex_search(commandLine, matchResults, commandLineWords);

		if (match && matchResults.length() > 0) {
			commandLineArgument = matchResults[0];
			return true;
		}
	}
	return false;
}

std::string getCommandLineArgumentRange(std::string commandLine, int begin, int end) {
	std::vector<std::string>::iterator commandLineArgumentsIterator;
	std::vector<std::string> commandLineArguments;
	std::string commandLineArgument = "";
	std::string commandLineArgumentRange = "";

	while (getCommandLineArgument(commandLine, commandLineArgument)) {
		commandLineArguments.push_back(commandLineArgument);
		commandLine.erase(0, commandLineArgument.length());
	}

	int i = 0;

	if (end < 0) {
		end += commandLineArguments.size() + 1;
	}

	for (commandLineArgumentsIterator = commandLineArguments.begin(); commandLineArgumentsIterator != commandLineArguments.end(); commandLineArgumentsIterator++) {
		if (i >= end) {
			break;
		}

		if (i >= begin) {
			commandLineArgumentRange += *commandLineArgumentsIterator;
		}

		i++;
	}
	return commandLineArgumentRange;
}

bool createSyncedProcess(LPCSTR software, HANDLE &syncedProcessHandle, HANDLE &syncedProcessMainThreadHandle, DWORD &syncedProcessID, bool syncedProcessMainThreadOnly, HANDLE &jobHandle) {
	consoleLog("Creating Synced Process");

	if (!software) {
		consoleLog("software cannot be NULL", true, false, true);
		return false;
	}

	if (!strlen(software)) {
		consoleLog("software cannot be EMPTY", true, false, true);
		return false;
	}

	// we create a job so that if either the processHandle or the synced processHandle ends
	// for whatever reason, we don't sync the processHandle anymore
	jobHandle = CreateJobObject(NULL, NULL);

	if (!jobHandle || jobHandle == INVALID_HANDLE_VALUE) {
		consoleLog("Failed to Create Job Object", true, false, true);
		return false;
	}

	// this is how we kill both processes if either ends
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobobjectExtendedLimitInformation = {};
	jobobjectExtendedLimitInformation.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

	if (!SetInformationJobObject(jobHandle, JobObjectExtendedLimitInformation, &jobobjectExtendedLimitInformation, sizeof(jobobjectExtendedLimitInformation))) {
		consoleLog("Failed to Set Job Object Information", true, false, true);
		return false;
	}

	// assign the current processHandle to the job object
	// we assign the synced processHandle later
	if (!AssignProcessToJobObject(jobHandle, GetCurrentProcess())) {
		consoleLog("Failed to Assign Current Process to Job Object", true, false, true);
		return false;
	}

	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(CA2W(software), &argc);

	if (!argv || argc < 1) {
		consoleLog("Failed to Get Command Line Arguments", true, false, true);
		return false;
	}

	// this is where we create the synced processHandle and get a handle to it and its main thread, as well as its ID
	LPSTR path = new CHAR[MAX_PATH];

	if (!path) {
		consoleLog("Failed to Allocate path", true, false, true);
		return false;
	}

	// SetCurrentDirectory may add a backslash if not present, so
	// you must use MAX_PATH - 1 characters
	if (strncpy_s(path, MAX_PATH - 1, CW2A(argv[0]), MAX_PATH - 1)) {
		consoleLog("Failed to Copy String Maximum", true, false, true);
		delete[] path;
		path = NULL;
		return false;
	}

	if (!GetFullPathName(path, MAX_PATH - 1, path, NULL)) {
		consoleLog("Failed to Get Full Path Name", true, false, true);
		delete[] path;
		path = NULL;
		return false;
	}

	while (!PathIsDirectory(path)) {
		if (!PathRemoveFileSpec(path)) {
			consoleLog("Failed to Remove Path File Spec", true, false, true);
			delete[] path;
			path = NULL;
			return false;
		}
	}

	if (!SetCurrentDirectory(path)) {
		consoleLog("Failed to Set Current Directory", true, false, true);
		delete[] path;
		path = NULL;
		return false;
	}

	delete[] path;
	path = NULL;

	STARTUPINFO syncedProcessStartupInformation;
	PROCESS_INFORMATION syncedProcessStartedInformation;

	// default settings for these arguments
	ZeroMemory(&syncedProcessStartupInformation, sizeof(syncedProcessStartupInformation));
	ZeroMemory(&syncedProcessStartedInformation, sizeof(syncedProcessStartedInformation));

	// the cb needs to match the size
	syncedProcessStartupInformation.cb = sizeof(syncedProcessStartupInformation);

	// create the processHandle, fail if we can't
	if (!CreateProcess(NULL, (LPSTR)software, NULL, NULL, TRUE, 0, NULL, NULL, &syncedProcessStartupInformation, &syncedProcessStartedInformation)
		|| !syncedProcessStartedInformation.hProcess
		|| syncedProcessStartedInformation.hProcess == INVALID_HANDLE_VALUE
		|| !syncedProcessStartedInformation.hThread
		|| syncedProcessStartedInformation.hThread == INVALID_HANDLE_VALUE) {
		consoleLog("Failed to Create Process", true, false, true);
		return false;
	}

	// get the handles and the ID
	syncedProcessHandle = syncedProcessStartedInformation.hProcess;
	syncedProcessMainThreadHandle = syncedProcessStartedInformation.hThread;
	syncedProcessID = syncedProcessStartedInformation.dwProcessId;

	// assign the synced processHandle to the job object
	// we've now set up the job processHandle
	// commented because Windows does this for us automatically
	/*
	if (!AssignProcessToJobObject(jobHandle, syncedProcessHandle)) {
		consoleLog("Failed to Assign Process To Job Object", true, false, true);
		return false;
	}
	*/
	return true;
}

bool terminateSyncedProcess(HANDLE &syncedProcessHandle, HANDLE &syncedProcessMainThreadHandle, bool syncedProcessMainThreadOnly, HANDLE &jobHandle) {
	consoleLog("Terminating Synced Process");

	if (syncedProcessMainThreadOnly) {
		if (syncedProcessMainThreadHandle && syncedProcessMainThreadHandle != INVALID_HANDLE_VALUE) {
			if (!CloseHandle(syncedProcessMainThreadHandle)) {
				consoleLog("Failed to Close Process Main Thread Handle", true, false, true);
				return false;
			}

			syncedProcessMainThreadHandle = NULL;
		}
	}

	// if not already closed
	if (syncedProcessHandle && syncedProcessHandle != INVALID_HANDLE_VALUE) {
		if (!TerminateProcess(syncedProcessHandle, -1)) {
			consoleLog("Failed to Terminate Process", true, false, true);
			return false;
		}

		syncedProcessHandle = NULL;
	}

	if (jobHandle && jobHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(jobHandle);
		jobHandle = NULL;
	}
	return true;
}

bool openSyncedProcessThread(DWORD syncedProcessThreadID, HANDLE &syncedProcessThreadHandle) {
	//consoleLog("Opening Synced Process Thread");

	// open the synced processHandle's thread
	// we don't bail if it fails
	// it's a very real possibility that it could fail
	// and we need to be able to just ignore it
	syncedProcessThreadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, syncedProcessThreadID);

	if (!syncedProcessThreadHandle || syncedProcessThreadHandle == INVALID_HANDLE_VALUE) {
		consoleLog("Failed to Open Process Thread", true, false, true);
		return false;
	}
	return true;
}

bool closeSyncedProcessThread(HANDLE &syncedProcessThreadHandle) {
	//consoleLog("Closing Synced Process Thread");

	if (syncedProcessThreadHandle && syncedProcessThreadHandle != INVALID_HANDLE_VALUE) {
		if (!CloseHandle(syncedProcessThreadHandle)) {
			consoleLog("Failed to Close Process Thread Handle", true, false, true);
			return false;
		}

		syncedProcessThreadHandle = NULL;
	}
	return true;
}

bool beginRefreshTimePeriod(UINT &refreshHz, UINT &refreshMs, UINT &suspendMs, UINT &resumeMs, UINT &ms, UINT &s, DOUBLE suspend, DOUBLE resume, bool refreshHzFloorFifteen) {
	consoleLog("Beginning Refresh Time Period");

	TIMECAPS devCaps;

	if (timeGetDevCaps(&devCaps, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
		consoleLog("Failed to Get Time Dev Caps", true, false, true);
		return false;
	}

	// one millisecond (approximately)
	ms = clamp(ms, devCaps.wPeriodMin, devCaps.wPeriodMax);
	// one second (approximately)
	s = clamp(s, devCaps.wPeriodMin, devCaps.wPeriodMax);
	// if we're suspended 3 / 4
	// and resumed 1 / 4
	// we'll be suspended a minimum of 3 Ms
	// and resumed a minimum of 1 Ms
	// (3 / 4) / (1 / 4) = 3 Ms
	// 3 Ms + 1 Ms = 4 Ms, our minRefreshMs
	DOUBLE minRefreshMs = ((DOUBLE)(max(suspend, resume)) / (DOUBLE)(min(suspend, resume)) * (DOUBLE)ms) + (DOUBLE)ms;
	DOUBLE maxRefreshHz = (DOUBLE)s / (DOUBLE)minRefreshMs;
	refreshHz = clamp(min(refreshHz, maxRefreshHz), ms, s);

	// we do this after in case the Refresh Rate before was well above the maximum
	if (refreshHzFloorFifteen) {
		refreshHz = clamp(min(((refreshHz + 8) / 15) * 15, maxRefreshHz), ms, s);
	}

	refreshMs = clamp((DOUBLE)s / (DOUBLE)refreshHz, ceil(minRefreshMs), s);

	// should never in any circumstance be lower than ms
	suspendMs = max(suspend * refreshMs, ms);
	resumeMs = max(resume * refreshMs, ms);

	// if suspendMs is divisible by resumeMs or vice versa
	if (!(suspendMs % resumeMs) || !(resumeMs % suspendMs)) {
		// set precision to highest value that will work for both suspend/resume time
		if (timeBeginPeriod(min(suspendMs, resumeMs)) != TIMERR_NOERROR) {
			consoleLog("Failed to Begin Time Period", true, false, true);
			return false;
		}
	} else {
		// well, everything is divisible by one
		if (timeBeginPeriod(ms) != TIMERR_NOERROR) {
			consoleLog("Failed to Begin Time Period", true, false, true);
			return false;
		}
	}
	return true;
}

bool endRefreshTimePeriod(UINT &suspendMs, UINT &resumeMs, UINT ms) {
	consoleLog("Ending Refresh Time Period");

	// find same value as before
	if (!(suspendMs % resumeMs) || !(resumeMs % suspendMs)) {
		if (timeEndPeriod(min(suspendMs, resumeMs)) != TIMERR_NOERROR) {
			consoleLog("Failed to End Time Period", true, false, true);
			return false;
		}
	} else {
		if (timeEndPeriod(ms) != TIMERR_NOERROR) {
			consoleLog("Failed to End Time Period", true, false, true);
			return false;
		}
	}
	return true;
}

void CALLBACK OneShotTimer(UINT, UINT, DWORD dwUser, DWORD, DWORD) {
	// posts the message to incite the timer
	// you're not supposed to call anything other than PostMessage in these callbacks
	PostMessage((HWND)dwUser, UWM_SIMULATE_OLD_CPUS_SYNC_PROCESS, NULL, NULL);
}

bool syncProcess(HWND messageOnlyWindowHandle,
				 HANDLE syncedProcessHandle,
				 HANDLE syncedProcessMainThreadHandle,
				 DWORD syncedProcessID,
				 std::vector<HANDLE> &syncedProcessThreadHandles,
				 bool syncedProcessMainThreadOnly,
				 bool &suspended,
				 char mode,
				 UINT suspendMs,
				 UINT resumeMs,
				 NTQUERYSYSTEMINFORMATION originalNtQuerySystemInformation,
				 NTSUSPENDPROCESS originalNtSuspendProcess,
				 NTRESUMEPROCESS originalNtResumeProcess) {
	//consoleLog("Syncing Process");
	std::vector<HANDLE>::iterator syncedProcessThreadHandlesIterator;

	if (!suspended) {
		suspended = true;

		if (!syncedProcessMainThreadOnly) {
			if (mode > 0) {
				// ensure this is safe first
				if (mode > 1) {
					// take a snapshot of all processes currently running's snapshots
					// say hello to our main bottleneck
					// thankfully, we'll probably only ever need to
					// use it on Windows ME or lower
					HANDLE syncedProcessThreadSnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

					if (!syncedProcessThreadSnapshotHandle || syncedProcessThreadSnapshotHandle == INVALID_HANDLE_VALUE) {
						consoleLog("Failed to Create Synced Process Thread Snapshot", true, false, true);
						return false;
					}

					// allows us to walk along the threads
					THREADENTRY32 syncedProcessThreadWalker;
					syncedProcessThreadWalker.dwSize = sizeof(THREADENTRY32);

					if (!Thread32First(syncedProcessThreadSnapshotHandle, &syncedProcessThreadWalker)) {
						consoleLog("Failed to Close Synced Process Thread Snapshot Handle", true, false, true);
						CloseHandle(syncedProcessThreadSnapshotHandle);
						syncedProcessThreadSnapshotHandle = NULL;
						return false;
					}

					do {
						if (syncedProcessThreadWalker.th32OwnerProcessID == syncedProcessID) {
							HANDLE syncedProcessThread = NULL;

							if (openSyncedProcessThread(syncedProcessThreadWalker.th32ThreadID, syncedProcessThread)) {
								syncedProcessThreadHandles.push_back(syncedProcessThread);
							}
						}
					} while (Thread32Next(syncedProcessThreadSnapshotHandle, &syncedProcessThreadWalker));

					// clean up, clean up, everybody everywhere
					if (!CloseHandle(syncedProcessThreadSnapshotHandle)) {
						consoleLog("Failed to Close Synced Process Thread Snapshot Handle", true, false, true);
						return false;
					}

					syncedProcessThreadSnapshotHandle = NULL;
				} else {
					// we'll need these in a bit
					const int SYSTEM_PROCESS_INFORMATION_SIZE = sizeof(__SYSTEM_PROCESS_INFORMATION);
					const int SYSTEM_THREAD_INFORMATION_SIZE = sizeof(SYSTEM_THREAD_INFORMATION);
					// first we'll allocate a buffer a single page in length
					unsigned long systemProcessInformationOutputBufferPointerSize = 0x10000;
					// create the buffer of that size
					LPVOID systemProcessInformationOutputBufferPointer = new BYTE[systemProcessInformationOutputBufferPointerSize];

					if (!systemProcessInformationOutputBufferPointer) {
						consoleLog("Failed to Create System Process Information Output Buffer", true, false, true);
						return false;
					}

					NTSTATUS NtStatus = originalNtQuerySystemInformation(SystemProcessInformation, systemProcessInformationOutputBufferPointer, systemProcessInformationOutputBufferPointerSize, NULL);
					
					// if the buffer wasn't large enough
					while (NtStatus == STATUS_INFO_LENGTH_MISMATCH) {
						// double the size
						systemProcessInformationOutputBufferPointerSize += systemProcessInformationOutputBufferPointerSize;
						delete[] systemProcessInformationOutputBufferPointer;
						systemProcessInformationOutputBufferPointer = new BYTE[systemProcessInformationOutputBufferPointerSize];

						if (!systemProcessInformationOutputBufferPointer) {
							consoleLog("Failed to Create System Process Information Output Buffer", true, false, true);
							return false;
						}

						NtStatus = originalNtQuerySystemInformation(SystemProcessInformation, systemProcessInformationOutputBufferPointer, systemProcessInformationOutputBufferPointerSize, NULL);
					}

					// check it worked
					if (NtStatus != STATUS_SUCCESS) {
						consoleLog("Failed to Query System Information", true, false, true);
						delete[] systemProcessInformationOutputBufferPointer;
						systemProcessInformationOutputBufferPointer = NULL;
						return false;
					} else {
						// cast it
						__PSYSTEM_PROCESS_INFORMATION systemProcessInformationPointer = (__PSYSTEM_PROCESS_INFORMATION)systemProcessInformationOutputBufferPointer;
						
						// then we'll loop through every processHandle
						while (systemProcessInformationPointer) {
							// if the processHandle's ID matches the synced processHandle ID
							if ((DWORD)systemProcessInformationPointer->UniqueProcessId == syncedProcessID) {
								// we'll be reading its thread information
								PSYSTEM_THREAD_INFORMATION systemThreadInformationPointer = (PSYSTEM_THREAD_INFORMATION)((LPBYTE)systemProcessInformationPointer + SYSTEM_PROCESS_INFORMATION_SIZE);
								
								// for each thread of the processHandle
								for (ULONG i = 0; i < systemProcessInformationPointer->NumberOfThreads; i++) {
									// go to next thread
									HANDLE syncedProcessThread = NULL;

									if (openSyncedProcessThread((DWORD)systemThreadInformationPointer->ClientId.UniqueThread, syncedProcessThread)) {
										syncedProcessThreadHandles.push_back(syncedProcessThread);
									}

									systemThreadInformationPointer++;
								}

								// syncedProcessThreadHandles now contains all the threads for the processHandle
								delete[] systemProcessInformationOutputBufferPointer;
								systemProcessInformationOutputBufferPointer = NULL;
								systemProcessInformationPointer = NULL;
								systemThreadInformationPointer = NULL;
								break;
							}
							if (!systemProcessInformationPointer->NextEntryOffset) {
								// there is no next processHandle and we didn't loop through any threads
								consoleLog("Failed to Go To Synced Process System Process Information", true, false, true);
								delete[] systemProcessInformationOutputBufferPointer;
								systemProcessInformationOutputBufferPointer = NULL;
								systemProcessInformationPointer = NULL;
								return false;
							} else {
								// go to next processHandle
								systemProcessInformationPointer = (__PSYSTEM_PROCESS_INFORMATION)((LPBYTE)systemProcessInformationPointer + systemProcessInformationPointer->NextEntryOffset);
							}
						}
					}
				}

				// SuspendThread already handles the situation in which the thread was already suspended
				// we don't close the handles here because we need to resume them too
				for (syncedProcessThreadHandlesIterator = syncedProcessThreadHandles.begin(); syncedProcessThreadHandlesIterator != syncedProcessThreadHandles.end(); syncedProcessThreadHandlesIterator++) {
					if (*syncedProcessThreadHandlesIterator && *syncedProcessThreadHandlesIterator != INVALID_HANDLE_VALUE) {
						SuspendThread(*syncedProcessThreadHandlesIterator);
					}
				}
			} else {
				if (syncedProcessHandle && syncedProcessHandle != INVALID_HANDLE_VALUE) {
					originalNtSuspendProcess(syncedProcessHandle);
				}
			}
		} else {
			if (syncedProcessMainThreadHandle && syncedProcessMainThreadHandle != INVALID_HANDLE_VALUE) {
				SuspendThread(syncedProcessMainThreadHandle);
			}
		}

		// set the timeout for the next time to run this function
		if (!timeSetEvent(suspendMs, 0, OneShotTimer, (DWORD)messageOnlyWindowHandle, TIME_ONESHOT)) {
			consoleLog("Failed to Set Time Event after Suspending Synced Process", true, false, true);
			return false;
		}
	} else {
		suspended = false;

		if (!syncedProcessMainThreadOnly) {
			// slower fallback in case the undocumented functions don't exist
			if (mode > 0) {
				// PLEASE FOR THE LOVE OF ALL THAT IS GOOD BE CAREFUL MODIFYING THESE LOOPS!!!
				// now we close the threads, backwards so it's in the same order
				// it's alright if we can't resume the thread
				// we'll just do it on the next loop
				syncedProcessThreadHandlesIterator = syncedProcessThreadHandles.end();

				while (syncedProcessThreadHandlesIterator != syncedProcessThreadHandles.begin()) {
					// this cannot be a for loop because this must happen AFTER the condition
					syncedProcessThreadHandlesIterator--;

					if (*syncedProcessThreadHandlesIterator && *syncedProcessThreadHandlesIterator != INVALID_HANDLE_VALUE) {
						ResumeThread(*syncedProcessThreadHandlesIterator);
						closeSyncedProcessThread(*syncedProcessThreadHandlesIterator);
					}
				}

				for (syncedProcessThreadHandlesIterator = syncedProcessThreadHandles.begin(); syncedProcessThreadHandlesIterator != syncedProcessThreadHandles.end(); syncedProcessThreadHandlesIterator++) {
					if (!*syncedProcessThreadHandlesIterator || *syncedProcessThreadHandlesIterator == INVALID_HANDLE_VALUE) {
						syncedProcessThreadHandles.erase(syncedProcessThreadHandlesIterator--);
					}
				}
			} else {
				if (syncedProcessHandle && syncedProcessHandle != INVALID_HANDLE_VALUE) {
					originalNtResumeProcess(syncedProcessHandle);
				}
			}
		} else {
			if (syncedProcessMainThreadHandle && syncedProcessMainThreadHandle != INVALID_HANDLE_VALUE) {
				ResumeThread(syncedProcessMainThreadHandle);
			}
		}

		if (!timeSetEvent(resumeMs, 0, OneShotTimer, (DWORD)messageOnlyWindowHandle, TIME_ONESHOT)) {
			consoleLog("Failed to Set Time Event After Resuming Synced Process", true, false, true);
			return false;
		}
	}
	return true;
}

int main(int argc, char** argv) {
	HANDLE oldCPUSimulatorMutexHandle = CreateMutex(NULL, FALSE, "Old CPU Simulator");

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		//consoleLog("You cannot run multiple instances of Old CPU Simulator.", true, false, true);
		return -2;
	}

	consoleLog("Old CPU Simulator 1.6.0");
	consoleLog("By Anthony Kleine", 2);

	/*
	if (argc < 2) {
		consoleLog("You must pass the filename of an executable with which to create a processHandle as the first argument.", 3, false, true);
		help();
		ReleaseMutex(oldCPUSimulatorMutexHandle);
		return -1;
	}
	*/

	std::string argString = "";
	ULONG currentMhz = 0;

	if (argc > 1) {
		argString = std::string(argv[1]);

		if (argString == "--help") {
			help();
			ReleaseMutex(oldCPUSimulatorMutexHandle);
			return 0;
		} else if (argString == "--dev-get-current-mhz") {
			if (!getCurrentMhz(currentMhz)
				|| !currentMhz) {
				consoleLog("Failed to Get Current Rate", true, false, true);
				ReleaseMutex(oldCPUSimulatorMutexHandle);
				return -1;
			}

			consoleLog(std::to_string(currentMhz).c_str(), false);
			ReleaseMutex(oldCPUSimulatorMutexHandle);
			return 0;
		}
	}

	int requiredArgs = 0;
	UINT refreshHz = 1000;
	ULONG targetMhz = 233;
	std::string software = "";
	bool setProcessPriorityHigh = false;
	bool setSyncedProcessAffinityOne = false;
	bool syncedProcessMainThreadOnly = false;
	bool refreshHzFloorFifteen = false;
	char mode = -1;

	for (int i = 1; i < argc; ++i) {
		argString = std::string(argv[i]);

		if (argString == "--dev-get-current-mhz") {
			if (!getCurrentMhz(currentMhz)
				|| !currentMhz) {
				consoleLog("Failed to Get Current Rate", true, false, true);
				ReleaseMutex(oldCPUSimulatorMutexHandle);
				return -3;
			}

			consoleLog(std::to_string(currentMhz).c_str(), false);
			ReleaseMutex(oldCPUSimulatorMutexHandle);
			return 0;
		} else if (argString == "-t") {
			if (!getCurrentMhz(currentMhz)
				|| !currentMhz) {
				consoleLog("Failed to Get Current Rate", true, false, true);
				ReleaseMutex(oldCPUSimulatorMutexHandle);
				return -1;
			}

			if (i + 1 < argc) {
				targetMhz = strtol(argv[++i], NULL, 10);

				if (!targetMhz) {
					consoleLog("The Target Rate must be a number.", true, false, true);
					help();
					ReleaseMutex(oldCPUSimulatorMutexHandle);
					return -1;
				}

				if (currentMhz <= targetMhz) {
					consoleLog("The Target Rate cannot exceed or equal the Current Rate of ", false, false, true);
					consoleLog(std::to_string(currentMhz).c_str(), false, false, true);
					consoleLog(".", 3, false, true);
					help();
					ReleaseMutex(oldCPUSimulatorMutexHandle);
					return -1;
				}

				requiredArgs++;
			} else {
				consoleLog("-t option requires one argument: the Target Rate (in MHz, from 1 to your CPU's clock speed of ", false, false, true);
				consoleLog(std::to_string(currentMhz).c_str(), false, false, true);
				consoleLog(") to simulate.", 3, false, true);
				help();
				ReleaseMutex(oldCPUSimulatorMutexHandle);
				return -1;
			}
		} else if (argString == "-r") {
			if (i + 1 < argc) {
				refreshHz = atoi(argv[++i]);

				if (!refreshHz) {
					consoleLog("The Refresh Rate cannot be zero.", 3, false, true);
					help();
					ReleaseMutex(oldCPUSimulatorMutexHandle);
					return -1;
				}

				//requiredArgs++;
			} else {
				consoleLog("-r option requires one argument: the Refresh Rate (in Hz, from 1 to 1000) at which to refresh.", 3, false, true);
				help();
				ReleaseMutex(oldCPUSimulatorMutexHandle);
				return -1;
			}
		} else if (argString == "-sw" || argString == "--software") {
			software = getCommandLineArgumentRange(GetCommandLine(), i + 1, -1);
			requiredArgs++;
			break;
		} else if (argString == "--set-process-priority-high") {
			setProcessPriorityHigh = true;
		} else if (argString == "--set-synced-process-affinity-one") {
			setSyncedProcessAffinityOne = true;
		} else if (argString == "--synced-process-main-thread-only") {
			syncedProcessMainThreadOnly = true;
		} else if (argString == "--refresh-rate-floor-fifteen") {
			refreshHzFloorFifteen = true;
		} else if (argString == "--dev-force-mode-0") {
			mode = 0;
		} else if (argString == "--dev-force-mode-1") {
			mode = 1;
		} else if (argString == "--dev-force-mode-2") {
			mode = 2;
		} else if (argString == "--help") {
			help();
			ReleaseMutex(oldCPUSimulatorMutexHandle);
			return 0;
		} else {
			consoleLog("Invalid Argument: ", false, false, true);
			consoleLog(argv[i], 3, false, true);
			help();
			ReleaseMutex(oldCPUSimulatorMutexHandle);
			return -1;
		}
	}

	if (requiredArgs < 2) {
		help();
		ReleaseMutex(oldCPUSimulatorMutexHandle);
		return -1;
	}

	if (setProcessPriorityHigh) {
		if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
			consoleLog("Failed to Set Synced Process Priority", true, false, true);
			ReleaseMutex(oldCPUSimulatorMutexHandle);
			return -1;
		}
	}

	// create message only window
	HMODULE moduleHandle = GetModuleHandle(NULL);
	WNDCLASSEX windowClassEx = {};
	windowClassEx.cbSize = sizeof(WNDCLASSEX);
	windowClassEx.lpfnWndProc = DefWindowProc;
	windowClassEx.hInstance = moduleHandle;
	windowClassEx.lpszClassName = "OLD_CPU_SIMULATOR";
	HWND messageOnlyWindowHandle = NULL;

	if (!RegisterClassEx(&windowClassEx)) {
		consoleLog("Failed to Register Window Class", true, false, true);
		ReleaseMutex(oldCPUSimulatorMutexHandle);
		return -1;
	} else {
		messageOnlyWindowHandle = CreateWindowEx(WS_OVERLAPPED, windowClassEx.lpszClassName, "Old CPU Simulator", WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, moduleHandle, NULL);
		
		if (!messageOnlyWindowHandle) {
			consoleLog("Failed to Create Message Only Window", true, false, true);
			ReleaseMutex(oldCPUSimulatorMutexHandle);
			return -1;
		}
	}

	HMODULE originalNtDll = NULL;
	NTQUERYSYSTEMINFORMATION originalNtQuerySystemInformation = NULL;
	NTSUSPENDPROCESS originalNtSuspendProcess = NULL;
	NTRESUMEPROCESS originalNtResumeProcess = NULL;
	getOriginalNtDll(originalNtDll, originalNtQuerySystemInformation, originalNtSuspendProcess, originalNtResumeProcess);

	DWORD syncedProcessID = 0;
	HANDLE syncedProcessHandle = NULL;
	HANDLE syncedProcessMainThreadHandle = NULL;
	HANDLE jobHandle = NULL;

	if (!createSyncedProcess(software.c_str(), syncedProcessHandle, syncedProcessMainThreadHandle, syncedProcessID, syncedProcessMainThreadOnly, jobHandle)
		|| !syncedProcessHandle
		|| syncedProcessHandle == INVALID_HANDLE_VALUE
		|| !syncedProcessMainThreadHandle
		|| syncedProcessMainThreadHandle == INVALID_HANDLE_VALUE) {
		consoleLog("Failed to Create Synced Process", true, false, true);
		ReleaseMutex(oldCPUSimulatorMutexHandle);
		return -1;
	}

	if (!syncedProcessMainThreadOnly) {
		if (!CloseHandle(syncedProcessMainThreadHandle)) {
			consoleLog("Failed to Close Process Main Thread Handle", true, false, true);
			return -1;
		}

		syncedProcessMainThreadHandle = NULL;
	}

	if (setSyncedProcessAffinityOne) {
		if (!setProcessAffinity(syncedProcessHandle, 1)) {
			consoleLog("Failed to Set Synced Process Affinity", true, false, true);
			ReleaseMutex(oldCPUSimulatorMutexHandle);

			if (!terminateSyncedProcess(syncedProcessHandle, syncedProcessMainThreadHandle, syncedProcessMainThreadOnly, jobHandle)) {
				consoleLog("Failed to Terminate Synced Process", true, false, true);
			}
			return -1;
		}
	}

	UINT refreshMs = 0;
	UINT suspendMs = 0;
	UINT resumeMs = 0;
	UINT ms = 1;
	UINT s = 1000;
	DOUBLE suspend = ((DOUBLE)(currentMhz - targetMhz) / (DOUBLE)currentMhz);
	DOUBLE resume = ((DOUBLE)targetMhz / (DOUBLE)currentMhz);

	if (!beginRefreshTimePeriod(refreshHz, refreshMs, suspendMs, resumeMs, ms, s, suspend, resume, refreshHzFloorFifteen)) {
		consoleLog("Failed to Begin Refresh Time Period", true, false, true);
		ReleaseMutex(oldCPUSimulatorMutexHandle);

		if (!terminateSyncedProcess(syncedProcessHandle, syncedProcessMainThreadHandle, syncedProcessMainThreadOnly, jobHandle)) {
			consoleLog("Failed to Terminate Synced Process", true, false, true);
		}
		return -1;
	}

	// determine mode if relevant
	if (!syncedProcessMainThreadOnly) {
		if (mode == -1) {
			if (!originalNtSuspendProcess || !originalNtResumeProcess) {
				if (!originalNtQuerySystemInformation) {
					mode = 2;
				} else {
					mode = 1;
				}
			} else {
				mode = 0;
			}
		}
	}

	std::vector<HANDLE> syncedProcessThreads = {};
	bool suspended = false;

	MSG message = {};

	// incite the timer that will begin syncing the processHandle
	if (!timeSetEvent(resumeMs, 0, OneShotTimer, (DWORD)messageOnlyWindowHandle, TIME_ONESHOT)) {
		consoleLog("Failed to Set Time Event Before Syncing Synced Process", true, false, true);

		if (!endRefreshTimePeriod(suspendMs, resumeMs, ms)) {
			consoleLog("Failed to End Refresh Time Period", true, false, true);
		}

		ReleaseMutex(oldCPUSimulatorMutexHandle);

		if (!terminateSyncedProcess(syncedProcessHandle, syncedProcessMainThreadHandle, syncedProcessMainThreadOnly, jobHandle)) {
			consoleLog("Failed to Terminate Synced Process", true, false, true);
		}
		return -1;
	}
	// while the processHandle is active
	while (WaitForSingleObject(syncedProcessHandle, 0) == WAIT_TIMEOUT) {
		message = {};

		if (PeekMessage(&message, messageOnlyWindowHandle, 0, 0, PM_REMOVE)) {
			if (message.message == UWM_SIMULATE_OLD_CPUS_SYNC_PROCESS) {
				if (!syncProcess(messageOnlyWindowHandle,
					syncedProcessHandle,
					syncedProcessMainThreadHandle,
					syncedProcessID,
					syncedProcessThreads,
					syncedProcessMainThreadOnly,
					suspended,
					mode,
					suspendMs,
					resumeMs,
					originalNtQuerySystemInformation,
					originalNtSuspendProcess,
					originalNtResumeProcess)) {
					consoleLog("Failed to Sync Synced Process", true, false, true);

					if (!endRefreshTimePeriod(suspendMs, resumeMs, ms)) {
						consoleLog("Failed to End Refresh Time Period", true, false, true);
					}

					ReleaseMutex(oldCPUSimulatorMutexHandle);

					if (!terminateSyncedProcess(syncedProcessHandle, syncedProcessMainThreadHandle, syncedProcessMainThreadOnly, jobHandle)) {
						consoleLog("Failed to Terminate Synced Process", true, false, true);
					}
					return -1;
				}
			}
		}
	}

	if (!endRefreshTimePeriod(suspendMs, resumeMs, ms)) {
		consoleLog("Failed to End Refresh Time Period", true, false, true);
	}

	ReleaseMutex(oldCPUSimulatorMutexHandle);
	//terminateSyncedProcess(syncedProcessHandle, syncedProcessMainThreadHandle, syncedProcessMainThreadOnly, jobHandle);
	return 0;
}