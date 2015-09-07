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


FEATURES ?= ZIGBEE_JIP_FEATURE_ZEROCONF

SOURCE := zigbee-jip-daemon.c TunDevice.c Zeroconf.c CommissioningServer.c JIP_Common.c JIP_BorderRouter.c JIP_ControlBridge.c JIP_ColourLamp.c JIP_Thermostat.c 

ifeq ($(findstring ZIGBEE_JIP_FEATURE_ZEROCONF,$(FEATURES)),ZIGBEE_JIP_FEATURE_ZEROCONF)
SOURCE += Zeroconf.c
PROJ_CFLAGS += -DUSE_ZEROCONF
PROJ_LDFLAGS += -lavahi-client -lavahi-common -ldbus-1
endif

CFLAGS += -O2 -Wall -g

OBJ := $(SOURCE:.c=.o)

PROJ_CFLAGS += -I../Source/
PROJ_CFLAGS += -DVERSION="\"$(shell if [ -f ../../Build/version.txt ]; then cat ../../Build/version.txt; else svnversion ../Source; fi)\""

PROJ_CFLAGS += -I../../ZCB/Include/

PROJ_CFLAGS += -I../../../../JIP/libJIP/Include/
PROJ_LDFLAGS += -L../../../../JIP/libJIP/Library -lJIP

PROJ_LDFLAGS += -ldaemon -lpthread -lJIP -lsqlite3 -lxml2

vpath %.c ../Source

TARGET = zigbee-jip-daemon

all: $(TARGET)

.PHONY : ../../ZCB/Library/libZCB.a

../../ZCB/Library/libZCB.a:
	$(MAKE) -C ../../ZCB/Build


$(TARGET): $(OBJ) ../../ZCB/Library/libZCB.a
	$(CC)  $^ $(PROJ_LDFLAGS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC)  -I. $(CFLAGS) $(PROJ_CFLAGS) -c $<

install:
	mkdir -p $(DESTDIR)/sbin/
	cp $(TARGET) $(DESTDIR)/sbin/

clean:
	$(MAKE) -C ../../ZCB/Build clean
	rm -f *.o $(TARGET)

