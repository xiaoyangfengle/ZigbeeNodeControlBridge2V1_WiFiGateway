#
# Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=serial-tester
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/serial-tester/Default
  SECTION:=nxp
  CATEGORY:=NXP
  URL:=http://www.jennic.com
endef

define Package/serial-tester
$(call Package/serial-tester/Default)
  TITLE:=Serial Tester
endef

define Package/serial-tester/description
 Web based test program to verify FTDI cable manufacture
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	svn co https://www.collabnet.nxp.com/svn/lprf_sware/Projects/6LoWPAN/Application/Host_Machine/SerialTester/Branches/Stable $(PKG_BUILD_DIR)/SerialTester
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/SerialTester/Build    $(MAKE_FLAGS) $(MAKE_VARS) all
endef

define Build/Install
	# Nothing to do here.
endef

define Package/serial-tester/conffiles
endef

# Serial Tester

define Package/serial-tester/install
	$(INSTALL_DIR) $(1)/www/cgi-bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/SerialTester/Build/serialtest.cgi $(1)/www/cgi-bin/serialtest.cgi
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/SerialTester/Build/pass.png $(1)/www/pass.png
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/SerialTester/Build/fail.png $(1)/www/fail.png

endef

$(eval $(call BuildPackage,serial-tester))

