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
# Copyright NXP B.V. 2013. All rights reserved
#
############################################################################

##############################################################################
# Target name

TARGET    = TestServer

##############################################################################
# Path definitions

LIBJIP_BASE_DIR = $(abspath ..)
LIBJIP_BUILD    = $(LIBJIP_BASE_DIR)/Build
LIBJIP_INC      = $(LIBJIP_BASE_DIR)/Include
LIBJIP_LIB      = $(LIBJIP_BASE_DIR)/Library

##############################################################################
# Object files

vpath % $(LIBJIP_SRC) $(JIPCLI_SRC)

# JIP Sources
SRCS += TestServer.c

##############################################################################
# Header search paths

INCFLAGS += -I$(LIBJIP_INC)


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

PROJ_CFLAGS += -fPIC -Wall -O2 -D_GNU_SOURCE

PROJ_LDFLAGS += -lpthread

# CLI Version
PROJ_CFLAGS += -DVERSION="\"$(shell if [ -f version.txt ]; then cat version.txt; else svnversion ../Source; fi)\""

##############################################################################
# Library objects

OBJS  += $(SRCS:.c=.o)

DEPS = $(OBJS:.o=.d)

#########################################################################
# Dependency rules

.PHONY: all clean libJIP

all: libJIP $(TARGET)

libJIP: $(LIBJIP_BUILD)
	$(info Making libJIP)
	$(MAKE) -C $(LIBJIP_BUILD);


%.o: %.c
	$(info Compiling $(<F) ...)
	$(CC) -c -o $*.o $(CFLAGS) $(INCFLAGS) $(PROJ_CFLAGS) $< -MD -MF $*.d -MP
	@echo

$(TARGET): $(OBJS)
	$(info Linking $@ ...)
	$(CC) -o $@ $< $(LDFLAGS) $(PROJ_LDFLAGS) -L$(LIBJIP_LIB) -lJIP

clean:
	rm -f *.o *.d
	rm -f $(OBJS)
	rm -f $(TARGET)

#########################################################################
