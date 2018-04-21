Old CPU Emulator 1.0.3
By Anthony Kleine

	This command line tool emulates running a process on a CPU with a
	slower clock speed in order to make old games run at the correct speed
	or underclock CPU intensive processes like video encoding.


Usage: OldCPUEmulator pathname.exe -t targetRate [options]

-t targetRate
	The Target Rate (in MHz, from 1 to your CPU's clock speed) to emulate.
	This argument is required.
	
	Try 233 to emulate an Intel Pentium II 233 MHz from the late 1990s.
	Try 750 to emulate an Intel Pentium III 750 MHz from the early 2000s.

	Go to http://intel.com/pressroom/kits/quickrefyr.htm
	for a quick reference of year to clock speed.
	Note that many of the measurements in said reference
	are in GHz. This tool uses MHz.

-r refreshRate
	The Refresh Rate (in Hz, from 1 to 1000) at which to refresh.
	This argument is not required.
	
	Effectively an accuracy meter.
	Lower numbers are more accurate but result in choppier playback.
	Higher numbers are less accurate but result in smoother playback.
	If not specified, Old CPU Emulator will default to the
	smoothest possible playback setting.
	
	Try 60, 30 or 15 for gaming, 1 to 6 for video encoding.

--set-process-priority-high
	Set the process priority of Old CPU Emulator to High,
	in order to potentially improve the accuracy of the emulation.

--set-synced-process-affinity-one
	Set the process affinity of the synced process
	to one, which may make the speed more consistent and prevent crashes.
	May be unstable with newer games.

--synced-process-main-thread-only
	Only throttle the speed of the synced process's main thread,
	which may reduce audio stutters, and improve the accuracy of
	the emulation on some Windows versions,
	but could also introduce instability with some games.

--refresh-rate-floor-fifteen
	Rounds Refresh Rate to the nearest multiple of 15 if applicable.