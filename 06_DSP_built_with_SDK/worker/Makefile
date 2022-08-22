############################################################################
# audio_recorder/worker/Makefile
#
#   Copyright 2018 Sony Semiconductor Solutions Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name of Sony Semiconductor Solutions Corporation nor
#    the names of its contributors may be used to endorse or promote
#    products derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

include $(TOPDIR)/.config
include $(APPDIR)/Make.defs
include $(SDKDIR)/Make.defs

ifeq ($(WINTOOL),y)
LIB_DIR = "${shell cygpath -w lib}"
else
LIB_DIR = "lib"
endif

LDLIBPATH += -L $(LIB_DIR)
LDLIBPATH += -Lext/lib

# This order is important! It should be the right one, however I surround the libs by
# -Wl,--start-group and -Wl,--end-group, which makes the order not imporant anymore
LDLIBS += -lasmpw
LDLIBS += -lopus
LDLIBS += -loggz
LDLIBS += -logg

WORKER_ELF = OPUSENC

CXXSRCS = encoderPoc_ogg.cpp main.cpp

ifeq ($(WINTOOL),y)
  LDRAWELFFLAGS = -T "${shell cygpath -w $(SDKDIR)/tools/asmp-elf.ld}"
else
  LDRAWELFFLAGS = -T $(SDKDIR)/tools/asmp-elf.ld
endif
LDRAWELFFLAGS += -Wl,-u,__vectors
LDRAWELFFLAGS += -Wl,-zmax-page-size=256
LDRAWELFFLAGS += -Wl,--defsym,STACK_SIZE=0x8000
LDRAWELFFLAGS += -Wl,--gc-sections
LDRAWELFFLAGS += -ggdb
LDRAWELFFLAGS += -mthumb
LDRAWELFFLAGS += -mcpu=cortex-m4
LDRAWELFFLAGS += -mfloat-abi=hard
LDRAWELFFLAGS += -mfpu=fpv4-sp-d16
LDRAWELFFLAGS += -specs=nano.specs
LDRAWELFFLAGS += -specs=nosys.specs
LDRAWELFFLAGS += -Wl,-Map=OPUSENC.map

CXXELFFLAGS += -Os
ifeq ($(WINTOOL),y)
CXXELFFLAGS += -I"$(shell cygpath -w userproc$(DELIM)include)"
CXXELFFLAGS += -I"$(shell cygpath -w $(APPDIR))"
CXXELFFLAGS += -I"$(shell cygpath -w $(SDKDIR)$(DELIM)modules$(DELIM)asmp$(DELIM)worker)"
CXXELFFLAGS += -I"$(shell cygpath -w $(SDKDIR)$(DELIM)modules$(DELIM)audio$(DELIM)include)"
CXXELFFLAGS += -I"$(shell cygpath -w $(SDKDIR)$(DELIM)modules$(DELIM)audio)"
else
CXXELFFLAGS += -Iext/include
CXXELFFLAGS += -I$(APPDIR)
CXXELFFLAGS += -I$(SDKDIR)/modules/asmp/worker
CXXELFFLAGS += -I$(SDKDIR)/modules/audio/include
CXXELFFLAGS += -I$(SDKDIR)/modules/audio
endif
 
# https://e2e.ti.com/support/tools/code-composer-studio-group/ccs/f/code-composer-studio-forum/382293/undefined-reference-to-end-in-sbrk-c-in-library-libnosys-a
# above link says The symbol end needs to be defined in the .lds linker script, which is used to set the location of the heap.
# So I added end to the linker script so that is points to the start of the stack which should be the end of the heap.
# _ebss = ALIGN(4);
# end = . + STACK_SIZE ;
# __stack_pointer = . + STACK_SIZE ;

CXXOBJS = $(CXXSRCS:.cpp=$(OBJEXT))

# Build

.PHONY: lib depend clean
all: $(WORKER_ELF)

# Build ASMP worker library

lib:
	$(Q) $(MAKE) -C lib TOPDIR="$(TOPDIR)" SDKDIR="$(SDKDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV)

# Complile

$(CXXOBJS): %$(OBJEXT):%.cpp
	@echo "CXX: $<"
	$(Q) $(CXX) -c $(CXXELFFLAGS) $< -o $@

# Build workers

$(WORKER_ELF): lib $(CXXOBJS)
	@echo "Link with g++: $<"
	$(Q) $(CXX) $(LDRAWELFFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) -Wl,--start-group $(CXXOBJS) $(LDLIBS) -Wl,--end-group
	$(Q) $(STRIP) -d $(WORKER_ELF)

.depend:
	$(Q) $(MAKE) -C lib TOPDIR="$(TOPDIR)" SDKDIR="$(SDKDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV) depend
	@touch $@

depend: .depend

lib_clean:
	@echo "LD: $(Q) $(LD) $(LDRAWELFFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) $(CXXOBJS) $(LDLIBS)"
	$(Q) $(MAKE) -C lib TOPDIR="$(TOPDIR)" SDKDIR="$(SDKDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV) clean

clean: lib_clean
	$(call DELFILE, $(WORKER_ELF))
	$(call CLEAN)

