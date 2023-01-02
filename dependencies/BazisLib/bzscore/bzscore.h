#pragma once

/*! \mainpage BazisLib framework
 
  \section intro_sec Introduction
  BazisLib is an library that accelerates the development of system-level applications and drivers. It consists of 2 main parts:
	* Core library (bzscore) that defines a platform-independent abstraction layer for various system API (e.g. file access)
	* Helper libraries
		* bzswin provides convenient C++ wrappers for various Win32 API calls

	\section platform_types The following platforms are supported by BazisLib:
	* User-mode Win32 (_WIN32)
	* Windows kernel-mode environment (_DDK_)
*/

#include "cmndef.h"