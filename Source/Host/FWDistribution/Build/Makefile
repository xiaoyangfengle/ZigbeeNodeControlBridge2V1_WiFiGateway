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

TARGET_FWDIST_DAEMON := FWDistributiond
TARGET_FWDIST_CLI := FWDistributionControl
TARGET_FWDIST_LUA := FWDistribution.so

TARGETS := $(TARGET_FWDIST_DAEMON) $(TARGET_FWDIST_CLI) $(TARGET_FWDIST_LUA)

FEATURES = 

PROGRAM_VERSION := 0.11

CFLAGS += -O2 -Wall -g



FWDIST_DAEMON_SOURCE := main.c FWMonitor.c Firmwares.c OND.c OND_Network.c Threads.c Status.c
FWDIST_DAEMON_OBJ := $(FWDIST_DAEMON_SOURCE:.c=.o)


FWDIST_CLI_SOURCE := CLI_main.c
FWDIST_CLI_OBJ := $(FWDIST_CLI_SOURCE:.c=.o)

FWDIST_LUA_SOURCE := FWDist_lua.c
FWDIST_LUA_OBJ := $(FWDIST_LUA_SOURCE:.c=.o)


PROJ_CFLAGS += -g -I../Source/
PROJ_CFLAGS += -DVERSION="\"$(PROGRAM_VERSION) (r$(shell if [ -f version.txt ]; then cat version.txt; else svnversion ../Source; fi))\""

PROJ_CFLAGS += -fPIC

PROJ_LDFLAGS += -ldaemon -lpthread

vpath %.c ../Source/Common/ ../Source/Daemon/ ../Source/Clients


all: $(TARGETS)

$(TARGET_FWDIST_DAEMON): $(FWDIST_DAEMON_OBJ)
	$(CC)  $^ $(LDFLAGS) $(PROJ_LDFLAGS) -o $@

$(TARGET_FWDIST_CLI): $(FWDIST_CLI_OBJ)
	$(CC)  $^ $(LDFLAGS) $(PROJ_LDFLAGS) -o $@

$(TARGET_FWDIST_LUA): $(FWDIST_LUA_OBJ)
	$(CC) -o $(TARGET_FWDIST_LUA) -shared $(FWDIST_LUA_OBJ) $(LFLAGS)

%.o: %.c
	$(CC)  -I. $(CFLAGS) $(PROJ_CFLAGS) -c $<

install:
	mkdir -p $(DESTDIR)/sbin/
	cp $(TARGET_FWDIST_DAEMON) $(DESTDIR)/sbin/

clean:
	rm -f *.o $(TARGET_FWDIST_DAEMON) $(TARGET_FWDIST_CLI) $(TARGET_FWDIST_LUA)
