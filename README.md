# StreamLib
(C) 2019 Christopher Bazley

Release 11 (28 Jul 2022)

Introduction
------------
  The purpose of this C library is to provide a uniform interface to read
from or write to streams of different types. Several implementations are
provided, including for compressed and uncompressed input/output. The
interface is similar to that provided by the standard C library in order to
ease porting of existing code.

  Support for reading or writing data compressed using the same algorithm
used by old 4th Dimension and Fednet games such as 'Chocks Away', 'Stunt
Racer 2000' and 'Star Fighter 3000' relies on another C library, GKeyLib, by
the same author.

Fortified memory allocation
---------------------------
  I use Simon's P. Bullen's fortified memory allocation shell 'Fortify' to
find memory leaks in my applications, detect corruption of the heap
(e.g. caused by writing beyond the end of a heap block), and do stress
testing (by causing some memory allocations to fail). Fortify is available
separately from this web site:
http://web.archive.org/web/20020615230941/www.geocities.com/SiliconValley/Horizon/8596/fortify.html

  The debugging version of StreamLib must be linked with 'Fortify', for
example by adding 'C:o.Fortify' to the list of object files specified to the
linker. Otherwise, you will get build-time errors like this:
```
ARM Linker: (Error) Undefined symbol(s).
ARM Linker:     Fortify_malloc, referred to from C:debug.StreamLib(ReaderGKey).
ARM Linker:     Fortify_free, referred to from C:debug.StreamLib(ReaderGKey).
```
Rebuilding the library
----------------------
  You should ensure that the standard C library, GKeyLib, CBUtilLib and
CBDebugLib (all three by the same author as StreamLib) are on your header
include path (C$Path if using the supplied make files on RISC OS), otherwise
the compiler won't be able to find the required header files. The dependency
on CBDebugLib isn't very strong: it can be eliminated by modifying the make
file so that the macro USE_CBDEBUG is no longer predefined.

  Three make files are supplied:

- 'Makefile' is intended for use with GNU Make and the GNU C Compiler on Linux.
- 'NMakefile' is intended for use with Acorn Make Utility (AMU) and the
   Norcroft C compiler supplied with the Acorn C/C++ Development Suite.
- 'GMakefile' is intended for use with GNU Make and the GNU C Compiler on RISC OS.

These make files share some variable definitions (lists of objects to be
built) by including a common make file.

  The APCS variant specified for the Norcroft compiler is 32 bit for
compatibility with ARMv5 and fpe2 for compatibility with older versions of
the floating point emulator. Generation of unaligned data loads/stores is
disabled for compatibility with ARM v6.

  The suffix rules generate output files with different suffixes (or in
different subdirectories, if using the supplied make files on RISC OS),
depending on the compiler options used to compile the source code:

o: Assertions and debugging output are disabled. The code is optimised for
   execution speed.

oz: Assertions and debugging output are disabled. The code is suitable for
    inclusion in a relocatable module (multiple instantiation of static
    data and stack limit checking disabled). When the Norcroft compiler is
    used, the compiler optimises for smaller code size. (The equivalent GCC
    option seems to be broken.)

debug: Assertions and debugging output are enabled. The code includes
       symbolic debugging data (e.g. for use with DDT). The macro FORTIFY
       is pre-defined to enable Simon P. Bullen's fortified shell for memory
       allocations.

d: 'GMakefile' passes '-MMD' when invoking gcc so that dynamic dependencies
   are generated from the #include commands in each source file and output
   to a temporary file in the directory named 'd'. GNU Make cannot
   understand rules that contain RISC OS paths such as /C:Macros.h as
   prerequisites, so 'sed', a stream editor, is used to strip those rules
   when copying the temporary file to the final dependencies file.

  The above suffixes must be specified in various system variables which
control filename suffix translation on RISC OS, including at least
UnixEnv$ar$sfix, UnixEnv$gcc$sfix and UnixEnv$make$sfix.
Unfortunately GNU Make doesn't apply suffix rules to make object files in
subdirectories referenced by path even if the directory name is in
UnixEnv$make$sfix, which is why 'GMakefile' uses the built-in function
addsuffix instead of addprefix to construct lists of the objects to be
built (e.g. foo.o instead of o.foo).

  Before compiling the library for RISC OS, move the C source and header
files with .c and .h suffixes into subdirectories named 'c' and 'h' and
remove those suffixes from their names. You probably also need to create
'o', 'oz', 'd' and 'debug' subdirectories for compiler output.

Licence and disclaimer
----------------------
  This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

  This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
for more details.

  You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Credits
-------
  This library was derived from CBLibrary. Both libraries are free software,
available under the same license.

History
-------
Release 1 (04 Nov 2018)
- Extracted the relevant components from CBLib to make a standalone library.

Release 2 (07 Sep 2019)
- Fixed a broken backward-seek when reading from a compressed file, which
  previously only appeared to work for small offsets and files (which was
  sufficient to pass my tests).
- reader_feof and reader_ferror are now actual functions instead of macros.
- Reads that would cause the file position indicator to overflow now set the
  error indicator instead.
- Added an abstract writer interface to complement the existing reader
  interface.
- Added implementations of the reader interface for reading from memory, a
  simulated null device (empty file) or memory allocated by Acorn's Flex
  library.
- Added implementations of the writer interface for writing to any of the
  above, a malloc block, a compressed file, or a plain file.

Release 3 (02 Nov 2019)
- Added a new writer type to estimate the size of compressed files without
  discarding output by means of a null file writer.

Release 4 (09 Nov 2019)
- Moved non-core functionality such as seeking within a stream and accessing
  byte/halfwords/words into separate compilation units to reduce the size of
  programs linked with this library.
- Added fast paths for common cases in fread and fwrite (one member was
  read/written or member size is one byte).

Release 5 (11 Nov 2019)
- Modified the Flex memory reader to allow reading from a null anchor (which
  is treated as a stream of length 0).

Release 6 (12 Nov 2019)
- The uncompressed file size is now read when a compressed stream is first
  read from instead of when initializing the reader object.
- The minimum uncompressed file size is now passed as type long int instead
  of int32_t when initializing a compressed file writer.
- The minimum uncompressed file size is now written when a compressed stream
  is first written to instead of when initializing the writer object.

Release 7 (16 Feb 2020)
- Deleted a non-existent return value's description.
- Changed the output of writer_gkc_init (predicted compressed size) from
  type int32_t to long int.

Release 8 (30 Sep 2020)
- Added signed 16-bit and unsigned 32-bit read and write functions.
- Less verbose debugging output by default.
- Added support for padding the end of the output to reach a specified
  minimum size when estimating the size of compressed data.

Release 9 (30 Nov 2020)
- Initialize structs using compound literal assignment to guard against
  accidentally leaving some members uninitialized.
- Improved debug output on failure to seek within a flex block.
- Fixed a bug in the compressed file writer: instead of invoking the
  compressor until the output buffer does not overflow, it now stops when
  all of the buffered input data (so far) has been compressed. The previous
  behaviour could cause an infinite loop because the compressor ignores
  further input after it has returned GKeyStatus_Finished in response to
  receiving an empty input buffer.

Release 10 (06 Dec 2020)
- Fixed another bug in the compressed file writer: calling empty_in twice
  before empty_out on writer destruction wasn't sufficient to avoid
  truncated output in the case where the first empty_in had consumed all the
  input data and exactly filled the output buffer. The second empty_in
  returned prematurely before compression finished due to lack of input.
  Solved by using a different termination condition when flushing.

Release 11 (28 Jul 2022)
- Assert that the minimum size specified when creating a compressed file
  writer, and the uncompressed size actually written to the file header, are
  not negative.
- Removed redundant use of 'extern' and 'const' keywords.

Contact details
---------------
Christopher Bazley

Email: mailto:cs99cjb@gmail.com

WWW:   http://starfighter.acornarcade.com/mysite/
