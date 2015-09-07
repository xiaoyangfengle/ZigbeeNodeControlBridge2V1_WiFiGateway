############################################################################
#
# This software is owned by NXP B.V. and/or its supplier and is protected
# under applicable copyright laws. All rights are reserved. We grant You,
# and any third parties, a license to use this software solely and
# exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139]. 
# You, and any third parties must reproduce the copyright and warranty notice
# and any other legend of ownership on each copy or partial copy of the 
# software.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Copyright NXP B.V. 2012. All rights reserved
#
############################################################################

##############################################################################
# Library target name

TARGET    = libJIP

TARGET_CLI = JIP

FEATURES ?= LIBJIP_FEATURE_PERSIST

LIBJIP_VERSION_MAJOR = 2
LIBJIP_VERSION_MINOR = 3

INSTALL ?= install

##############################################################################
# Path definitions

LIBJIP_BASE_DIR    = $(abspath ..)
LIBJIP_INC         = $(LIBJIP_BASE_DIR)/Include
LIBJIP_LIB         = $(LIBJIP_BASE_DIR)/Library
LIBJIP_SRC_COMMMON = $(LIBJIP_BASE_DIR)/Source/Common 
LIBJIP_SRC_CLIENT  = $(LIBJIP_BASE_DIR)/Source/Client 
LIBJIP_SRC_SERVER  = $(LIBJIP_BASE_DIR)/Source/Server

JIPCLI_SRC         = $(LIBJIP_BASE_DIR)/CLI


##############################################################################
# Library object files

vpath % $(LIBJIP_SRC_COMMMON) $(LIBJIP_SRC_CLIENT) $(LIBJIP_SRC_SERVER) $(JIPCLI_SRC)

# JIP Sources
LIBJIPSRCS += Utils.c
LIBJIPSRCS += libJIP.c
LIBJIPSRCS += libJIPclient.c
LIBJIPSRCS += libJIPserver.c
LIBJIPSRCS += Network.c
LIBJIPSRCS += DiscoverNetwork.c
LIBJIPSRCS += Node.c
LIBJIPSRCS += Tables.c
LIBJIPSRCS += Cache.c
LIBJIPSRCS += Groups.c

ifeq ($(findstring LIBJIP_FEATURE_PERSIST,$(FEATURES)),LIBJIP_FEATURE_PERSIST)
LIBJIPSRCS += Persist.c
INCFLAGS += $(shell xml2-config --cflags)
PROJ_LDFLAGS += $(shell xml2-config --libs) -lz
endif

LIBJIPOBJS  += $(LIBJIPSRCS:.c=.o)

LIBJIP_NAME = $(TARGET).so.$(LIBJIP_VERSION_MAJOR).$(LIBJIP_VERSION_MINOR)
LIBJIP_SONAME = $(TARGET).so.$(LIBJIP_VERSION_MAJOR)


JIPCLISRCS += JIP_CLI.c
JIPCLIOBJS  += $(JIPCLISRCS:.c=.o)


##############################################################################
# Library header search paths

INCFLAGS += -I$(LIBJIP_INC)
INCFLAGS += -I$(LIBJIP_SRC_COMMMON)
INCFLAGS += -I$(LIBJIP_SRC_CLIENT)
INCFLAGS += -I$(LIBJIP_SRC_SERVER)


##############################################################################
# Debugging 
# Define TRACE to use with DBG module
TRACE ?=0
DEBUG = 0

ifeq ($(DEBUG), 1)
CFLAGS  := $(subst -Os,,$(CFLAGS))
CFLAGS  += -g -O0 -DGDB -w
$(info Building debug version ...)
endif


###############################################################################

PROJ_CFLAGS += -fPIC -Wall -O2 -D_GNU_SOURCE -g

PROJ_CFLAGS += ${addprefix -D,$(FEATURES)}

PROJ_CFLAGS += -DLIBJIP_VERSION="\"$(shell if [ -f version.txt ]; then cat version.txt; else svnversion ../Source; fi)\""
PROJ_CFLAGS += -DLIBJIP_VERSION_MAJOR="\"$(LIBJIP_VERSION_MAJOR)\"" -DLIBJIP_VERSION_MINOR="\"$(LIBJIP_VERSION_MINOR)\""

PROJ_LDFLAGS += -lpthread -ltecla

# CLI Version
PROJ_CFLAGS += -DVERSION="\"$(shell if [ -f version.txt ]; then cat version.txt; else svnversion ../Source; fi)\""

##############################################################################
# Library objects

LIBOBJS += $(LIBJIPOBJS) 

LIBDEPS = $(LIBOBJS:.o=.d)

#########################################################################
# Archiver

ARFLAGS = rcs

#########################################################################
# Dependency rules

.PHONY: all clean ../Source/version.h 

all: $(LIBJIP_LIB)/$(TARGET).a $(LIBJIP_LIB)/$(LIBJIP_NAME) $(TARGET_CLI)

-include $(LIBDEPS)
%.d:
	rm -f $*.o

%.o: %.S
	$(info Assembling $< ...)
	$(CC) -c -o $*.o $(CFLAGS) $(INCFLAGS) $< -MD -MF $*.d -MP
	@echo

%.o: %.c
	$(info Compiling $(<F) ...)
	$(CC) -c -o $*.o $(CFLAGS) $(INCFLAGS) $(PROJ_CFLAGS) $< -MD -MF $*.d -MP
	@echo

$(LIBJIP_LIB)/$(TARGET).a: $(LIBJIPOBJS)
	$(info Archiving $@ ...)
	rm -f $@
	$(AR) $(ARFLAGS) $@ $(LIBJIPOBJS)
	@echo

$(LIBJIP_LIB)/$(LIBJIP_NAME): $(LIBJIPOBJS)
	$(info Building shared library $@ ...)
	$(CC) -shared -Wl,-soname,$(LIBJIP_SONAME) $(LDFLAGS) $(PROJ_LDFLAGS) -o $@ $^
	ln -sf $(LIBJIP_NAME) $(LIBJIP_LIB)/$(TARGET).so.$(LIBJIP_VERSION_MAJOR)
	ln -sf $(TARGET).so.$(LIBJIP_VERSION_MAJOR) $(LIBJIP_LIB)/$(TARGET).so 
	@echo

$(TARGET_CLI): $(JIPCLIOBJS) $(LIBJIP_LIB)/$(LIBJIP_NAME)
	$(info Linking $@ ...)
	$(CC) -o $@ $< -L$(LIBJIP_LIB) -lJIP $(LDFLAGS) $(PROJ_LDFLAGS)

clean:
	rm -f *.o
	rm -f *.a *.so*
	rm -f *.d
	rm -f *.pp
	rm -f $(LIBJIPOBJS)
	rm -f $(TARGET_CLI) $(JIPCLIOBJS)

install:
	$(INSTALL) $(LIBJIP_INC)/JIP.h $(DESTDIR)/usr/include
	$(INSTALL) $(LIBJIP_LIB)/* $(DESTDIR)/usr/lib/
	$(INSTALL) $(TARGET_CLI) $(DESTDIR)/usr/bin/

uninstall:
	rm $(DESTDIR)/usr/include/JIP.h
	rm $(DESTDIR)/usr/lib/libJIP*
	rm $(DESTDIR)/usr/bin/$(TARGET_CLI)

#########################################################################
