#
# Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=JIP-Cloud-Nabto
PKG_RELEASE:=v0_1

NabtoStarterKit_FILENAME:=unabto-starterkit.zip

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/JIP-Cloud-Nabto/Default
  SUBMENU:=Cloud Support
  URL:=http://www.jennic.com
endef

define Package/JIP-Cloud-Nabto
$(call Package/JIP-Cloud-Nabto/Default)
  SECTION:=nxp
  CATEGORY:=NXP
  TITLE:=Nabto - JIP integration
  DEPENDS:=6LoWPAN-libJIP +libavahi-common +libavahi-client +libdbus
endef

define Package/JIP-Cloud-Nabto/description
 Provides support the Nabto cloud system
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	unzip $(DL_DIR)/$(NabtoStarterKit_FILENAME) -d $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/ $(MAKE_FLAGS) $(MAKE_VARS) PROFILE=$(PROFILE) all
endef

define Build/Install
	# Nothing to do here.
endef

define Package/JIP-Cloud-Nabto/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/JIP-nabto $(1)/usr/sbin/
endef


$(eval $(call BuildPackage,JIP-Cloud-Nabto))

