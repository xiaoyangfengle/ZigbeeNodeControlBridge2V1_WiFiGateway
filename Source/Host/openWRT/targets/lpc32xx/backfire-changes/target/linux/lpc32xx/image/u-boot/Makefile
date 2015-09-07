#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=u-boot
PKG_VERSION:=2009.03
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://git.lpclinux.com/uboot-2009.03-lpc32xx
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=u-boot-2009.03_lpc32x0-v1.06
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk

UBOOT_CONFIG=$(call qstrip,$(CONFIG_UBOOT_TARGET))

define Build/Compile
	if [ -f $(PKG_BUILD_DIR)/u-boot.bin ]; then \
		echo "u-boot already built"; \
	else \
		$(MAKE) -C $(PKG_BUILD_DIR) $(UBOOT_CONFIG)_config; \
		export CROSS_COMPILE=$(TARGET_CROSS); \
		$(MAKE) -C $(PKG_BUILD_DIR) EXTRA_CFLAGS=$(CONFIG_CFLAGS); \
	fi
endef

define Build/InstallDev
	mkdir -p $(BIN_DIR)/
	cp $(PKG_BUILD_DIR)/u-boot.bin $(BIN_DIR)/u-boot.lpc32xx.bin
endef

$(eval $(call Build/DefaultTargets))
