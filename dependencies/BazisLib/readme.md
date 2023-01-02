BazisLib Overview
=================

The BazisLib framework provides a convenient multi-platform abstraction layer for the following APIs:
* Thread and synchronization API
* File access API
* Atomic operations
* Reference counting
* Sockets (Windows Kernel not supported)
* FS path manipulation
* Configuration storage

BazisLib also provides some convenience classes that simplify driver development for Windows and MacOS in C++.

BazisLib can be compiled for the following targets:
* Windows user-mode using Visual Studio compiler
* Windows kernel-mode using WDK 7.x or 8.x
* Linux or MacOS user-mode using GCC or Clang
* MacOS kernel-mode

The best way to start exploring BazisLib is to open the tests\BigCrossPlatformTest\BigCrossPlatformTest.sln solution and try  building/running the Windows configuration. Other configurations of the solution 
target POSIX user-mode, Windows kernel-mode and MacOS kernel-mode environments. The Examples folder contains some other sample projects using BazisLib.
