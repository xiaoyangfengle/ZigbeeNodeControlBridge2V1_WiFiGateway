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

TARGET_JIP_CGI              = JIP.cgi

##############################################################################
# Default target is the JN514x family since we're building a library

#JENNIC_CHIP_FAMILY  ?= JN514x
#BUILD               ?= ROM

BUILD ?= PC

##############################################################################
# Development Kit target hardware

JENNIC_PCB = None 

##############################################################################
# Define the SDK location

SDK_BASE_DIR      =  $(abspath ../../..)

##############################################################################
# Configure for the selected chip or chip family

#export JENNIC_CHIP
#export JENNIC_CHIP_FAMILY
#export JENNIC_PCB
#export SDK_BASE_DIR

#include $(SDK_BASE_DIR)/Chip/Common/Build/config.mk

##############################################################################
# Path definitions

JIP_CGI_BASE_DIR = $(abspath ..)
JIP_CGI_INC      = $(JIP_CGI_BASE_DIR)/Include
JIP_CGI_SRC      = $(JIP_CGI_BASE_DIR)/Source


##############################################################################
# Library object files

vpath % $(JIP_CGI_SRC)

# JIP Sources
JIPCGISRCS += JIP_cgi.c
JIPCGISRCS += Zeroconf.c
JIPCGISRCS += CGI.c
JIPCGIOBJS  += $(JIPCGISRCS:.c=.o)

##############################################################################
# Library header search paths

INCFLAGS += -I$(JIP_CGI_INC)
INCFLAGS += -I$(JIP_CGI_SRC)
INCFLAGS += -I../../libJIP/Include

INCFLAGS += $(shell xml2-config --cflags)
LDFLAGS += $(shell xml2-config --libs)

INCFLAGS += -I/usr/include/json-c/
LDFLAGS += -L/usr/lib/json-c/ -ljson-c

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


PROJ_CFLAGS += -DVERSION="\"$(shell if [ -f version.txt ]; then cat version.txt; else svnversion ../Source; fi)\""

#PROJ_LDFLAGS += -L/usr/lib/ -lJIP -lavahi-client -lavahi-common -ldbus-1 -lxml2 -lz
PROJ_LDFLAGS += -L../../libJIP/Library -lJIP -lavahi-client -lavahi-common -ldbus-1 -lxml2 -lz -lpthread -ljson-c

CGI_LDFLAGS = $(PROJ_LDFLAGS)

TEST_LDFLAGS = $(PROJ_LDFLAGS)


##############################################################################
# Library objects

LIBOBJS += $(JIPCGIOBJS) 

LIBDEPS = $(LIBOBJS:.o=.d)

#########################################################################
# Archiver

ARFLAGS = rcs

#########################################################################
# Dependency rules

.PHONY: all clean ../Source/version.h 

all: $(TARGET_JIP_CGI)

-include $(LIBDEPS)
%.d:
	rm -f $*.o

%.o: %.S
	$(info Assembling $< ...)
	$(CC) -c -o $*.o $(CFLAGS) $(INCFLAGS)$< -MD -MF $*.d -MP
	@echo

%.o: %.c
	$(info Compiling $(<F) ...)
	#cpp $(CFLAGS) $(INCFLAGS) $< -o $*.pp 
	$(CC) -c -o $*.o $(CFLAGS) $(INCFLAGS) $(PROJ_CFLAGS)  $< -MD -MF $*.d -MP
	@echo

$(TARGET_JIP_CGI): $(JIPCGIOBJS)
	$(info Linking $@ ...)
	$(CC) -o $@ $^ $(LDFLAGS) $(CGI_LDFLAGS)

clean:
	rm -f *.o
	rm -f *.d
	rm -f $(TARGET_JIP_CGI)
	rm -f $(JIPCGIOBJS)

#########################################################################
