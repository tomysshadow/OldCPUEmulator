Old CPU Simulator 1.6.6
By Anthony Kleine

	This command line tool simulates running a process on a CPU with a
	slower clock speed in order to make old games run at the correct speed
	or underclock CPU intensive processes like video encoding.


Usage: OldCPUSimulator -t targetRate [options] -sw software.exe

-t targetRate
	The Target Rate (in MHz, from 1 to your CPU's current clock speed)
	to simulate.
	This argument is required.
	
	Try 233 to simulate an Intel Pentium II 233 MHz from the late 1990s.
	Try 350 to simulate an Intel Pentium II 350 MHz from the early 2000s.

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
	If not specified, Old CPU Simulator will default to the
	smoothest possible playback setting.
	
	Try 60, 30 or 15 for gaming, 1 to 6 for video encoding.

--set-process-priority-high
	Set the process priority of Old CPU Simulator to High,
	in order to potentially improve the accuracy of the simulation.

--set-synced-process-affinity-one
	Set the process affinity of the synced process
	to one, which may make the speed more consistent and prevent crashes.
	May be unstable with newer games.

--synced-process-main-thread-only
	Try enabling this if the process you're running
	seems to be barely affected by Old CPU Simulator,
	as it may increase accuracy on some Windows versions,
	as well as reduce audio stutters,
	but could also introduce instability with some games.

--refresh-rate-floor-fifteen
	Rounds Refresh Rate to the nearest multiple of 15 if applicable.

-sw (or --software)
	The software that will be created as the synced process.
	This argument is required.

	It must be given last, after all the options. It may
	include command line arguments, which will be passed
	to the software specified.
	

Notice: this tool was previously titled Old CPU Emulator. It was renamed because it is not an emulator.