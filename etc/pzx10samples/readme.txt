Here are few example PZX 1.0 files together with the original data they were
created from. Note that none of the PZX files has been adjusted in any way,
so anyone interested can use PZX tools on the originals and see if they get
identical output. This however means that most of the PZX files are not as
polished as they should or could, so they should be used for testing
purposes only, not as a final PZX version of the corresponding tape file.

The files are grouped in several directories, each testing some of the
aspects of the PZX format or the conversion to PZX format. Here is a brief list:

normal          - games with standard loaders, something to start with.
polarity	- games with polarity-sensitive loaders, to get the polarity right.
alkatraz	- games using Alcatraz protection and turbo-loader.
speedlock	- games using Speedlock protection and turbo-loader.
manyblocks	- TZX files containing lot of blocks.
flowcontrol	- TZX files using flow control blocks like jump and call.
gdb		- TZX files using the GDB block.
csw		- CSW files, both 1.0 and 2.0.

When testing the files, please note that some of them load only in 48k mode,
some only in 128k mode via the Tape Loader, and some only in 128k mode after
using USR 0 and LOAD "". So make sure you try either of these modes in case
the game doesn't seem to load for the first time.
