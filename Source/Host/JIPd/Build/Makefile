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

FEATURES ?= JIPD_FEATURE_ZEROCONF

TARGET_JIPD   = JIPd

##############################################################################
# Default target is the JN514x family since we're building a library

#JENNIC_CHIP_FAMILY  ?= JN514x
#BUILD               ?= ROM

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

JIPD_BASE_DIR = $(abspath ..)
JIPD_INC      = $(JIPD_BASE_DIR)/Include
JIPD_SRC      = $(JIPD_BASE_DIR)/Source

##############################################################################
# Library object files

vpath % $(JIPD_SRC)

# JIP Sources
JIPD_SRCS += JIPd.c
JIPD_SRCS += IPv4_UDP.c
JIPD_SRCS += IPv4_TCP.c

ifeq ($(findstring JIPD_FEATURE_ZEROCONF,$(FEATURES)),JIPD_FEATURE_ZEROCONF)
JIPD_SRCS += Zeroconf.c
else
JIPD_SRCS += Common.c
endif

JIPD_OBJS += $(JIPD_SRCS:.c=.o)

##############################################################################
# Library header search paths

INCFLAGS += -I$(JIPD_INC)
INCFLAGS += -I$(JIPD_SRC)


# This won't be right for everyone!
INCFLAGS += -I../../libJIP/Include

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

PROJ_LDFLAGS += -lpthread -ldaemon

ifeq ($(findstring JIPD_FEATURE_ZEROCONF,$(FEATURES)),JIPD_FEATURE_ZEROCONF)
JIPD_SRCS += Zeroconf.c
PROJ_CFLAGS += -DUSE_ZEROCONF
PROJ_LDFLAGS += -lavahi-client -lavahi-common -ldbus-1
endif

##############################################################################
# Library objects

#########################################################################
# Archiver

ARFLAGS = rcs

#########################################################################
# Dependency rules

.PHONY: all clean ../Source/version.h 

all: $(TARGET_JIPD)

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

$(TARGET_JIPD): $(JIPD_OBJS)
	$(info Linking $@ ...)
	$(CC) -o $@ $^ $(LDFLAGS) $(PROJ_LDFLAGS)

clean:
	rm -f *.o
	rm -f *.d
	rm -f $(TARGET_JIPD)
	rm -f $(JIPD_OBJS)

#########################################################################
