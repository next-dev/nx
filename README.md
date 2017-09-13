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

# Source code organisation.

My style is to use a single .c file with all the platform specific code in it (thanks Casey from handmadehero.org) and
use single header libraries for features (thanks Sean Barrett).  The header files can be re-used in other projects
if you require.  The planned headers will be:


| Header              | Description                                                                 |
|---------------------|-----------------------------------------------------------------------------|
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
|---------------------|-----------------------------------------------------------------------------|

The way the header libraries work is that you can #include them anywhere but in ONE .c file, you must define NX_IMPL
so that the implementation is pulled into that compilation object.