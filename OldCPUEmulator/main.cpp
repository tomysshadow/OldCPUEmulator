#include "main.h"

BOOL createSyncedProcess(LPSTR lpCommandLine, HANDLE &syncedProcess, HANDLE &syncedProcessMainThread, DWORD &syncedProcessID, BOOL syncedProcessMainThreadOnly, HANDLE &hJob) {
	OutputDebugStringNewline("Creating Synced Process");

	// we create a job so that if either the process or the synced process ends
	// for whatever reason, we don't sync the process anymore
	hJob = CreateJobObject(NULL, NULL);
	// CreateJobObject returns NULL, not INVALID_HANDLE_VALUE
	if (!hJob) {
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
	if (!syncedProcessMainThreadOnly) {
		if (!CloseHandle(syncedProcessMainThread)) {
			return FALSE;
		}
	}
	syncedProcessID = syncedProcessStartedInformation.dwProcessId;

	// assign the synced process to the job object
	// we've now set up the job process
	if (!AssignProcessToJobObject(hJob, syncedProcess)) {
		return FALSE;
	}

	return TRUE;
}

BOOL destroySyncedProcess(HANDLE &syncedProcess, HANDLE &syncedProcessMainThread, BOOL syncedProcessMainThreadOnly, HANDLE &hJob) {
	OutputDebugStringNewline("Destroying Synced Process");
	if (syncedProcessMainThreadOnly) {
		if (syncedProcessMainThread != INVALID_HANDLE_VALUE) {
			if (!CloseHandle(syncedProcessMainThread)) {
				return FALSE;
			}
		}
	}
	// if not already closed
	if (syncedProcess != INVALID_HANDLE_VALUE) {
		if (!TerminateProcess(syncedProcess, 0)) {
			return FALSE;
		}
	}
	if (hJob != INVALID_HANDLE_VALUE) {
		CloseHandle(hJob);
	}
	return TRUE;
}

BOOL openSyncedProcessThread(DWORD syncedProcessThreadID, std::vector<HANDLE> &syncedProcessThreads) {
	//OutputDebugStringNewline("Opening Synced Process Thread");
	// open the synced process's thread
	// we don't bail if it fails
	// it's a very real possibility that it could fail
	// and we need to be able to just ignore it
	HANDLE syncedProcessThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, syncedProcessThreadID);
	if (syncedProcessThread != INVALID_HANDLE_VALUE) {
		// push it to the back of the others
		syncedProcessThreads.push_back(syncedProcessThread);
	} else {
		return FALSE;
	}
	return TRUE;
}

BOOL closeSyncedProcessThread(unsigned int i, std::vector<HANDLE> &syncedProcessThreads) {
	//OutputDebugStringNewline("Closing Synced Process Thread");
	if (syncedProcessThreads[i] != INVALID_HANDLE_VALUE) {
		if (!CloseHandle(syncedProcessThreads[i])) {
			return FALSE;
		}
	}
	syncedProcessThreads.erase(syncedProcessThreads.begin() + i);
	return TRUE;
}

BOOL beginRefreshTimePeriod(UINT &refreshHz, UINT &refreshMs, UINT &ms, UINT &s, DOUBLE suspend, DOUBLE resume, BOOL refreshHzFloorFifteen) {
	OutputDebugStringNewline("Beginning Refresh Time Period");
	TIMECAPS devCaps;
	if (timeGetDevCaps(&devCaps, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
		OutputDebugStringNewline("Failed to get Dev Caps");
		return FALSE;
	}
	// one millisecond (approximately)
	ms = clamp(ms, devCaps.wPeriodMin, devCaps.wPeriodMax);
	// one second (approximately)
	s = clamp(s, devCaps.wPeriodMin, devCaps.wPeriodMax);
	if (timeBeginPeriod(ms) != TIMERR_NOERROR) {
		return FALSE;
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
	return TRUE;
}

BOOL endRefreshTimePeriod(UINT ms) {
	OutputDebugStringNewline("Ending Refresh Time Period");
	if (timeEndPeriod(ms) != TIMERR_NOERROR) {
		return FALSE;
	}
	return TRUE;
}

void CALLBACK OneShotTimer(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2) {
	// posts the message to incite the timer
	// you're not supposed to call anything other than PostMessage in these callbacks
	PostMessage((HWND)dwUser, UWM_EMULATE_OLD_CPUS_SYNC_PROCESS, NULL, NULL);
}

BOOL syncProcess(HWND hWnd,
				 HANDLE syncedProcess,
				 HANDLE syncedProcessMainThread,
				 DWORD syncedProcessID,
				 std::vector<HANDLE> &syncedProcessThreads,
				 BOOL syncedProcessMainThreadOnly,
				 BOOL &suspended,
				 BYTE mode,
				 UINT suspendMs,
				 UINT resumeMs,
				 NTQUERYSYSTEMINFORMATION originalNtQuerySystemInformation,
				 NTSUSPENDPROCESS originalNtSuspendProcess,
				 NTRESUMEPROCESS originalNtResumeProcess) {
	//OutputDebugStringNewline("Syncing Process");
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
					NTSTATUS NtStatus = originalNtQuerySystemInformation(SystemProcessInformation, lpSystemProcessInformationOutputBuffer, sizeOfLpSystemProcessInformationOutputBuffer, NULL);
					// if the buffer wasn't large enough
					while (NtStatus == STATUS_INFO_LENGTH_MISMATCH) {
						// double the size
						sizeOfLpSystemProcessInformationOutputBuffer += sizeOfLpSystemProcessInformationOutputBuffer;
						delete[] lpSystemProcessInformationOutputBuffer;
						lpSystemProcessInformationOutputBuffer = new BYTE[sizeOfLpSystemProcessInformationOutputBuffer];
						NtStatus = originalNtQuerySystemInformation(SystemProcessInformation, lpSystemProcessInformationOutputBuffer, sizeOfLpSystemProcessInformationOutputBuffer, NULL);
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
								// there is no next process and we didn't loop through any threads
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
				originalNtSuspendProcess(syncedProcess);
			}
		} else {
			SuspendThread(syncedProcessMainThread);
		}
		// set the timeout for the next time to run this function
		if (!timeSetEvent(suspendMs, 0, OneShotTimer, (DWORD)hWnd, TIME_ONESHOT)) {
			return FALSE;
		}
		return TRUE;
	} else {
		suspended = FALSE;
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
			return FALSE;
		}
		return TRUE;
	}
}

BOOL getCurrentMhz(ULONG &currentMhz) {
	//OutputDebugStringNewline("Getting Current Rate");
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

	currentMhz = lpProcessorPowerInformation->CurrentMhz;

	delete[] lpProcessorPowerInformationOutputBuffer;
	lpProcessorPowerInformationOutputBuffer = NULL;
	lpProcessorPowerInformation = NULL;
	return TRUE;
}

BOOL setProcessAffinity(HANDLE process, BYTE affinity) {
	OutputDebugStringNewline("Setting Process Affinity");
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

void openOriginalNtDll(HINSTANCE &originalNtDll,
					   NTQUERYSYSTEMINFORMATION &originalNtQuerySystemInformation,
					   NTSUSPENDPROCESS &originalNtSuspendProcess,
					   NTRESUMEPROCESS &originalNtResumeProcess) {
	OutputDebugStringNewline("Opening Original NtDll");
	originalNtDll = GetModuleHandle("ntdll.dll");
	if (originalNtDll) {
		originalNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(originalNtDll, "NtQuerySystemInformation");
		originalNtSuspendProcess = (NTSUSPENDPROCESS)GetProcAddress(originalNtDll, "NtSuspendProcess");
		originalNtResumeProcess = (NTRESUMEPROCESS)GetProcAddress(originalNtDll, "NtResumeProcess");
	}
}

void closeOriginalNtDll(HINSTANCE &originalNtDll,
						NTQUERYSYSTEMINFORMATION &originalNtQuerySystemInformation,
						NTSUSPENDPROCESS &originalNtSuspendProcess,
						NTRESUMEPROCESS &originalNtResumeProcess) {
	OutputDebugStringNewline("Closing Original NtDll");
	if (originalNtDll) {
		originalNtQuerySystemInformation = NULL;
		originalNtSuspendProcess = NULL;
		originalNtResumeProcess = NULL;
		// if we free the library prematurely it could cause a crash
		//FreeLibrary(originalNtDll);
	}
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	HANDLE oldCPUEmulatorMutex = CreateMutex(NULL, FALSE, "Old CPU Emulator");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		OutputDebugStringNewline("You cannot run multiple instances of Old CPU Emulator.");
		return -1;
	}

	OutputDebugStringNewline("Old CPU Emulator 1.1.5");
	OutputDebugStringNewline("By Anthony Kleine\n");

	const size_t MAX_ULONG_CSTRING_LENGTH = std::to_string(ULONG_MAX).length() + 1;

	ULONG currentMhz = 0;

	if (__argc < 2) {
		OutputDebugStringNewline("You must pass the filename of an executable with which to create a process as the first argument.\n\n");
		help();
		ReleaseMutex(oldCPUEmulatorMutex);
		return -1;
	}

	if (std::string(__argv[1]) == "--dev-get-current-mhz") {
		char* currentMhzString = (char*)malloc(MAX_ULONG_CSTRING_LENGTH);
		if (!getCurrentMhz(currentMhz)
			|| !currentMhz
			|| !currentMhzString) {
			ReleaseMutex(oldCPUEmulatorMutex);
			return -1;
		}
		if (sprintf_s(currentMhzString, MAX_ULONG_CSTRING_LENGTH, "%d", currentMhz) < 0) {
			delete[] currentMhzString;
			ReleaseMutex(oldCPUEmulatorMutex);
			return -1;
		}
		OutputDebugStringNewline(currentMhzString);
		delete[] currentMhzString;
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
		if (std::string(__argv[i]) == "--dev-get-current-mhz") {
			char* currentMhzString = (char*)malloc(MAX_ULONG_CSTRING_LENGTH);
			if (!getCurrentMhz(currentMhz)
				|| !currentMhz
				|| !currentMhzString) {
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
			if (sprintf_s(currentMhzString, MAX_ULONG_CSTRING_LENGTH, "%d", currentMhz) < 0) {
				delete[] currentMhzString;
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
			OutputDebugStringNewline(currentMhzString);
			delete[] currentMhzString;
			ReleaseMutex(oldCPUEmulatorMutex);
			return 0;
		} else if (std::string(__argv[i]) == "-t") {
			if (!getCurrentMhz(currentMhz)
				|| !currentMhz) {
				OutputDebugStringNewline("Failed to get Current Rate");
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
			if (i + 1 < __argc) {
				targetMhz = atoi(__argv[++i]);
				if (currentMhz < targetMhz) {
					char* currentMhzString = (char*)malloc(MAX_ULONG_CSTRING_LENGTH);
					if (!currentMhzString) {
						ReleaseMutex(oldCPUEmulatorMutex);
						return -1;
					}
					if (sprintf_s(currentMhzString, MAX_ULONG_CSTRING_LENGTH, "%d", currentMhz) < 0) {
						delete[] currentMhzString;
						ReleaseMutex(oldCPUEmulatorMutex);
						return -1;
					}
					OutputDebugString("The Target Rate may not exceed the Current Rate of ");
					OutputDebugString(currentMhzString);
					OutputDebugStringNewline(".\n\n");
					delete[] currentMhzString;
					help();
					ReleaseMutex(oldCPUEmulatorMutex);
					return -1;
				}
				args++;
			} else {
				char* currentMhzString = (char*)malloc(MAX_ULONG_CSTRING_LENGTH);
				if (!currentMhzString) {
					ReleaseMutex(oldCPUEmulatorMutex);
					return -1;
				}
				if (sprintf_s(currentMhzString, MAX_ULONG_CSTRING_LENGTH, "%d", currentMhz) < 0) {
					delete[] currentMhzString;
					ReleaseMutex(oldCPUEmulatorMutex);
					return -1;
				}
				OutputDebugString("-t option requires one argument: the Target Rate (in MHz, from 1 to your CPU's clock speed of ");
				OutputDebugString(currentMhzString);
				OutputDebugStringNewline(") to emulate.\n\n");
				delete[] currentMhzString;
				help();
				ReleaseMutex(oldCPUEmulatorMutex);
				return -1;
			}
		} else if (std::string(__argv[i]) == "-r") {
			if (i + 1 < __argc) {
				refreshHz = atoi(__argv[++i]);
				if (!refreshHz) {
					OutputDebugStringNewline("The Refresh Rate cannot be zero.");
					help();
					ReleaseMutex(oldCPUEmulatorMutex);
					return -1;
				}
				//args++;
			} else {
				OutputDebugStringNewline("-r option requires one argument: the Refresh Rate (in Hz, from 1 to 1000) at which to refresh.\n\n");
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
			OutputDebugString("Unrecognized command line argument: ");
			OutputDebugString(__argv[i]);
			OutputDebugStringNewline("\n\n");
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
			OutputDebugStringNewline("Failed to set Synced Process Priority");
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
		OutputDebugStringNewline("Failed to register the Window Class");
		ReleaseMutex(oldCPUEmulatorMutex);
		return -1;
	} else {
		hWnd = CreateWindowEx(WS_OVERLAPPED, windowClassEx.lpszClassName, "Old CPU Emulator", WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hInstance, NULL);
		if (!hWnd) {
			OutputDebugStringNewline("Failed to create the Message Only Window");
			ReleaseMutex(oldCPUEmulatorMutex);
			return -1;
		}
	}

	HINSTANCE originalNtDll = NULL;
	NTQUERYSYSTEMINFORMATION originalNtQuerySystemInformation = NULL;
	NTSUSPENDPROCESS originalNtSuspendProcess = NULL;
	NTRESUMEPROCESS originalNtResumeProcess = NULL;
	openOriginalNtDll(originalNtDll, originalNtQuerySystemInformation, originalNtSuspendProcess, originalNtResumeProcess);

	DWORD syncedProcessID = 0;
	HANDLE syncedProcess = INVALID_HANDLE_VALUE;
	HANDLE syncedProcessMainThread = INVALID_HANDLE_VALUE;
	HANDLE hJob = NULL;
	if (!createSyncedProcess((LPSTR)__argv[1], syncedProcess, syncedProcessMainThread, syncedProcessID, syncedProcessMainThreadOnly, hJob)
		|| syncedProcess == INVALID_HANDLE_VALUE
		|| syncedProcessMainThread == INVALID_HANDLE_VALUE) {
		closeOriginalNtDll(originalNtDll, originalNtQuerySystemInformation, originalNtSuspendProcess, originalNtResumeProcess);
		ReleaseMutex(oldCPUEmulatorMutex);
		OutputDebugStringNewline("Failed to create the Synced Process");
		return -1;
	}

	if (setSyncedProcessAffinityOne) {
		if (!setProcessAffinity(syncedProcess, 1)) {
			OutputDebugStringNewline("Failed to set Synced Process Affinity");
			closeOriginalNtDll(originalNtDll, originalNtQuerySystemInformation, originalNtSuspendProcess, originalNtResumeProcess);
			ReleaseMutex(oldCPUEmulatorMutex);
			destroySyncedProcess(syncedProcess, syncedProcessMainThread, syncedProcessMainThreadOnly, hJob);
			return -1;
		}
	}

	UINT refreshMs = 0;
	UINT ms = 1;
	UINT s = 1000;
	DOUBLE suspend = ((DOUBLE)(currentMhz - targetMhz) / (DOUBLE)currentMhz);
	DOUBLE resume = ((DOUBLE)targetMhz / (DOUBLE)currentMhz);
	if (!beginRefreshTimePeriod(refreshHz, refreshMs, ms, s, suspend, resume, refreshHzFloorFifteen)) {
		OutputDebugStringNewline("Failed to begin Refresh Time Period");
		closeOriginalNtDll(originalNtDll, originalNtQuerySystemInformation, originalNtSuspendProcess, originalNtResumeProcess);
		ReleaseMutex(oldCPUEmulatorMutex);
		destroySyncedProcess(syncedProcess, syncedProcessMainThread, syncedProcessMainThreadOnly, hJob);
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

	MSG message = {};
	BOOL suspended = FALSE;
	// incite the timer that will begin syncing the process
	if (!timeSetEvent(resumeMs, 0, OneShotTimer, (DWORD)hWnd, TIME_ONESHOT)) {
		endRefreshTimePeriod(ms);
		closeOriginalNtDll(originalNtDll, originalNtQuerySystemInformation, originalNtSuspendProcess, originalNtResumeProcess);
		ReleaseMutex(oldCPUEmulatorMutex);
		destroySyncedProcess(syncedProcess, syncedProcessMainThread, syncedProcessMainThreadOnly, hJob);
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
					endRefreshTimePeriod(ms);
					closeOriginalNtDll(originalNtDll, originalNtQuerySystemInformation, originalNtSuspendProcess, originalNtResumeProcess);
					ReleaseMutex(oldCPUEmulatorMutex);
					destroySyncedProcess(syncedProcess, syncedProcessMainThread, syncedProcessMainThreadOnly, hJob);
					return -1;
				}
			}
		}
	}

	endRefreshTimePeriod(ms);
	closeOriginalNtDll(originalNtDll, originalNtQuerySystemInformation, originalNtSuspendProcess, originalNtResumeProcess);
	ReleaseMutex(oldCPUEmulatorMutex);
	destroySyncedProcess(syncedProcess, syncedProcessMainThread, syncedProcessMainThreadOnly, hJob);
	return 0;
}