#
# Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=GatewayFactoryTest
PKG_RELEASE:=v0_2

GatewayFactoryTest_FILENAME:=$(PKG_NAME)-$(PKG_RELEASE).tar.gz

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/GatewayFactoryTest/Default
  SECTION:=nxp
  CATEGORY:=NXP
  URL:=http://www.jennic.com
endef

define Package/GatewayFactoryTest
$(call Package/GatewayFactoryTest/Default)
  TITLE:=Factory Test Programs for gateway
  DEPENDS:=+libftdi +libusb
endef

define Package/GatewayFactoryTest/description
 Provides support for factory testing of gateway hardware
 Provides support for configuring USB dongles via cgi page
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	tar -xzf $(DL_DIR)/$(GatewayFactoryTest_FILENAME) -C $(PKG_BUILD_DIR)
endef



define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/GatewayFactoryTest/Build $(MAKE_FLAGS) $(MAKE_VARS) PROFILE=$(PROFILE) all
endef

define Build/Install
	# Nothing to do here.
endef

define Package/GatewayFactoryTest/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/GatewayFactoryTest/Build/FTProg $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/www/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/GatewayFactoryTest/Build/FactoryTest.css $(1)/www/
	$(INSTALL_DIR) $(1)/www/cgi-bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/GatewayFactoryTest/Build/FactoryTest.cgi $(1)/www/cgi-bin/
endef


$(eval $(call BuildPackage,GatewayFactoryTest))

