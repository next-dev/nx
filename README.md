# NX - ZX Spectrum Next Emulator

This project is my attempt to emulate the Next for development purposes.  This project will happen in several phases.

Phase 1 is complete.  Phase 2 shortly behind it (floating bus not implemented yet).  Some of phase 3 has been implemented
too (tape browser, .tap files, kempston joysticks).

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

# Global Controls

These controls are available regardless which mode you are in.

| Key              | Description                                           |
|------------------|-------------------------------------------------------|
| Ctrl+1           | Select small window size.                             |
| Ctrl+2           | Select large window size (default).                   |
| Ctrl+3           | Select medium window size.                            |

Please note, because currently the debugger/editor UIs use twice the resolution of the emulated display, and
because medium size uses 3x3 PC pixels for every 1 emulated pixel, they do not look great.  You should only use medium
mode for just using the emulator if you can't put up with the odd display.  I will eventually get round to fixing this but
I have other priorities currently.  But for those people who think small is too small and large is too large, this will be
a temporary solution.

# Emulator Controls

These controls are only valid when no other window is showing.

| Key              | Description                                           |
|------------------|-------------------------------------------------------|
| Right Shift      | Symbol shift                                          |
| Ctrl+A           | Open editor/assembler                                 |
| Ctrl+D           | Open disassembler                                     |
| Ctrl+O           | Open file                                             |
| Ctrl+K           | Toggle Kempston joystick                              |
| Ctrl+R           | Restart the machine                                   |
| Ctrl+T           | Toggle tape browser                                   |
| Ctrl+Z           | Zoom mode (maximum clock speed)                       |
| Ctrl+Space       | Start/Stop tape                                       |
| Ctrl+Tab         | Switch machines                                       |
| F5               | Pause the machine and enter debugger mode             |
| `                | Enter debugger mode                                   |

# Debugger Controls

| Key              | Description                                                      |
|------------------|------------------------------------------------------------------|
| `                | Leave debugger mode.                                             |
| Up/Down          | Scroll through current window.                                   |
| PgUp/PgDn        | Page through current window.                                     |
| Tab              | Cycle through current window.                                    |
| F3               | Jump to next search element.                                     |
| Shift-F3         | Jump to previous search element.                                 |
| F5               | Toggle pause.  Pausing while running will bring up the debugger. |
| Ctrl+F5          | Run to.  Will stop the debugger at that point if running.        |
| F6               | Step over.  Will pause when running.                             |
| F7               | Step in.  Will pause when running.                               |
| F8               | Step out.  Will pause when running.                              |
| F9               | Toggle breakpoint.                                               |
| Alt+1            | Select disassembly view.                                         |
| Alt+2            | Select memory view.                                              |
| Alt+3            | Select command view.                                             |

When pausing from a running state, if interrupts are enabled,
the debugger will always stop inside the interrupt handler since emulator keys are polled after a frame interrupt
is triggered.  Later, breakpoints will be implemented to allow more control about where you stop.

# Editor/Assembler Controls

While in the editor these controls are available:

| Key              | Description                                                            |
|------------------|------------------------------------------------------------------------|
| ESC              | Exit editor.                                                           |
| Ctrl+O           | Open a file.                                                           |
| Ctrl+S           | Save a file.  Will ask for filename if file has not been saved before. |
| Ctrl+Shift+S     | Save as.  Will ask for filename every time.                            |
| Ctrl+Tab         | Switch buffers.                                                        |
| Ctrl+B           | Build current file.                                                    |
| Ctrl+R           | Run current file.  Requires an 'OPT START' option in the source.       |
| F4               | Jump to next error.                                                    |
| Shift+F4         | Jump to previous error.                                                |

Some of the usual controls via cursor keys, page up/down, home & end work.  Currently the
assembler does not exist.

# Command line parameters

The emulator works on a key/value system for configuration.  The command line syntax for an option is:
```
-<key>[=<value>]
```
The value is optional and implies "true".  So `-<key>` is the same as `-<key>=true`.  The supported keys are:

| Key               | Description                                        |
|-------------------|----------------------------------------------------|
| -kempston         | Set to true for kempston support.  Cursor keys<br/>and tab control the joystick. |


# Building on PC

The build environment is set up for Visual Studio 2017.  It uses premake to generate the projects and solutions.  It
uses 3 batch files:

* gen.bat - this builds the solution and projects and puts them in a generated '_build' folder.
* clean.bat - this deletes all folders generated by the build process.  All generated folders will start with an underscore.
* edit.bat - will open the generated solution.

So the normal operation is to run `gen.bat` then run `edit.bat`.

If you require use of a different version of Visual Studio, you can edit the build batch file.  Look for the parameter
passed to premake.  It should be obvious what you change.

# Building on Mac

There is a Xcode project in the `xcode/nx` folder.  Just open it up and build.  You will find the app in:
```
xcode/nx/DerivedData/nx/Build/Products/<Build Type>/
```

where &lt;Build Type&gt; is the configuration you chose (either Debug or Release).

Currently, the xcode project is broken.  It will be fixed when a release is ready for Windows.

# Legal notices

First with the distribution of the ROM:

_Amstrad have kindly given their permission for the redistribution of their copyrighted material but retain that copyright_

Thanks Amstrad & Cliff Lawson!  I love how you are so cool with the retro scene and Sinclair technology.  This information is from
http://www.worldofspectrum.org/permits/amstrad-roms.txt.

Nx also uses SFML for visuals and PortAudio for audio.
