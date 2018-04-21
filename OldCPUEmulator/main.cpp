#include "main.h"

// we will be using some undocumented ntDll features if possible because they're faster
// we need this program to be super optimized
typedef NTSTATUS(NTAPI *NTQUERYSYSTEMINFORMATION)(__in SYSTEM_INFORMATION_CLASS SystemInformationClass, __inout PVOID SystemInformation, __in ULONG SystemInformationLength, __out_opt PULONG ReturnLength);
typedef NTSTATUS(NTAPI *NTSUSPENDPROCESS)(IN HANDLE ProcessHandle);
typedef NTSTATUS(NTAPI *NTRESUMEPROCESS)(IN HANDLE ProcessHandle);
NTQUERYSYSTEMINFORMATION _NtQuerySystemInformation = NULL;
NTSUSPENDPROCESS _NtSuspendProcess = NULL;
NTRESUMEPROCESS _NtResumeProcess = NULL;
HANDLE syncedProcess = INVALID_HANDLE_VALUE;
HANDLE syncedProcessMainThread = INVALID_HANDLE_VALUE;
HANDLE oldCPUEmulatorMutex = INVALID_HANDLE_VALUE;
BOOL suspended = FALSE;
UINT ms = 1;
UINT s = 1000;

BOOL createSyncedProcess(LPSTR lpCommandLine, DWORD &syncedProcessID) {
	OutputDebugString("Creating Synced Process");

	// we create a job so that if either the process or the synced process ends
	// for whatever reason, we don't sync the process anymore
	HANDLE hJob = CreateJobObject(NULL, NULL);
	// CreateJobObject returns NULL, not INVALID_HANDLE_VALUE
	if (hJob == NULL) {
		return FALSE;
	}
	// this is how we kill both processes if either ends
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobobjectExtendedLimitInformation = {};
	jobobjectExtendedLimitInformation.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (!SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobobjectExtendedLimitInformation, sizeof(jobobjectExtendedLimitInformation))) {
		return FALSE;
	}
	// assign the current process to the job object
	// we assign the synced process later
	if (!AssignProcessToJobObject(hJob, GetCurrentProcess())) {
		return FALSE;
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
		return FALSE;
	}
	// get the handles and the ID
	syncedProcess = syncedProcessStartedInformation.hProcess;
	syncedProcessMainThread = syncedProcessStartedInformation.hThread;
	syncedProcessID = syncedProcessStartedInformation.dwProcessId;

	// assign the synced process to the job object
	// we've now set up the job process
	AssignProcessToJobObject(hJob, syncedProcess);

	return TRUE;
}

BOOL destroySyncedProcess() {
	OutputDebugString("Destroying Synced Process");
	if (syncedProcess != INVALID_HANDLE_VALUE) {
		if (!TerminateProcess(syncedProcess, 0)) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOL openSyncedProcessThread(DWORD syncedProcessThreadID, std::vector<HANDLE> &syncedProcessThreads) {
	//OutputDebugString("Opening Synced Process Thread");
	// open the synced process's thread
	// we don't bail if it fails
	// it's a very real possibility that it could fail
	// and we need to be able to just ignore it
	HANDLE syncedProcessThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, syncedProcessThreadID);
	if (syncedProcessThread != INVALID_HANDLE_VALUE) {
		try {
			// push it to the back of the others
			syncedProcessThreads.push_back(syncedProcessThread);
		} catch (...) {
			// Fail silently...
			return FALSE;
		}
	} else {
		return FALSE;
	}
	return TRUE;
}

VOID closeSyncedProcessThreads(std::vector<HANDLE> &syncedProcessThreads) {
	//OutputDebugString("Closing Synced Process Threads");
	for (unsigned int i = 0;i < syncedProcessThreads.size();i++) {
		if (syncedProcessThreads[i] != INVALID_HANDLE_VALUE) {
			CloseHandle(syncedProcessThreads[i]);
		}
	}
	syncedProcessThreads.clear();
}

BOOL beginRefreshTimePeriod(UINT &refreshHz, UINT &refreshMs, UINT &ms, UINT &s, DOUBLE suspend, DOUBLE resume, BOOL refreshHzFloorFifteen) {
	OutputDebugString("Beginning Refresh Time Period");
	TIMECAPS devCaps;
	if (timeGetDevCaps(&devCaps, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
		OutputDebugString("Failed to get Dev Caps.");
		return FALSE;
	}
	// one millisecond (approximately)
	ms = clamp(ms, devCaps.wPeriodMin, devCaps.wPeriodMax);
	// one second (approximately)
	s = clamp(s, devCaps.wPeriodMin, devCaps.wPeriodMax);
	if (timeBeginPeriod(ms) != TIMERR_NOERROR) {
		return FALSE;
	}
	// if we're suspended 25%
	// and resumed 75%
	// we'll be suspended a minimum of 1 Ms
	// and resumed a minimum of 3 Ms
	// 75% / 25% = 3 Ms
	// 3 Ms + 1 Ms = 4 Ms, our minRefreshMs
	DOUBLE minRefreshMs = ((DOUBLE)(max(suspend, resume)) / (DOUBLE)(min(suspend, resume)) * (DOUBLE)ms) + (DOUBLE)ms;
	DOUBLE maxRefreshHz = (DOUBLE)s / (DOUBLE)minRefreshMs;
	refreshHz = clamp(min(refreshHz, maxRefreshHz), ms, s);
	// we do this after in case the Refresh Rate before was well above the maximum
	if (refreshHzFloorFifteen) {
		refreshHz = clamp(min(((refreshHz + 8) / 15) * 15, maxRefreshHz), ms, s);
	}
	refreshMs = clamp((DOUBLE)s / (DOUBLE)refreshHz, ceil(minRefreshMs), s);
	return TRUE;
}

BOOL endRefreshTimePeriod() {
	OutputDebugString("Ending Refresh Time Period");
	if (timeEndPeriod(ms) != TIMERR_NOERROR) {
		OutputDebugString("Failed to End Refresh Time Period.");
		return FALSE;
	}
	return TRUE;
}

void CALLBACK OneShotTimer(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2) {
	// posts the message to incite the timer
	if (!PostMessage((HWND)dwUser, UWM_EMULATE_OLD_CPUS_SYNC_PROCESS, NULL, NULL)) {
		endRefreshTimePeriod();
		ReleaseMutex(oldCPUEmulatorMutex);
		destroySyncedProcess();
		TerminateProcess(GetCurrentProcess(), -1);
	}
}

BOOL syncProcess(HWND hWnd, HANDLE syncedProcess, DWORD syncedProcessID, std::vector<HANDLE> &syncedProcessThreads, BYTE mode, BOOL syncedProcessMainThreadOnly, UINT suspendMs, UINT resumeMs) {
	//OutputDebugString("Syncing Process");
	if (!suspended) {
		suspended = TRUE;
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
						return FALSE;
					}

					// allows us to walk along the threads
					THREADENTRY32 syncedProcessThreadWalker;
					syncedProcessThreadWalker.dwSize = sizeof(THREADENTRY32);

					if (!Thread32First(syncedProcessThreadSnapshot, &syncedProcessThreadWalker)) {
						CloseHandle(syncedProcessThreadSnapshot);
						return FALSE;
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
					NTSTATUS NtStatus = _NtQuerySystemInformation(SystemProcessInformation, lpSystemProcessInformationOutputBuffer, sizeOfLpSystemProcessInformationOutputBuffer, NULL);
					// if the buffer wasn't large enough
					while (NtStatus == STATUS_INFO_LENGTH_MISMATCH) {
						// DOUBLE the size
						sizeOfLpSystemProcessInformationOutputBuffer += sizeOfLpSystemProcessInformationOutputBuffer;
						delete[] lpSystemProcessInformationOutputBuffer;
						lpSystemProcessInformationOutputBuffer = new BYTE[sizeOfLpSystemProcessInformationOutputBuffer];
						NtStatus = _NtQuerySystemInformation(SystemProcessInformation, lpSystemProcessInformationOutputBuffer, sizeOfLpSystemProcessInformationOutputBuffer, NULL);
					}
					// check it worked
					if (NtStatus != STATUS_SUCCESS) {
						delete[] lpSystemProcessInformationOutputBuffer;
						lpSystemProcessInformationOutputBuffer = NULL;
						return FALSE;
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
								// there is no next process and we didn't loop through any threads!
								delete[] lpSystemProcessInformationOutputBuffer;
								lpSystemProcessInformationOutputBuffer = NULL;
								lpSystemProcessInformation = NULL;
								return FALSE;
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
				_NtSuspendProcess(syncedProcess);
			}
		} else {
			SuspendThread(syncedProcessMainThread);
		}
		// set the timeout for the next time to run this function
		timeSetEvent(suspendMs, 0, OneShotTimer, (DWORD)hWnd, TIME_ONESHOT);
		return TRUE;
	} else {
		suspended = FALSE;
		if (!syncedProcessMainThreadOnly) {
			// slower fallback in case the undocumented functions don't exist
			if (mode > 0) {
				// now we close the threads, backwards so it's in the same order
				// we need to be careful here
				// before, we check if i was greater than or equal to zero
				// and subtracted one from size
				// that caused an underflow if there was only one thread on the next loop
				// and since it's unsigned (because size returns an unsigned int)
				// there was no way of knowing
				// instead, we must set i to size directly and check
				// it's greater than zero, not greater than or equal to
				for (unsigned int i = size(syncedProcessThreads);i > 0;i--) {
					if (syncedProcessThreads[i]) {
						ResumeThread(syncedProcessThreads[i]);
					} else {
						// a thread magically disappeared on us
						closeSyncedProcessThreads(syncedProcessThreads);
						return FALSE;
					}
				}
				closeSyncedProcessThreads(syncedProcessThreads);
			} else {
				_NtResumeProcess(syncedProcess);
			}
		} else {
			ResumeThread(syncedProcessMainThread);
		}
		timeSetEvent(resumeMs, 0, OneShotTimer, (DWORD)hWnd, TIME_ONESHOT);
		return TRUE;
	}
}

BOOL getMaxMhz(ULONG &maxMhz) {
	//OutputDebugString("Getting MaxMhz");
	SYSTEM_INFO systemInfo = {};
	GetSystemInfo(&systemInfo);
	const int SIZE_OF_PROCESSOR_POWER_INFORMATION = sizeof(PROCESSOR_POWER_INFORMATION) * systemInfo.dwNumberOfProcessors;
	PVOID lpProcessorPowerInformationOutputBuffer = new BYTE[SIZE_OF_PROCESSOR_POWER_INFORMATION];

	// TODO: we assume all CPU cores have the same clock speed (it's not normal for anything else to be true right?)
	if (CallNtPowerInformation(ProcessorInformation, NULL, NULL, lpProcessorPowerInformationOutputBuffer, SIZE_OF_PROCESSOR_POWER_INFORMATION) != STATUS_SUCCESS) {
		delete lpProcessorPowerInformationOutputBuffer;
		lpProcessorPowerInformationOutputBuffer = NULL;
		return FALSE;
	}

	PPROCESSOR_POWER_INFORMATION lpProcessorPowerInformation = (PPROCESSOR_POWER_INFORMATION)lpProcessorPowerInformationOutputBuffer;

	maxMhz = lpProcessorPowerInformation->MaxMhz;

	delete lpProcessorPowerInformationOutputBuffer;
	lpProcessorPowerInformationOutputBuffer = NULL;
	lpProcessorPowerInformation = NULL;
	return TRUE;
}

BOOL setProcessAffinity(HANDLE process, BYTE affinity) {
	OutputDebugString("Setting Process Affinity");
	// set synced process's affinity
	DWORD_PTR processAffinityMask = NULL;
	DWORD_PTR systemAffinityMask = NULL;
	if (!GetProcessAffinityMask(process, &processAffinityMask, &systemAffinityMask)) {
		processAffinityMask = NULL;
		systemAffinityMask = NULL;
		return FALSE;
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
		processAffinityMask = NULL;
		systemAffinityMask = NULL;
		return FALSE;
	}
	processAffinityMask = NULL;
	systemAffinityMask = NULL;
	return TRUE;
}

void getNtDllExports() {
	OutputDebugString("Getting NtDll Exports");
	HINSTANCE NtDll = GetModuleHandle("ntdll.dll");
	if (NtDll) {
		_NtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(NtDll, "NtQuerySystemInformation");
		_NtSuspendProcess = (NTSUSPENDPROCESS)GetProcAddress(NtDll, "NtSuspendProcess");
		_NtResumeProcess = (NTRESUMEPROCESS)GetProcAddress(NtDll, "NtResumeProcess");
	}
	//CloseHandle(NtDll);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	oldCPUEmulatorMutex = CreateMutex(NULL, FALSE, "Old CPU Emulator");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		return -1;
	}

	const size_t MAX_ULONG_STRING_LENGTH = std::to_string(ULONG_MAX).length() + 1;

	ULONG maxMhz = 0;

	if (__argc < 2) {
		OutputDebugString("You must pass the filename of an executable with which to create a process as the first argument.\n\n");
		help();
		ReleaseMutex(oldCPUEmulatorMutex);
		return -1;
	}

	unsigned int sizeOfDebugString = 0;
	if (std::string(__argv[1]) == "--dev-get-max-mhz") {
		if (!getMaxMhz(maxMhz)
			|| !maxMhz) {
			ReleaseMutex(oldCPUEmulatorMutex);
			return -1;
		}
		char* debugString = new char[MAX_ULONG_STRING_LENGTH];
		sprintf_s(debugString, MAX_ULONG_STRING_LENGTH, "%d", maxMhz);
		OutputDebugString(debugString);
		delete[] debugString;
		ReleaseMutex(oldCPUEmulatorMutex);
		return 0;
	}

	int args = 0;
	UINT refreshHz = 1000;
	ULONG targetMhz = 233;
	BOOL setProcessPriorityHigh = FALSE;
	BOOL setSyncedProcessAffinityOne = FALSE;
	BOOL syncedProcessMainThreadOnly = FALSE;
	BOOL refreshHzFloorFifteen = FALSE;
	char mode = -1;
	for (int i = 2; i < __argc; ++i) {
		if (std::string(__argv[i]) == "--dev-get-max-mhz") {
			if (!getMaxMhz(maxMhz)
				|| !maxMhz) {
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
			char* debugString = new char[MAX_ULONG_STRING_LENGTH];
			sprintf_s(debugString, MAX_ULONG_STRING_LENGTH, "%d", maxMhz);
			OutputDebugString(debugString);
			delete[] debugString;
			ReleaseMutex(oldCPUEmulatorMutex);
			return 0;
		} else if (std::string(__argv[i]) == "-t") {
			if (!getMaxMhz(maxMhz)
				|| !maxMhz) {
				OutputDebugString("Failed to get Max Rate.");
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
			if (i + 1 < __argc) {
				targetMhz = atoi(__argv[++i]);
				if (maxMhz < targetMhz) {
					sizeOfDebugString = MAX_ULONG_STRING_LENGTH + 41;
					char* debugString = new char[sizeOfDebugString];
					sprintf_s(debugString, sizeOfDebugString, "Target Rate may not exceed Max Rate of %d.", maxMhz);
					OutputDebugString(debugString);
					delete[] debugString;
					help();
					ReleaseMutex(oldCPUEmulatorMutex);
					return -1;
				}
				args++;
			} else {
				sizeOfDebugString = MAX_ULONG_STRING_LENGTH + 142;
				char* debugString = new char[sizeOfDebugString];
				sprintf_s(debugString, sizeOfDebugString, "-t option requires one argument: the Target Rate (in MHz, from 1 to your CPU's clock speed) to emulate, from 1 to your CPU clock speed of %d.\n\n", maxMhz);
				OutputDebugString(debugString);
				delete[] debugString;
				help();
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
		} else if (std::string(__argv[i]) == "-r") {
			if (i + 1 < __argc) {
				refreshHz = atoi(__argv[++i]);
				if (!refreshHz) {
					OutputDebugString("Refresh Rate cannot be zero.");
					help();
					ReleaseMutex(oldCPUEmulatorMutex);
					return -1;
				}
				//args++;
			} else {
				OutputDebugString("-r option requires one argument: the Refresh Rate, from 1 to 1000.\n\n");
				help();
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
		} else if (std::string(__argv[i]) == "--set-process-priority-high") {
			setProcessPriorityHigh = TRUE;
		} else if (std::string(__argv[i]) == "--set-synced-process-affinity-one") {
			setSyncedProcessAffinityOne = TRUE;
		} else if (std::string(__argv[i]) == "--synced-process-main-thread-only") {
			syncedProcessMainThreadOnly = TRUE;
		} else if (std::string(__argv[i]) == "--refresh-rate-floor-fifteen") {
			refreshHzFloorFifteen = TRUE;
		} else if (std::string(__argv[i]) == "--dev-force-mode-0") {
			mode = 0;
		} else if (std::string(__argv[i]) == "--dev-force-mode-1") {
			mode = 1;
		} else if (std::string(__argv[i]) == "--dev-force-mode-2") {
			mode = 2;
		} else {
			sizeOfDebugString = sizeof(__argv[i]) + 37;
			char* debugString = new char[sizeOfDebugString];
			sprintf_s(debugString, sizeOfDebugString, "Unrecognized command line argument: %s", __argv[i]);
			OutputDebugString(debugString);
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
			OutputDebugString("Failed to set synced process priority.");
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
		OutputDebugString("Failed to register the Window Class.");
		ReleaseMutex(oldCPUEmulatorMutex);
		return -1;
	} else {
		hWnd = CreateWindowEx(WS_OVERLAPPED, windowClassEx.lpszClassName, "Old CPU Emulator", WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hInstance, NULL);
		if (!hWnd) {
			OutputDebugString("Failed to create the Message Only Window.");
			ReleaseMutex(oldCPUEmulatorMutex);
			return -1;
		}
	}

	getNtDllExports();

	DWORD syncedProcessID = 0;
	if (!createSyncedProcess((LPSTR)__argv[1], syncedProcessID)
		|| syncedProcess == INVALID_HANDLE_VALUE
		|| syncedProcessMainThread == INVALID_HANDLE_VALUE) {
		ReleaseMutex(oldCPUEmulatorMutex);
		OutputDebugString("Failed to create the synced process.");
		return -1;
	}

	if (setSyncedProcessAffinityOne) {
		if (!setProcessAffinity(syncedProcess, 1)) {
			OutputDebugString("Failed to set synced process affinity.");
			ReleaseMutex(oldCPUEmulatorMutex);
			destroySyncedProcess();
			return -1;
		}
	}

	UINT refreshMs = 0;
	ms = 1;
	s = 1000;
	DOUBLE suspend = ((DOUBLE)(maxMhz - targetMhz) / (DOUBLE)maxMhz);
	DOUBLE resume = ((DOUBLE)targetMhz / (DOUBLE)maxMhz);
	if (!beginRefreshTimePeriod(refreshHz, refreshMs, ms, s, suspend, resume, refreshHzFloorFifteen)) {
		OutputDebugString("Failed to begin Refresh Time Period.");
		ReleaseMutex(oldCPUEmulatorMutex);
		destroySyncedProcess();
		return -1;
	}
	UINT suspendMs = suspend * refreshMs;
	UINT resumeMs = resume * refreshMs;

	// determine mode if relevant
	if (!syncedProcessMainThreadOnly) {
		if (mode == -1) {
			if (!_NtSuspendProcess || !_NtResumeProcess) {
				if (!_NtQuerySystemInformation) {
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

	MSG message = {};
	if (!PostMessage(hWnd, UWM_EMULATE_OLD_CPUS_SYNC_PROCESS, NULL, NULL)) {
		OutputDebugString("Failed to post the message to sync the process.");
		endRefreshTimePeriod();
		ReleaseMutex(oldCPUEmulatorMutex);
		destroySyncedProcess();
		return -1;
	}
	// while the process is active...
	while (WaitForSingleObject(syncedProcess, 0) == WAIT_TIMEOUT) {
		message = {};
		if (PeekMessage(&message, hWnd, 0, 0, PM_REMOVE)) {
			if (message.message == UWM_EMULATE_OLD_CPUS_SYNC_PROCESS) {
				if (!syncProcess(hWnd, syncedProcess, syncedProcessID, syncedProcessThreads, mode, syncedProcessMainThreadOnly, suspendMs, resumeMs)) {
					endRefreshTimePeriod();
					ReleaseMutex(oldCPUEmulatorMutex);
					destroySyncedProcess();
					return -1;
				}
			}
		}
	}

	endRefreshTimePeriod();
	ReleaseMutex(oldCPUEmulatorMutex);
	destroySyncedProcess();
	return 0;
}