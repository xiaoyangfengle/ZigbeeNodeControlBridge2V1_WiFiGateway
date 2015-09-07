#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

BOARD:=lpc32xx
BOARDNAME:=NXP LPC32xx Based system
FEATURES:=targz usb
CFLAGS:=-Os -fsigned-char -mfloat-abi=softfp -mfpu=vfp
SUBTARGETS:=EA3250 JNRD6040

LINUX_VERSION:=3.3.8

include $(INCLUDE_DIR)/target.mk

KERNELNAME:="uImage"

#DEFAULT_PACKAGES +=

$(eval $(call BuildTarget))
