This solution demonstrates two kernel-mode Windows virtual device drivers:
	* File disk
	* RAM disk
	
To build it, follow the steps below:
1. Build the WinKernel.sln solution for your platform.
2. Build the SampleInstaller\SampleInstaller.sln solution.
3. Go to AllOutputs\<platform> and run the SampleInstaller.exe/SampleInstaller64.exe on the machine where you want to deploy the drivers