# Project:   StreamLib
include MakeCommon

# Tools
CC = gcc
LibFile = ar

# Toolflags:
CCCommonFlags = -c -Wall -Wextra -pedantic -std=c99 -MMD -MP -o $@
CCFlags = $(CCCommonFlags) -DNDEBUG -O3
CCDebugFlags = $(CCCommonFlags) -g -DDEBUG_OUTPUT
LibFileFlags = -rcs $@

ReleaseObjects = $(addsuffix .o,$(ObjectList))
DebugObjects = $(addsuffix .debug,$(ObjectList))

# Final targets:
all: lib$(LibName).a lib$(LibName)dbg.a

lib$(LibName).a: $(ReleaseObjects)
	$(LibFile) $(LibFileFlags) $(ReleaseObjects)

lib$(LibName)dbg.a: $(DebugObjects)
	$(LibFile) $(LibFileFlags) $(DebugObjects)

# User-editable dependencies:
# All of these suffixes must also be specified in UnixEnv$*$sfix
.SUFFIXES: .o .c .debug .s
.c.o:
	${CC} $(CCFlags) -MF $*.d $<
.c.debug:
	${CC} $(CCDebugFlags) -MF $*D.d $<

# These files are generated during compilation to track C header #includes.
# It's not an error if they don't exist.
-include $(addsuffix .d,$(ObjectList))
-include $(addsuffix D.d,$(ObjectList))
