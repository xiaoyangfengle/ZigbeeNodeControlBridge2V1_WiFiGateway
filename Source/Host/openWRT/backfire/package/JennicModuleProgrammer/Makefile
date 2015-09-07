#
# Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=JennicModuleProgrammer
PKG_RELEASE:=v0_6

JennicModuleProgrammer_FILENAME:=$(PKG_NAME)-$(PKG_RELEASE).tar.gz

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/JennicModuleProgrammer/Default
  SECTION:=nxp
  CATEGORY:=NXP
  URL:=http://www.jennic.com
endef

define Package/JennicModuleProgrammer
$(call Package/JennicModuleProgrammer/Default)
  TITLE:=Jennic Module Programmer
endef

define Package/JennicModuleProgrammer/description
 Provides support for programming the flash of Jennic JN51xx devices
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	tar -xzf $(DL_DIR)/$(JennicModuleProgrammer_FILENAME) -C $(PKG_BUILD_DIR)
endef



define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/JennicModuleProgrammer/Build        $(MAKE_FLAGS) $(MAKE_VARS) all
endef

define Build/Install
	# Nothing to do here.
endef

define Package/JennicModuleProgrammer/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/JennicModuleProgrammer/Build/JennicModuleProgrammer $(1)/usr/bin/
endef


$(eval $(call BuildPackage,JennicModuleProgrammer))

