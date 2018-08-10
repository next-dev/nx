//----------------------------------------------------------------------------------------------------------------------
//! Defines all the configuration constants and macros
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

//! Show a console on Win32 platform
#define NX_DEBUG_CONSOLE        0

//----------------------------------------------------------------------------------------------------------------------

// We usually want to output if the debug console is open
#if NX_DEBUG_CONSOLE
#   include <iostream>
#endif

//----------------------------------------------------------------------------------------------------------------------
// Version information
//----------------------------------------------------------------------------------------------------------------------

//! The major version of this software.
//!
//! If this is 0, the version is either a dev, alpha or beta version, i.e. a pre-release version.  This should change
//! major additions have been added to the emulator.  At this point the minor version should be reset to 0.
#define NX_MAJOR_VERSION        0

//! The minor version of this software.
//!
//! If the major version is 0, then a minor version of 0 means a dev version.  Otherwise, a value of 1 to 8 is an
//! alpha version.  Finally, a value of 9 is a beta version.  This should be changed if minor alterations have been
//! made to the emulator.
#define NX_MINOR_VERSION        0

//! The patch version of this software.
//!
//! This should increase if bug-fixes have been applied but the functionality of the software hasn't changed (unless
//! to fix a bug).  Also this is the major version during a dev version (i.e. major and minor versions are both 0).
#define NX_PATCH_VERSION        8

//! The development minor version.
//!
//! This is only used for dev versions.  Acts like a minor version.
#define NX_DEV_MINOR            0

//! The development version patch letter.
//!
//! If quick bug-fixes go into a development version, this letter is set to 'A' or the next letter
#define NX_DEV_PATCH            "A"

//! Used to customise a version for a customer.
//!
//! Used for customers that require a quick build of the latest software.  This should be undefined otherwise.
//#define NX_DEV_TESTER           ""

#define NX_STR2(x) #x
#define NX_STR(x) NX_STR2(x)

#if NX_MAJOR_VERSION == 0
#   if NX_MINOR_VERSION == 0
#       ifdef NX_DEV_TESTER
#           define NX_VERSION "Dev." NX_STR(NX_PATCH_VERSION) "." NX_STR(NX_DEV_MINOR) " (" NX_DEV_TESTER ")"
#       elif defined(NX_DEV_PATCH)
#           define NX_VERSION "Dev." NX_STR(NX_PATCH_VERSION) "." NX_STR(NX_DEV_MINOR) NX_DEV_PATCH
#       else
#           define NX_VERSION "Dev." NX_STR(NX_PATCH_VERSION) "." NX_STR(NX_DEV_MINOR)
#       endif
#   elif NX_MINOR_VERSION == 9
#       define NX_VERSION "Beta." NX_STR(NX_PATCH_VERSION)
#   else
#       define NX_VERSION "Alpha." NX_STR(NX_PATCH_VERSION)
#   endif
#else
#   define NX_VERSION NX_STR(NX_MAJOR_VERSION) "." NX_STR(NX_MINOR_VERSION) "." NX_STR(NX_PATCH_VERSION)
#endif

//----------------------------------------------------------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------------------------------------------------------

#if NX_DEBUG_CONSOLE
#   define NX_LOG(...) printf(__VA_ARGS__)
#else
#   define NX_LOG(...)
#endif

//----------------------------------------------------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------------------------------------------------

//! The width of the actual pixel area of the screen.
const int kScreenWidth = 256;
//! The height of the actual pixel area of the screen.
const int kScreenHeight = 192;

//! The width of the TV.
//!
//! Image comprises of 64 lines of border, 192 lines of pixel data, and 56 lines of border.
//! Each line comprises of 48 pixels of border, 256 pixels of pixel data, followed by another 48 pixels of border.
//! Timing of a line is 24T for each border, 128T for the pixel data and 48T for the horizontal retrace (224 t-states).
const int kTvWidth = 352;

//! The width and height of the TV.
//!
//! Image comprises of 64 lines of border, 192 lines of pixel data, and 56 lines of border.
//! Each line comprises of 48 pixels of border, 256 pixels of pixel data, followed by another 48 pixels of border.
//! Timing of a line is 24T for each border, 128T for the pixel data and 48T for the horizontal retrace (224 t-states).
const int kTvHeight = 312;

//! The width of the window that displays the emulated image (can be smalled than the TV size).
const int kWindowWidth = 320;
//! The height of the window that displays the emulated image (can be smalled than the TV size).
const int kWindowHeight = 256;

//! The horizontal border size (in pixels).
const int kBorderWidth = (kWindowWidth - kScreenWidth) / 2;

//! The vertical border size (in pixels).
const int kBorderHeight = (kWindowHeight - kScreenHeight) / 2;

//! The simulated pixel to real pixel ratio.
const int kDefaultScale = 4;

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

