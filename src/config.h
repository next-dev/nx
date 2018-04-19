//----------------------------------------------------------------------------------------------------------------------
// Configuration settings
//----------------------------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------------------------

// Show a console on Win32 platform
#define NX_DEBUG_CONSOLE        0
// Debug the editor buffer
#define NX_DEBUG_EDITOR         0

//----------------------------------------------------------------------------------------------------------------------

namespace std {}

using namespace std;

// We usually want to output if the debug console is open
#if NX_DEBUG_CONSOLE
#   include <iostream>
#endif

//----------------------------------------------------------------------------------------------------------------------
// Version information
//----------------------------------------------------------------------------------------------------------------------

#define NX_MAJOR_VERSION        0
#define NX_MINOR_VERSION        0
#define NX_PATCH_VERSION        7
#define NX_DEV_PATCH            3
#define NX_DEV_TESTER           "Richard Dodds"

#define NX_STR2(x) #x
#define NX_STR(x) NX_STR2(x)

#if NX_MAJOR_VERSION == 0
#   if NX_MINOR_VERSION == 0
#       ifdef NX_DEV_TESTER
#           define NX_VERSION "Dev." NX_STR(NX_PATCH_VERSION) "." NX_STR(NX_DEV_PATCH) " (" NX_DEV_TESTER ")"
#       else
#           define NX_VERSION "Dev." NX_STR(NX_PATCH_VERSION) "." NX_STR(NX_DEV_PATCH)
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

// The width and height of the actual pixel area of the screen
const int kScreenWidth = 256;
const int kScreenHeight = 192;

// The width and height of the TV
// Image comprises of 64 lines of border, 192 lines of pixel data, and 56 lines of border.
// Each line comprises of 48 pixels of border, 256 pixels of pixel data, followed by another 48 pixels of border.
// Timing of a line is 24T for each border, 128T for the pixel data and 48T for the horizontal retrace (224 t-states).
const int kTvWidth = 352;
const int kTvHeight = 312;

// The width and height of the window that displays the emulated image (can be smalled than the TV size)
const int kWindowWidth = 320;
const int kWindowHeight = 256;

const int kBorderWidth = (kWindowWidth - kScreenWidth) / 2;
const int kBorderHeight = (kWindowHeight - kScreenHeight) / 2;

static const int kUiWidth = kWindowWidth * 2;
static const int kUiHeight = kWindowHeight * 2;

const int kDefaultScale = 3;

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------


