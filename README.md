# NX - ZX Spectrum Next Emulator

This project is my attempt to emulate the Next for development purposes.  This project will happen in several phases.

Currently no phases are complete.

## Phase 1 - ZX Spectrum 48K uncontended.

The first stage is to emulator the 64K address space, Z80 CPU (including undocumented instructions), and I/O port
$fe (which includes border, keyboard, ear, mic and loudspeaker).  There will be no accurate contention timings at
this stage, or hardware hacks (such as floating bus).  The goal is to get ZX Basic up and running.

File formats supported will be .SNA and .Z80.

## Phase 2 - ZX Spectrum 48K contended.

Now we add the contention and as many hardware hacks support that I can figure out.  The goal of this phase is
to run the Overscan demo.

## Phase 3 - Complete ZX Spectrum 48K support.

Now I will add all the common file format supports such as .TAP and .TZX, plus add a cassette tape browser.  Will also
add multiface and microdrive support.  For joysticks, Kempston, Sinclair and Protek/Cursor.  For mice, Kempston.

## Phase 4 - 128K/+2/+2A support.

Now we add RAM/ROM paging (including contention) and AY-3-8910 support.  Also MF2 & MF3.

## Phase 5 - +3 support.

Disk-drive support.

## Phase 6 - Next support.

Will support all the new video modes, 3 AYs, hardware sprites and scrolling and extra memory.  Also support SD access.

# Keyboard Controls

| Key              | Description                                           |
|------------------|-------------------------------------------------------|
| Ctrl             | Symbol shift                                          |
| F1               | Open file                                             |
| ~                | Enter debugger mode                                   |

# Debugger Controls

| Key              | Description                                           |
|------------------|-------------------------------------------------------|
| ~                | Leave debugger mode                                   |
| Up/Down          | Scroll through memory viewer                          |
| PgUp/PgDn        | Page through memory viewer                            |

# Source code organisation.

My style is to use a single .c file with all the platform specific code in it (thanks Casey from handmadehero.org) and
use single header libraries for features (thanks Sean Barrett).  The header files can be re-used in other projects
if you require.  The planned headers will be:


| Header              | Description                                                                 |
|---------------------|-----------------------------------------------------------------------------|
| nx.h                | This is the emulator.  Houses the UI and Machine structures.                |
| ui.h                | Manages the UI of the emulator.                                             |
| machine.h           | Main glue code for a particular machine.  Manages memory and IO ports.      |
| memory.h            | Memory emulation including ROM and page handling.                           |
| video.h             | Video emulation of ULA modes and Next's sprites and layer 2.                |
| z80.h               | Z80 emulation.  Supports hooking into ED prefixes and contention.           |
| next_z80.h          | Uses z80.h, but adds the other opcodes.                                     |
| tape.h              | Supports tape-based file formats.                                           |
| microdrive.h        | Supports microdrive file formats.                                           |
| keyboard.h          | Emulates the keyboard.                                                      |
| joystick.h          | Emulates the joysticks, through keyboard.h and XInput.                      |

The way the header libraries work is that you can #include them anywhere but in ONE .c file, you must define NX_IMPL
so that the implementation is pulled into that compilation object.

# Building

The build environment is set up for Visual Studio 2017.  It uses premake to generate the projects and solutions.  It
uses 3 batch files:

* build2017.bat - this builds the solution and projects and puts them in a generated '_build' folder.
* clean.bat - this deletes all folders generated by the build process.  All generated folders will start with an underscore.
* edit.bat - will open the generated solution.

So the normal operation is to run `build2017.bat` then run `edit.bat`.

If you require use of a different version of Visual Studio, you can edit the build batch file.  Look for the parameter
passed to premake.  It should be obvious what you change.

# Legal notices

First with the distribution of the ROM:

_Amstrad have kindly given their permission for the redistribution of their copyrighted material but retain that copyright_

Thanks Amstrad & Cliff Lawson!  I love how you are so cool with the retro scene and Sinclair technology.  This information is from
http://www.worldofspectrum.org/permits/amstrad-roms.txt.
