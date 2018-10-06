#include "main.h"

bool createSyncedProcess(LPSTR lpCommandLine, HANDLE &syncedProcess, HANDLE &syncedProcessMainThread, DWORD &syncedProcessID, bool syncedProcessMainThreadOnly, HANDLE &hJob) {
	consoleLog("Creating Synced Process");
	// we create a job so that if either the process or the synced process ends
	// for whatever reason, we don't sync the process anymore
	hJob = CreateJobObject(NULL, NULL);
	// CreateJobObject returns NULL, not INVALID_HANDLE_VALUE
	if (!hJob) {
		consoleLog("Failed to create Job Object", true, false, true);
		return false;
	}
	// this is how we kill both processes if either ends
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobobjectExtendedLimitInformation = {};
	jobobjectExtendedLimitInformation.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (!SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobobjectExtendedLimitInformation, sizeof(jobobjectExtendedLimitInformation))) {
		consoleLog("Failed to set Job Object Information", true, false, true);
		return false;
	}
	// assign the current process to the job object
	// we assign the synced process later
	if (!AssignProcessToJobObject(hJob, GetCurrentProcess())) {
		consoleLog("Failed to assign Current Process to Job Object", true, false, true);
		return false;
	}

	// this is where we create the synced process and get a handle to it and its main thread, as well as its ID
	STARTUPINFO syncedProcessStartupInformation;
	PROCESS_INFORMATION syncedProcessStartedInformation;

	// default settings for these arguments
	ZeroMemory(&syncedProcessStartupInformation, sizeof(syncedProcessStartupInformation));
	ZeroMemory(&syncedProcessStartedInformation, sizeof(syncedProcessStartedInformation));

	// the cb needs to match the size
	syncedProcessStartupInformation.cb = sizeof(syncedProcessStartupInformation);

	// create the process, fail if we can't
	if (!CreateProcess(NULL, lpCommandLine, NULL, NULL, TRUE, 0, NULL, NULL, &syncedProcessStartupInformation, &syncedProcessStartedInformation)
		|| syncedProcessStartedInformation.hProcess == INVALID_HANDLE_VALUE
		|| syncedProcessStartedInformation.hThread == INVALID_HANDLE_VALUE) {
		consoleLog("Failed to create Synced Process", true, false, true);
		return false;
	}
	// get the handles and the ID
	syncedProcess = syncedProcessStartedInformation.hProcess;
	syncedProcessMainThread = syncedProcessStartedInformation.hThread;
	if (!syncedProcessMainThreadOnly) {
		if (!CloseHandle(syncedProcessMainThread)) {
			consoleLog("Failed to close Synced Process's Main Thread", true, false, true);
			return false;
		}
	}
	syncedProcessID = syncedProcessStartedInformation.dwProcessId;

	// assign the synced process to the job object
	// we've now set up the job process
	if (!AssignProcessToJobObject(hJob, syncedProcess)) {
		consoleLog("Failed to assign Synced Process to Job Object", true, false, true);
		return false;
	}
	return true;
}

bool terminateSyncedProcess(HANDLE &syncedProcess, HANDLE &syncedProcessMainThread, bool syncedProcessMainThreadOnly, HANDLE &hJob) {
	consoleLog("Terminating Synced Process");
	if (syncedProcessMainThreadOnly) {
		if (syncedProcessMainThread != INVALID_HANDLE_VALUE) {
			if (!CloseHandle(syncedProcessMainThread)) {
				consoleLog("Failed to close Synced Process's Main Thread", true, false, true);
				return false;
			}
		}
	}
	// if not already closed
	if (syncedProcess != INVALID_HANDLE_VALUE) {
		if (!TerminateProcess(syncedProcess, -1)) {
			consoleLog("Failed to terminate Synced Process", true, false, true);
			return false;
		}
	}
	if (hJob != INVALID_HANDLE_VALUE) {
		consoleLog("Failed to close Job Object", true, false, true);
		CloseHandle(hJob);
	}
	return true;
}

bool openSyncedProcessThread(DWORD syncedProcessThreadID, std::vector<HANDLE> &syncedProcessThreads) {
	//consoleLog("Opening Synced Process Thread");
	// open the synced process's thread
	// we don't bail if it fails
	// it's a very real possibility that it could fail
	// and we need to be able to just ignore it
	HANDLE syncedProcessThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, syncedProcessThreadID);
	if (syncedProcessThread != INVALID_HANDLE_VALUE) {
		// push it to the back of the others
		syncedProcessThreads.push_back(syncedProcessThread);
	} else {
		return false;
	}
	return true;
}

bool closeSyncedProcessThread(unsigned int i, std::vector<HANDLE> &syncedProcessThreads) {
	//consoleLog("Closing Synced Process Thread");
	if (syncedProcessThreads[i] != INVALID_HANDLE_VALUE) {
		if (!CloseHandle(syncedProcessThreads[i])) {
			return false;
		}
	}
	syncedProcessThreads.erase(syncedProcessThreads.begin() + i);
	return true;
}

bool beginRefreshTimePeriod(UINT &refreshHz, UINT &refreshMs, UINT &ms, UINT &s, DOUBLE suspend, DOUBLE resume, bool refreshHzFloorFifteen) {
	consoleLog("Beginning Refresh Time Period");
	TIMECAPS devCaps;
	if (timeGetDevCaps(&devCaps, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
		consoleLog("Failed to get Time Dev Caps", true, false, true);
		return false;
	}
	// one millisecond (approximately)
	ms = clamp(ms, devCaps.wPeriodMin, devCaps.wPeriodMax);
	// one second (approximately)
	s = clamp(s, devCaps.wPeriodMin, devCaps.wPeriodMax);
	if (timeBeginPeriod(ms) != TIMERR_NOERROR) {
		return false;
	}
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
	return true;
}

bool endRefreshTimePeriod(UINT ms) {
	consoleLog("Ending Refresh Time Period");
	if (timeEndPeriod(ms) != TIMERR_NOERROR) {
		consoleLog("Failed to end Refresh Time Period", true, false, true);
		return false;
	}
	return true;
}

void CALLBACK OneShotTimer(UINT, UINT, DWORD dwUser, DWORD, DWORD) {
	// posts the message to incite the timer
	// you're not supposed to call anything other than PostMessage in these callbacks
	PostMessage((HWND)dwUser, UWM_EMULATE_OLD_CPUS_SYNC_PROCESS, NULL, NULL);
}

bool syncProcess(HWND hWnd,
				 HANDLE syncedProcess,
				 HANDLE syncedProcessMainThread,
				 DWORD syncedProcessID,
				 std::vector<HANDLE> &syncedProcessThreads,
				 bool syncedProcessMainThreadOnly,
				 bool &suspended,
				 char mode,
				 UINT suspendMs,
				 UINT resumeMs,
				 NTQUERYSYSTEMINFORMATION originalNtQuerySystemInformation,
				 NTSUSPENDPROCESS originalNtSuspendProcess,
				 NTRESUMEPROCESS originalNtResumeProcess) {
	//consoleLog("Syncing Process");
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
					HANDLE syncedProcessThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
					if (syncedProcessThreadSnapshot == INVALID_HANDLE_VALUE) {
						consoleLog("Failed to create Synced Process Thread Snapshot", true, false, true);
						return false;
					}

					// allows us to walk along the threads
					THREADENTRY32 syncedProcessThreadWalker;
					syncedProcessThreadWalker.dwSize = sizeof(THREADENTRY32);

					if (!Thread32First(syncedProcessThreadSnapshot, &syncedProcessThreadWalker)) {
						consoleLog("Failed to close Synced Process Thread Snapshot", true, false, true);
						CloseHandle(syncedProcessThreadSnapshot);
						return false;
					}

					do {
						if (syncedProcessThreadWalker.th32OwnerProcessID == syncedProcessID) {
							openSyncedProcessThread(syncedProcessThreadWalker.th32ThreadID, syncedProcessThreads);
						}
					} while (Thread32Next(syncedProcessThreadSnapshot, &syncedProcessThreadWalker));

					// clean up, clean up, everybody everywhere
					CloseHandle(syncedProcessThreadSnapshot);
				} else {
					// we'll need these in a bit
					const int SIZE_OF_SYSTEM_PROCESS_INFORMATION = sizeof(SYSTEM_PROCESS_INFORMATION);
					const int SIZE_OF_SYSTEM_THREAD_INFORMATION = sizeof(SYSTEM_THREAD_INFORMATION);
					// first we'll allocate a buffer a single page in length
					unsigned long sizeOfLpSystemProcessInformationOutputBuffer = 0x10000;
					// create the buffer of that size
					LPVOID lpSystemProcessInformationOutputBuffer = new BYTE[sizeOfLpSystemProcessInformationOutputBuffer];
					if (!lpSystemProcessInformationOutputBuffer) {
						consoleLog("Failed to create System Process Information Output Buffer", true, false, true);
						return false;
					}
					NTSTATUS NtStatus = originalNtQuerySystemInformation(SystemProcessInformation, lpSystemProcessInformationOutputBuffer, sizeOfLpSystemProcessInformationOutputBuffer, NULL);
					// if the buffer wasn't large enough
					while (NtStatus == STATUS_INFO_LENGTH_MISMATCH) {
						// double the size
						sizeOfLpSystemProcessInformationOutputBuffer += sizeOfLpSystemProcessInformationOutputBuffer;
						delete[] lpSystemProcessInformationOutputBuffer;
						lpSystemProcessInformationOutputBuffer = new BYTE[sizeOfLpSystemProcessInformationOutputBuffer];
						if (!lpSystemProcessInformationOutputBuffer) {
							consoleLog("Failed to create System Process Information Output Buffer", true, false, true);
							return false;
						}
						NtStatus = originalNtQuerySystemInformation(SystemProcessInformation, lpSystemProcessInformationOutputBuffer, sizeOfLpSystemProcessInformationOutputBuffer, NULL);
					}
					// check it worked
					if (NtStatus != STATUS_SUCCESS) {
						consoleLog("Failed to query System Information", true, false, true);
						delete[] lpSystemProcessInformationOutputBuffer;
						lpSystemProcessInformationOutputBuffer = NULL;
						return false;
					} else {
						// cast it
						PSYSTEM_PROCESS_INFORMATION lpSystemProcessInformation = (PSYSTEM_PROCESS_INFORMATION)lpSystemProcessInformationOutputBuffer;
						// then we'll loop through every process
						while (lpSystemProcessInformation) {
							// if the process's ID matches the synced process ID
							if ((DWORD)lpSystemProcessInformation->UniqueProcessId == syncedProcessID) {
								// we'll be reading its thread information
								PSYSTEM_THREAD_INFORMATION lpSystemThreadInformation = (PSYSTEM_THREAD_INFORMATION)((LPBYTE)lpSystemProcessInformation + SIZE_OF_SYSTEM_PROCESS_INFORMATION);
								// for each thread of the process
								for (ULONG i = 0; i < lpSystemProcessInformation->NumberOfThreads; i++) {
									// go to next thread
									openSyncedProcessThread((DWORD)lpSystemThreadInformation->ClientId.UniqueThread, syncedProcessThreads);
									lpSystemThreadInformation++;
								}
								// syncedProcessThreads now contains all the threads for the process
								delete[] lpSystemProcessInformationOutputBuffer;
								lpSystemProcessInformationOutputBuffer = NULL;
								lpSystemProcessInformation = NULL;
								lpSystemThreadInformation = NULL;
								break;
							}
							if (!lpSystemProcessInformation->NextEntryOffset) {
								// there is no next process and we didn't loop through any threads
								consoleLog("Failed to go to Synced Process's System Process Information", true, false, true);
								delete[] lpSystemProcessInformationOutputBuffer;
								lpSystemProcessInformationOutputBuffer = NULL;
								lpSystemProcessInformation = NULL;
								return false;
							} else {
								// go to next process
								lpSystemProcessInformation = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)lpSystemProcessInformation + lpSystemProcessInformation->NextEntryOffset);
							}
						}
					}
				}
				// SuspendThread already handles the situation in which the thread was already suspended
				// we don't close the handles here because we need to resume them too
				for (unsigned int i = 0;i < size(syncedProcessThreads);i++) {
					SuspendThread(syncedProcessThreads[i]);
				}
			} else {
				originalNtSuspendProcess(syncedProcess);
			}
		} else {
			SuspendThread(syncedProcessMainThread);
		}
		// set the timeout for the next time to run this function
		if (!timeSetEvent(suspendMs, 0, OneShotTimer, (DWORD)hWnd, TIME_ONESHOT)) {
			consoleLog("Failed to set Time Event after suspending Synced Process", true, false, true);
			return false;
		}
	} else {
		suspended = false;
		if (!syncedProcessMainThreadOnly) {
			// slower fallback in case the undocumented functions don't exist
			if (mode > 0) {
				// now we close the threads, backwards so it's in the same order
				// we need to be careful here
				// because we're subtracting from i, an unsigned int
				// hence why I ensure it's always greater than one
				// it's alright if we can't resume the thread
				// we'll just do it on the next loop
				for (unsigned int i = size(syncedProcessThreads);i > 0;i--) {
					if (syncedProcessThreads[i - 1]) {
						ResumeThread(syncedProcessThreads[i - 1]);
					}
				}
				// it's alright if we can't close the thread
				// we'll just do it on the next loop
				for (unsigned int i = syncedProcessThreads.size();i > 0;i--) {
					closeSyncedProcessThread(i - 1, syncedProcessThreads);
				}
			} else {
				originalNtResumeProcess(syncedProcess);
			}
		} else {
			ResumeThread(syncedProcessMainThread);
		}
		if (!timeSetEvent(resumeMs, 0, OneShotTimer, (DWORD)hWnd, TIME_ONESHOT)) {
			consoleLog("Failed to set Time Event after resuming Synced Process", true, false, true);
			return false;
		}
	}
	return true;
}

bool getCurrentMhz(ULONG &currentMhz) {
	//consoleLog("Getting Current Rate");
	SYSTEM_INFO systemInfo = {};
	GetSystemInfo(&systemInfo);
	const int SIZE_OF_PROCESSOR_POWER_INFORMATION = sizeof(PROCESSOR_POWER_INFORMATION) * systemInfo.dwNumberOfProcessors;
	PVOID lpProcessorPowerInformationOutputBuffer = new BYTE[SIZE_OF_PROCESSOR_POWER_INFORMATION];
	if (!lpProcessorPowerInformationOutputBuffer) {
		consoleLog("Failed to create Processor Power Information Output Buffer", true, false, true);
		return false;
	}

	// TODO: we assume all CPU cores have the same clock speed (it's not normal for anything else to be true right?)
	if (CallNtPowerInformation(ProcessorInformation, NULL, NULL, lpProcessorPowerInformationOutputBuffer, SIZE_OF_PROCESSOR_POWER_INFORMATION) != STATUS_SUCCESS) {
		consoleLog("Failed to call NtPowerInformation", true, false, true);
		delete[] lpProcessorPowerInformationOutputBuffer;
		lpProcessorPowerInformationOutputBuffer = NULL;
		return false;
	}

	PPROCESSOR_POWER_INFORMATION lpProcessorPowerInformation = (PPROCESSOR_POWER_INFORMATION)lpProcessorPowerInformationOutputBuffer;

	currentMhz = lpProcessorPowerInformation->CurrentMhz;

	delete[] lpProcessorPowerInformationOutputBuffer;
	lpProcessorPowerInformationOutputBuffer = NULL;
	lpProcessorPowerInformation = NULL;
	return true;
}

bool setProcessAffinity(HANDLE process, byte affinity) {
	consoleLog("Setting Process Affinity");
	// set synced process's affinity
	DWORD_PTR processAffinityMask = NULL;
	DWORD_PTR systemAffinityMask = NULL;
	if (!GetProcessAffinityMask(process, &processAffinityMask, &systemAffinityMask)) {
		consoleLog("Failed to get Process Affinity Mask", true, false, true);
		processAffinityMask = NULL;
		systemAffinityMask = NULL;
		return false;
	}
	// loop through all the cores, leaving only one bit lit
	BYTE processAffinityMaskLit = 0;
	for (BYTE i = 0;i < 32;i++) {
		if (processAffinityMaskLit < affinity) {
			if (processAffinityMask & (1 << i)) {
				processAffinityMaskLit++;
			}
		} else {
			// clear the bit
			processAffinityMask &= ~(1 << i);
		}
	}
	if (!SetProcessAffinityMask(process, processAffinityMask)) {
		consoleLog("Failed to set Process Affinity Mask", true, false, true);
		processAffinityMask = NULL;
		systemAffinityMask = NULL;
		return false;
	}
	processAffinityMask = NULL;
	systemAffinityMask = NULL;
	return true;
}

void getOriginalNtDll(HINSTANCE &originalNtDll,
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

int main(int argc, char** argv) {
	HANDLE oldCPUEmulatorMutex = CreateMutex(NULL, FALSE, "Old CPU Emulator");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		//consoleLog("You cannot run multiple instances of Old CPU Emulator.", true, false, true);
		return -2;
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);

	consoleLog("Old CPU Emulator 1.4.2");
	consoleLog("By Anthony Kleine", 2);

	ULONG currentMhz = 0;

	if (argc < 2) {
		consoleLog("You must pass the filename of an executable with which to create a process as the first argument.", 3, false, true);
		help();
		ReleaseMutex(oldCPUEmulatorMutex);
		return -1;
	}

	std::string argString = std::string(argv[1]);
	if (argString == "--help") {
		help();
		ReleaseMutex(oldCPUEmulatorMutex);
		return 0;
	} else if (argString == "--dev-get-current-mhz") {
		if (!getCurrentMhz(currentMhz)
			|| !currentMhz) {
			consoleLog("Failed to get Current Rate");
			ReleaseMutex(oldCPUEmulatorMutex);
			return -1;
		}
		consoleLog(std::to_string(currentMhz).c_str(), false);
		ReleaseMutex(oldCPUEmulatorMutex);
		return 0;
	}

	int args = 0;
	UINT refreshHz = 1000;
	ULONG targetMhz = 233;
	bool setProcessPriorityHigh = false;
	bool setSyncedProcessAffinityOne = false;
	bool syncedProcessMainThreadOnly = false;
	bool refreshHzFloorFifteen = false;
	char mode = -1;
	for (int i = 2; i < argc; ++i) {
		argString = std::string(argv[i]);
		if (argString == "--dev-get-current-mhz") {
			if (!getCurrentMhz(currentMhz)
				|| !currentMhz) {
				consoleLog("Failed to get Current Rate");
				ReleaseMutex(oldCPUEmulatorMutex);
				return -3;
			}
			consoleLog(std::to_string(currentMhz).c_str(), false);
			ReleaseMutex(oldCPUEmulatorMutex);
			return 0;
		} else if (argString == "-t") {
			if (!getCurrentMhz(currentMhz)
				|| !currentMhz) {
				consoleLog("Failed to get Current Rate", true, false, true);
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
			if (i + 1 < argc) {
				targetMhz = strtol(argv[++i], NULL, 10);
				if (!targetMhz) {
					consoleLog("The Target Rate must be a number.", true, false, true);
					help();
					ReleaseMutex(oldCPUEmulatorMutex);
					return -1;
				}
				if (currentMhz <= targetMhz) {
					consoleLog("The Target Rate cannot exceed or equal the Current Rate of ", false, false, true);
					consoleLog(std::to_string(currentMhz).c_str(), false, false, true);
					consoleLog(".", 3, false, true);
					help();
					ReleaseMutex(oldCPUEmulatorMutex);
					return -1;
				}
				args++;
			} else {
				consoleLog("-t option requires one argument: the Target Rate (in MHz, from 1 to your CPU's clock speed of ", false, false, true);
				consoleLog(std::to_string(currentMhz).c_str(), false, false, true);
				consoleLog(") to emulate.", 3, false, true);
				help();
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
		} else if (argString == "-r") {
			if (i + 1 < argc) {
				refreshHz = atoi(argv[++i]);
				if (!refreshHz) {
					consoleLog("The Refresh Rate cannot be zero.", 3, false, true);
					help();
					ReleaseMutex(oldCPUEmulatorMutex);
					return -1;
				}
				//args++;
			} else {
				consoleLog("-r option requires one argument: the Refresh Rate (in Hz, from 1 to 1000) at which to refresh.", 3, false, true);
				help();
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
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
			ReleaseMutex(oldCPUEmulatorMutex);
			return 0;
		} else {
			consoleLog("Unrecognized command line argument: ", false, false, true);
			consoleLog(argv[i], 3, false, true);
			help();
			ReleaseMutex(oldCPUEmulatorMutex);
			return -1;
		}
	}

	if (args < 1) {
		help();
		ReleaseMutex(oldCPUEmulatorMutex);
		return -1;
	}

	if (setProcessPriorityHigh) {
		if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
			consoleLog("Failed to set Synced Process Priority", true, false, true);
			ReleaseMutex(oldCPUEmulatorMutex);
			return -1;
		}
	}

	// create message only window
	WNDCLASSEX windowClassEx = {};
	windowClassEx.cbSize = sizeof(WNDCLASSEX);
	windowClassEx.lpfnWndProc = DefWindowProc;
	windowClassEx.hInstance = hInstance;
	windowClassEx.lpszClassName = "OLD_CPU_EMULATOR";
	HWND hWnd = NULL;
	if (!RegisterClassEx(&windowClassEx)) {
		consoleLog("Failed to register the Window Class", true, false, true);
		ReleaseMutex(oldCPUEmulatorMutex);
		return -1;
	} else {
		hWnd = CreateWindowEx(WS_OVERLAPPED, windowClassEx.lpszClassName, "Old CPU Emulator", WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hInstance, NULL);
		if (!hWnd) {
			consoleLog("Failed to create the Message Only Window", true, false, true);
			ReleaseMutex(oldCPUEmulatorMutex);
			return -1;
		}
	}

	HINSTANCE originalNtDll = NULL;
	NTQUERYSYSTEMINFORMATION originalNtQuerySystemInformation = NULL;
	NTSUSPENDPROCESS originalNtSuspendProcess = NULL;
	NTRESUMEPROCESS originalNtResumeProcess = NULL;
	getOriginalNtDll(originalNtDll, originalNtQuerySystemInformation, originalNtSuspendProcess, originalNtResumeProcess);

	DWORD syncedProcessID = 0;
	HANDLE syncedProcess = INVALID_HANDLE_VALUE;
	HANDLE syncedProcessMainThread = INVALID_HANDLE_VALUE;
	HANDLE hJob = NULL;
	if (!createSyncedProcess((LPSTR)argv[1], syncedProcess, syncedProcessMainThread, syncedProcessID, syncedProcessMainThreadOnly, hJob)
		|| syncedProcess == INVALID_HANDLE_VALUE
		|| syncedProcessMainThread == INVALID_HANDLE_VALUE) {
		consoleLog("Failed to create the Synced Process", true, false, true);
		ReleaseMutex(oldCPUEmulatorMutex);
		return -1;
	}

	if (setSyncedProcessAffinityOne) {
		if (!setProcessAffinity(syncedProcess, 1)) {
			consoleLog("Failed to set Synced Process Affinity", true, false, true);
			ReleaseMutex(oldCPUEmulatorMutex);
			terminateSyncedProcess(syncedProcess, syncedProcessMainThread, syncedProcessMainThreadOnly, hJob);
			return -1;
		}
	}

	UINT refreshMs = 0;
	UINT ms = 1;
	UINT s = 1000;
	DOUBLE suspend = ((DOUBLE)(currentMhz - targetMhz) / (DOUBLE)currentMhz);
	DOUBLE resume = ((DOUBLE)targetMhz / (DOUBLE)currentMhz);
	if (!beginRefreshTimePeriod(refreshHz, refreshMs, ms, s, suspend, resume, refreshHzFloorFifteen)) {
		consoleLog("Failed to begin Refresh Time Period", true, false, true);
		ReleaseMutex(oldCPUEmulatorMutex);
		terminateSyncedProcess(syncedProcess, syncedProcessMainThread, syncedProcessMainThreadOnly, hJob);
		return -1;
	}
	UINT suspendMs = suspend * refreshMs;
	UINT resumeMs = resume * refreshMs;

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
	// incite the timer that will begin syncing the process
	if (!timeSetEvent(resumeMs, 0, OneShotTimer, (DWORD)hWnd, TIME_ONESHOT)) {
		consoleLog("Failed to set Time Event before syncing the Synced Process", true, false, true);
		endRefreshTimePeriod(ms);
		ReleaseMutex(oldCPUEmulatorMutex);
		terminateSyncedProcess(syncedProcess, syncedProcessMainThread, syncedProcessMainThreadOnly, hJob);
		return -1;
	}
	// while the process is active
	while (WaitForSingleObject(syncedProcess, 0) == WAIT_TIMEOUT) {
		message = {};
		if (PeekMessage(&message, hWnd, 0, 0, PM_REMOVE)) {
			if (message.message == UWM_EMULATE_OLD_CPUS_SYNC_PROCESS) {
				if (!syncProcess(hWnd,
					syncedProcess,
					syncedProcessMainThread,
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
					consoleLog("Failed to sync the Synced Process", true, false, true);
					endRefreshTimePeriod(ms);
					ReleaseMutex(oldCPUEmulatorMutex);
					terminateSyncedProcess(syncedProcess, syncedProcessMainThread, syncedProcessMainThreadOnly, hJob);
					return -1;
				}
			}
		}
	}

	endRefreshTimePeriod(ms);
	ReleaseMutex(oldCPUEmulatorMutex);
	//terminateSyncedProcess(syncedProcess, syncedProcessMainThread, syncedProcessMainThreadOnly, hJob);
	return 0;
}