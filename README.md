# BugChecker

<p align="center">
      <img src="https://raw.githubusercontent.com/vitoplantamura/BugChecker/master/assets/bcscreenshot1.png">
</p>

## Introduction

BugChecker is a [SoftICE](https://en.wikipedia.org/wiki/SoftICE)-like kernel and user debugger for Windows 11 (and Windows XP as well: it supports Windows versions from XP to 11, both x86 and x64). BugChecker doesn't require a second machine to be connected to the system being debugged, like in the case of WinDbg and KD. This version of BugChecker (unlike the [original version](https://github.com/vitoplantamura/BugChecker2002) developed 20 years ago) leverages the internal and undocumented KD API in NTOSKRNL. KD API allows WinDbg/KD to do calls like read/write virtual memory, read/write registers, place a breakpoint at an address etc.

By contrast, the original BugChecker, like SoftICE as well, used to "take over" the system, by hooking several kernel APIs (both exported and private), taking control of the APIC, sending IPIs etc. This approach increases complexity exponentially (and lowers system stability), since the implementation must be compatible with all the supported versions and sub-version of Windows (at the function signature level) as well as all the possible supported hardware configurations. Moreover, 20 years later, PatchGuard makes this solution impossible.

By contrast, this version of BugChecker, by intercepting calls to KdSendPacket and KdReceivePacket in the kernel, presents itself to the machine being debugged as a second system running an external kernel debugger, but, in reality, everything happens on the same machine. Typically this is achieved by [replacing KDCOM.DLL](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/kdserial-extensibility-code-samples) (which is the module that implements serial cable communication for the KD API in Windows) and by starting the system in kernel debugging mode. This approach (inspired by [VirtualKD](https://github.com/4d61726b/VirtualKD-Redux)) lowers complexity and increases stability and compatibility (and portability, for example, to ARM - and modularity, since the lower level debugger capabilities are implemented behind KdXxxPacket and could be replaced with a custom implementation). Moreover, the presence of a kernel debugger at boot time (although "fake") makes Windows disable PatchGuard.

At the moment, BugChecker requires a PS/2 keyboard for input and a linear framebuffer to write its output. Please note that the built-in keyboard of many modern laptops is still PS/2.

## Features

* Support for Windows XP up to Windows 11, x86 and x64, and SMP kernels. Support for WOW64 processes on x64.
* Integration of [QuickJSPP](https://github.com/c-smile/quickjspp), which is a port of [QuickJS](https://bellard.org/quickjs/) to MSVC++. Before calling QuickJS, BugChecker saves the FPU state (on x86) and switches to an expanded stack of 128KB.
* Commands accept JS expressions. For example, "U rip+rax*4" and "U MyJsFn(rax+2)" are valid commands. Custom functions can be defined in the Script Window. CPU registers are declared as global scope variables automatically by BugChecker.
* Support for PDB symbol files. PDB files can be specified manually or Symbol Loader can download them from a symbol server.
* JavaScript code can call the following asynchronous functions: WriteReg, ReadMem, WriteMem.
* Breakpoints can have a JS condition: if condition evaluates to 0, no "breakin" happens. This allows to set "Logpoints" and breakpoints that can change the flow of execution.
* Log window shows the messages sent to the kernel debugger (for example DbgPrint messages).
* JavaScript window with syntax highlighting.
* The tab key allows, given few digits, to cycle through all the hex numbers on the screen or, given few characters, to cycle through all the symbols containing those characters.
* EASTL and C++20 coroutines make creating new commands a breeze. **Feel free to send your pull requests!**

## Videos (Youtube)

Demonstration of BugChecker on Windows 11 22H2, inside VirtualBox 7.0.4. A JavaScript breakpoint condition is written that changes the flow of execution in an user mode thread.

[![Watch the video](https://img.youtube.com/vi/-EQ5Imy7zZo/hqdefault.jpg)](https://youtu.be/-EQ5Imy7zZo)

BugChecker running in a very constrained environment: a Raspberry Pi 4 (4GB RAM), via QEMU on Windows XP (512MB RAM). A breakpoint is used to log all the SYSENTER calls from user mode to the kernel. The service index is stored in a JavaScript array.

[![Watch the video](https://img.youtube.com/vi/bPUq8K6nViw/hqdefault.jpg)](https://youtu.be/bPUq8K6nViw)

Running BugChecker directly on bare metal, on an HP Pavilion Dv2000, which is an old PC with a PS/2 keyboard. The OS is Windows 7 Home 32bit.

[![Watch the video](https://img.youtube.com/vi/mzEBUHmknrA/hqdefault.jpg)](https://youtu.be/mzEBUHmknrA)

## Installation Instructions

### Introduction

Make sure that [Secure Boot](https://learn.microsoft.com/en-us/windows-hardware/design/device-experiences/oem-secure-boot) is disabled when installing and using BugChecker. Typically you can re-enable it later. If you are using VMware or VirtualBox, Secure Boot can be disabled in the virtual machine settings.

Consider also enabling legacy boot menu, if using Windows 8, 10 or 11, by using the command: **bcdedit /set "{current}" bootmenupolicy legacy**. It allows a smoother experience during boot, by allowing to select the BugChecker boot option and then to disable Driver Signature Enforcement at the same time.

### Instructions

The first step is to start Symbol Loader:

![Symbol Loader](https://raw.githubusercontent.com/vitoplantamura/BugChecker/master/assets/bcscreenshot2.png)

If necessary, disable the display drivers, by clicking on the "Disable Display Drvs" button. The same thing can be accomplished in Windows Device Manager. After the display drivers have been disabled, they remain disabled even after a system reboot. They can be re-enabled at any time later when not using BugChecker.

The point here is that BugChecker needs a linear framebuffer with a format of 32 bits-per-pixel, to draw its interface. When disabling the display drivers, Windows dismisses hardware acceleration for drawing its UI and falls back to VGA compatibility mode. If running on bare metal or VMware, you should disable display drivers. If running on VirtualBox, you should disable display drivers or set the vm_screen setting in BugChecker.dat, as described below. If running on QEMU, you don't need to disable display drivers but make sure to specify the "-vga std" display device.

Note that VGA compatibility mode could limit the maximum screen resolution. VMware is limited to a maximum resolution of 1152x864. QEMU with the "-vga std" display device doesn't suffer from this limitation.

Interestingly if BugChecker is installed on a system with more than one graphics card, it is possible to disable the display drivers of only one graphics card, which will be the card connected to the screen that will show the BugChecker UI. The second card (set as the main display) will retain all its 2D and 3D acceleration features, including OpenGL and DirectX support (NOTE: tested on VMware, with Windows 11 and a DisplayLink display). 

Then click on "Start Driver", then on "Auto Detect" and finally on "Save". "Auto Detect" should be able to determine width, height, physical address and stride of the framebuffer automatically. However, you can specify these settings manually (don't forget to click on "Save" when finished). If "Stride" is 0, it is calculated as "Width" * 4 automatically when starting the driver. "Address" (i.e. physical address of the framebuffer) can be get in Windows Device Manager, by clicking on "Properties" of the display device, under the "Resources" tab.

Then click on "Callback" in the "KDCOM Hook Method" section, then on "Copy/Replace Kdcom" and finally you can reboot the system.

This setup process has to be done only once and the display drivers can be re-enabled, if necessary. When using BugChecker however the display drivers must be disabled again, if required by your configuration.

### vm_screen setting for VirtualBox (Experimental)

The vm_screen setting in BugChecker.dat allows to open the BugChecker debugger UI in VirtualBox without specifying in advance a screen resolution in Symbol Loader and without disabling display drivers.

The idea is to write directly to the I/O ports and to the Command Buffer of the virtual display device in order to obtain the current screen resolution and to notify the hypervisor of any update in the framebuffer.

This solution was inspired by the [X.org xf86-video-vmware](https://github.com/freedesktop/xorg-xf86-video-vmware) driver.

This solution works only for VirtualBox VMs and by editing manually the BugChecker.dat file:

![vm_screen](https://raw.githubusercontent.com/vitoplantamura/BugChecker/master/assets/bcscreenshot3.png)

* In Symbol Loader, manually set the width and height of the framebuffer to the maximum resolution possible (i.e. the dimensions of your computer screen). Set the stride to 0.
* The BugChecker.dat file is created by Symbol Loader in "C:\Windows\BugChecker".
* The vm_screen setting should be added under "settings->framebuffer".
* The hierarchy of the settings in this file is determined by the tabulation characters (not spaces).
* The format of the setting is Command_Buffer_Start_Address (comma) Command_Buffer_End_Address (comma) I/O_Port_Base
* **IMPORTANT**: In the VM setting, under Display, select "VBoxSVGA" as the Graphics Controller and uncheck "Enable 3D Acceleration".

This is an experimental feature. In the future, this setting will be automatically added by Symbol Loader.

## Implemented Commands

The command name and syntax are chosen to be as close as possible to those of the original SoftICE for NT:

* **? javascript-expression**: Evaluate a javascript expression.
* **ADDR eprocess**: Switch to process context (returns control to OS).
* **BC list|***: Clear one or more breakpoints.
* **BD list|***: Disable one or more breakpoints.
* **BE list|***: Enable one or more breakpoints.
* **BL (no parameters)**: List all breakpoints.
* **BPX address [-t|-p|-kt thread|-kp process] [WHEN js-expression]**: Set a breakpoint on execution.
* **CLS (no parameters)**: Clear log window.
* **COLOR [normal bold reverse help line]|[reset]**: Display, set or reset the screen colors.
* **DB/DW/DD/DQ [address] [-l len-in-bytes]**: Display memory as 8/16/32/64-bit values.
* **EB/EW/ED/EQ address -v space-separated-values**: Edit memory as 8/16/32/64-bit values.
* **KL EN|IT**: Set keyboard layout.
* **LINES [rows-num]**: Display or set current display rows.
* **MOD [-u|-s] [search-string]**: Display module information.
* **P [RET]**: Execute one program step.
* **PAGEIN address**: Force a page of memory to be paged in (returns control to OS).
* **PROC [search-string]**: Display process information.
* **R register-name -v value**: Change a register value.
* **STACK [stack-ptr]**: Scan the stack searching for return addresses.
* **T (no parameters)**: Trace one instruction.
* **THREAD [-kt thread|-kp process]**: Display thread information.
* **U address|DEST**: Unassemble instructions.
* **VER (no parameters)**: Display version information.
* **WD [window-size]**: Toggle the Disassembler window or set its size.
* **WIDTH [columns-num]**: Display or set current display columns.
* **WR (no parameters)**: Toggle the Registers window.
* **WS [window-size]**: Toggle the Script window or set its size.
* **X (no parameters)**: Exit from the BugChecker screen.

## Build Instructions

### Prerequisites

* Visual Studio 2019
* Windows Driver Kit 7.1.0

**Note**: WDK should be installed in its default location, i.e. X:\WinDDK, where X is the drive where the BugChecker sources are saved.

A step-by-step guide to building the kernel driver is available [here](https://github.com/vitoplantamura/BugChecker/issues/8#issuecomment-1631970352).

### Visual Studio Projects Description

* **BugChecker**: this is the BugChecker kernel driver, where the entirety of the debugger is implemented. The "Release|x86" and "Release|x64" output files are included in the final package. During initialization, the driver loads its config file at "\SystemRoot\BugChecker\BugChecker.dat" (all the symbol files are stored in this directory too) and then it tries to locate "KDCOM.dll" in kernel space. If found, it tries to call its "KdSetBugCheckerCallbacks" exported function, thus hooking KdSendPacket and KdReceivePacket.
* **SymLoader**: this is the Symbol Loader. Only the "Release|x86" output file is included in the final package. Symbol Loader is used to change the BugChecker configuration (configuration is written to "\SystemRoot\BugChecker\BugChecker.dat"), to download PDB files and to install the custom KDCOM.dll module.
* **KDCOM**: this is the custom KDCOM.dll module that NTOSKRNL loads on system startup. It exports the "KdSetBugCheckerCallbacks" function that the driver calls to hook KdSendPacket and KdReceivePacket.
* **pdb**: this is the Ghidra "pdb" project. The original version outputs the contents of a PDB file to the standard output in xml format. The code was modified in order to generate a BCS file instead.
* **NativeUtil**: since Symbol Loader is a WOW64 application in Windows x64, the calls to those APIs that must be made from architecture native images were moved here (for example the calls to the Device and Driver Installation API).
* **HttpToHttpsProxy**: this is an ASP.NET Core application whose function is to act as an internet proxy for Symbol Loader when run in Windows XP. Since XP has outdated TLS support, Symbol Loader cannot download files from an arbitrary symbol server. After deploying this application in an IIS on the same network, it is possible to download files from a symbol server in Windows XP prepending "http://<YOUR_IIS_SERVER_IP>/HttpToHttpsProxy/" to the server URL in Symbol Loader.

## Credits

* [VirtualKD](https://github.com/4d61726b/VirtualKD-Redux): the first POC of BugChecker was built modifying VirtualKD.
* [BazisLib](https://github.com/sysprogs/BazisLib): the code behind the "Copy/Replace Kdcom + Add Boot Entry" button in Symbol Loader is from VirtualKD and uses BazisLib.
* [EASTL](https://github.com/electronicarts/EASTL): No way to use MSVC++ STL here. EASTL is an excellent alternative.
* [Ghidra](https://github.com/NationalSecurityAgency/ghidra): the "pdb" project in BugChecker is from Ghidra. It was modified to generate BCS files.
* [Zydis](https://github.com/zyantific/zydis): for the disassembler window in BugChecker.
* [QuickJSPP](https://github.com/c-smile/quickjspp), which is a port of [QuickJS](https://bellard.org/quickjs/) to MSVC++: for the JavaScript engine integrated in the kernel driver.
* [ReactOS](https://github.com/reactos/reactos): for the Windows KD internal type definitions.
* [SerenityOS](https://github.com/SerenityOS/serenity): for the low level bitmap manipulation functions used by the BugChecker memory allocator. Since I started BugChecker after I saw a video of [Andreas](https://www.youtube.com/andreaskling) (after 10 years of abstinence from C/C++ and any type of low level programming), I wanted to include a small piece of SerenityOS in BugChecker.
