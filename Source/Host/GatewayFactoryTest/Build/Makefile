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

# Overridden by openWRT makefiles with build profile.
# Used to select the active modules
PROFILE ?= GENERIC


SOURCE := Test_Device.c CGI.c

FTProg_SOURCE := FTProg.c

CFLAGS = -Wall -g

OBJ := $(SOURCE:.c=.o)

FTProg_OBJ := $(FTProg_SOURCE:.c=.o)

EXEC_CGI_OBJ := $(EXEC_CGI_SOURCE:.c=.o)


PROJ_CFLAGS += -I../Source/ -DPROFILE_$(PROFILE)
PROJ_CFLAGS += -DVERSION="\"$(shell if [ -f version.txt ]; then cat version.txt; else svnversion ../Source; fi)\""

PROJ_LDFLAGS += 
# --gc-sections

vpath %.c ../Source

TARGET = FactoryTest.cgi

all: $(TARGET) FTProg

FTProg: $(FTProg_OBJ)
	$(CC) $^ $(LDFLAGS) -lftdi -lusb $(PROJ_LDFLAGS) -o $@

$(TARGET): $(OBJ)
	$(CC)  $^ $(LDFLAGS) $(PROJ_LDFLAGS) -o $@

%.o: %.c
	$(CC)  -I. $(CFLAGS) $(PROJ_CFLAGS) -c $<

install:
	mkdir -p $(DESTDIR)/usr/bin/
	cp $(TARGET) $(DESTDIR)/usr/bin/

clean:
	rm -f *.o $(TARGET) FTProg
