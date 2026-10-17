# PS2 build, two flavors:
#  - demoreel.elf / demoreel_cd.elf: the real SCE SDK toolchain and
#    libraries at /usr/local/sce end to end (i386 execution works on
#    this machine; see librwps2/Makefile for the pattern).
#    Build librw for it first (with the SCE gcc on PATH so premake's
#    ee-g++ resolves to 3.2, matching the C++ ABI of these objects):
#      premake5 gmake
#      PATH=/usr/local/sce/ee/gcc/bin:$PATH \
#        make -C build librw config=debug_ps2 PS2SDK=/usr/local/sce
#  - demoreel_cd_freesce.elf: complete freesce build, no SCE anywhere;
#    see the FREESCE_ section below.

CC := /usr/local/sce/ee/gcc/bin/ee-gcc
CXX := /usr/local/sce/ee/gcc/bin/ee-g++

TARGET=demoreel

LIBRW := ../..

SRC := main.cpp proctex.cpp scene_tunnel.cpp scene_particles.cpp scene_knot.cpp
SKELSRC := skeleton.cpp camera.cpp ps2.cpp

SCELIBDIR := /usr/local/sce/ee/lib
GCCLIB := /usr/local/sce/ee/gcc/lib/gcc-lib/ee/3.2-ee-040921

CFLAGS := -Os -DRW_PS2 -fno-common -fno-exceptions

LIBS =	$(LIBRW)/lib/ps2/Debug/librw.a	\
	$(SCELIBDIR)/libgraph.a	\
	$(SCELIBDIR)/libdma.a	\
	$(SCELIBDIR)/libpc.a	\
	$(SCELIBDIR)/libpad.a	\
	$(SCELIBDIR)/libcdvd.a	\
	$(SCELIBDIR)/libkernl.a

CRT_BEGIN := crt0.o $(GCCLIB)/crti.o $(GCCLIB)/crtbegin.o
CRT_END := $(GCCLIB)/crtend.o $(GCCLIB)/crtn.o

INC :=	-I.	\
	-I$(LIBRW)	\
	-I$(LIBRW)/skeleton	\
	-I/usr/local/sce/ee/include	\
	-I/usr/local/sce/common/include

# Two ELFs: demoreel.elf runs off the host filesystem (host: for pcsx2,
# host0: for the SCE TOOL) for quick iteration. demoreel_cd.elf reboots
# the IOP from cdrom0:\MODULES\IOPRP300.IMG like a real disc boot (see
# SKEL_CDROM in skeleton/ps2.cpp) - only CDROM builds have been known to
# run correctly, so that's the one to test on hardware/dsedb.
all: $(TARGET).elf $(TARGET)_cd.elf

OBJDIR := obj/ps2/host
OBJ := $(addprefix $(OBJDIR)/,$(SRC:.cpp=.o) $(SKELSRC:.cpp=.o))
DEP := $(OBJ:.o=.d)

OBJDIR_CD := obj/ps2/cd
OBJ_CD := $(addprefix $(OBJDIR_CD)/,$(SRC:.cpp=.o) $(SKELSRC:.cpp=.o))
DEP_CD := $(OBJ_CD:.o=.d)

$(TARGET).elf: $(OBJ) crt0.o
	$(CXX) -o $@ $(CRT_BEGIN) $(OBJ) $(LIBS) $(CRT_END) -T $(SCELIBDIR)/app.cmd -L$(SCELIBDIR) -lm -nostartfiles -Wl,--gc-sections

$(TARGET)_cd.elf: $(OBJ_CD) crt0.o
	$(CXX) -o $@ $(CRT_BEGIN) $(OBJ_CD) $(LIBS) $(CRT_END) -T $(SCELIBDIR)/app.cmd -L$(SCELIBDIR) -lm -nostartfiles -Wl,--gc-sections

# CD build against freesce -- a COMPLETE freesce build: freesce's own
# ee-gcc 2.9 toolchain, freesce headers only, freesce's six libraries,
# crt0/crti/crtbegin/crtend/crtn and app.cmd.  libc/libm are the
# toolchain's newlib, libgcc its own; there is no libstdc++ (2.9's
# C++ runtime bits live in libgcc).  app.cmd deliberately has no
# GROUP(-lc ...) directive, so the runtime libs go inside an explicit
# link group.  Nothing comes from /usr/local/sce.
#
# Two roots, because they are two different axes (the xtc convention):
# FREESCE is the SDK vintage -- headers, libraries, crt, app.cmd -- a
# tree root with ee/include and ee/lib under it, which is how an
# install and a git worktree are both laid out.  FREESCE_GCC is the
# compiler root; the compiler is not SDK-versioned, so it deliberately
# does not derive from FREESCE (a source worktree has no compiler).
#
#	make demoreel_cd_freesce.elf
#	make demoreel_cd_freesce.elf FREESCE=~/src/freesce_24  # from a worktree
#
# librw.a for this flavor -- the SCE-vs-freesce choice is made at
# premake time, because the 2.9 and 3.2 C++ ABIs don't link, so one
# generated tree builds one flavor:
#   premake5 gmake --freesce; make -C build librw config=debug_ps2
# (a librw.a generated without --freesce is 3.2-ABI and won't link
# here; regenerate and rebuild)
FREESCE     ?= /usr/local/freesce
FREESCE_GCC ?= /usr/local/freesce/ee/gcc
FREESCE_LIB := $(FREESCE)/ee/lib
# ctorgen: gcc 2.9 on the EE emits file-scope ctors as _GLOBAL_$I
# functions but registers none of them; this collects them into .ctors
# entries.  Without it librw's global ctors (the PluginList statics)
# silently never run.  ee/bin/ctorgen in an install, tools/ctorgen.sh
# in a worktree.
FREESCE_CTORGEN ?= $(firstword $(wildcard $(FREESCE)/ee/bin/ctorgen $(FREESCE)/tools/ctorgen.sh))
FREESCE_CXX := $(FREESCE_GCC)/bin/ee-g++
FREESCE_NM := $(FREESCE_GCC)/bin/ee-nm
FREESCE_CFLAGS := -Os -mno-abicalls -G0 -DRW_PS2 -fno-common -fno-exceptions
FREESCE_INC := -I. -I$(LIBRW) -I$(LIBRW)/skeleton -I$(FREESCE)/ee/include
FREESCE_LIBRW := $(LIBRW)/lib/ps2/Debug/librw.a
OBJDIR_CDF := obj/ps2/cdf
OBJ_CDF := $(addprefix $(OBJDIR_CDF)/,$(SRC:.cpp=.o) $(SKELSRC:.cpp=.o))
FREESCE_LIBS = $(FREESCE_LIB)/libgraph.a $(FREESCE_LIB)/libdma.a $(FREESCE_LIB)/libpc.a \
	$(FREESCE_LIB)/libpad.a $(FREESCE_LIB)/libcdvd.a $(FREESCE_LIB)/libkernl.a

$(OBJDIR_CDF)/ctorreg.s: $(OBJ_CDF) $(FREESCE_LIBRW)
	NM=$(FREESCE_NM) $(FREESCE_CTORGEN) $(OBJ_CDF) $(FREESCE_LIBRW) > $@

$(OBJDIR_CDF)/ctorreg.o: $(OBJDIR_CDF)/ctorreg.s
	$(FREESCE_CXX) -c $< -o $@

# -nostdlib, not -nostartfiles: everything is listed explicitly, and
# the g++ driver must not append its usual -lstdc++ (which freesce's
# gcc 2.9 doesn't have) nor a trailing -lgcc, whose __main.o would
# collide with freesce's own __main in crtbegin.o.
$(TARGET)_cd_freesce.elf: $(OBJ_CDF) $(OBJDIR_CDF)/ctorreg.o $(FREESCE_LIBRW)
	$(FREESCE_CXX) -o $@ -nostdlib \
	    $(FREESCE_LIB)/crt0.o $(FREESCE_LIB)/crti.o $(FREESCE_LIB)/crtbegin.o \
	    $(OBJDIR_CDF)/ctorreg.o $(OBJ_CDF) $(FREESCE_LIBRW) \
	    -Wl,--start-group $(FREESCE_LIBS) -lc -lm -lgcc -Wl,--end-group \
	    $(FREESCE_LIB)/crtend.o $(FREESCE_LIB)/crtn.o \
	    -T $(FREESCE_LIB)/app.cmd -L$(FREESCE_LIB)

run: $(TARGET).elf
	dsedb -r run $(TARGET).elf

crt0.o: $(SCELIBDIR)/crt0.s
	$(CC) -c -xassembler-with-cpp -o $@ $(SCELIBDIR)/crt0.s

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) $(INC) -MMD -c $< -o $@

$(OBJDIR)/%.o: $(LIBRW)/skeleton/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) $(INC) -MMD -c $< -o $@

$(OBJDIR_CD)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) -DSKEL_CDROM $(INC) -MMD -c $< -o $@

$(OBJDIR_CD)/%.o: $(LIBRW)/skeleton/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) -DSKEL_CDROM $(INC) -MMD -c $< -o $@

# freesce-flavor CD objects: freesce compiler, freesce headers only
# (they carry the vintage's IOP_IMAGE_FILE -- IOPRP242.IMG -- in
# sifdev.h, exactly like SCE's headers carry theirs); libc headers are
# the toolchain's newlib ones.  No -MMD: the 2.9 driver has no -MF and
# drops .d files in the cwd, so this flavor goes without dep tracking.
$(OBJDIR_CDF)/%.o: %.cpp
	@mkdir -p $(@D)
	$(FREESCE_CXX) $(FREESCE_CFLAGS) -DSKEL_CDROM $(FREESCE_INC) -c $< -o $@

$(OBJDIR_CDF)/%.o: $(LIBRW)/skeleton/%.cpp
	@mkdir -p $(@D)
	$(FREESCE_CXX) $(FREESCE_CFLAGS) -DSKEL_CDROM $(FREESCE_INC) -c $< -o $@

clean:
	rm -rf obj/ps2 crt0.o $(TARGET).elf $(TARGET)_cd.elf $(TARGET)_cd_freesce.elf

-include $(DEP) $(DEP_CD)
