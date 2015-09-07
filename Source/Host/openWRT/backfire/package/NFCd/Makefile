#
# Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=NFCd
PKG_RELEASE:=0v2

NFCd_FILENAME:=NFCd-$(PKG_RELEASE).tar.gz

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/NFCd/Default
  SECTION:=nxp
  CATEGORY:=NXP
  URL:=http://www.jennic.com
endef

define Package/NFCd
$(call Package/NFCd/Default)
  TITLE:=NFC Daemon
  DEPENDS:=+libdaemon +libpthread
endef

define Package/NFCd/description
 Support for NFC expansion board. The NFC daemon allows commissioning of devices via NFC.
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	tar -xzf $(DL_DIR)/$(NFCd_FILENAME) -C $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/NFCd/Build $(MAKE_FLAGS) $(MAKE_VARS) PROFILE=$(PROFILE) all
endef

define Build/Install
	# Nothing to do here.
endef

define Package/NFCd/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/NFCd $(1)/etc/init.d/NFCd
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/NFCd $(1)/etc/config/NFCd
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/NFCd/Build/NFCd $(1)/usr/sbin/
endef

define Package/NFCd/postinst
#!/bin/sh
# check if we are on real system
echo "Enabling rc.d symlink for NFCd"
[ -n "$${IPKG_INSTROOT}" ] || /etc/init.d/NFCd enable || true
endef

define Package/NFCd/prerm
#!/bin/sh
# check if we are on real system
echo "Removing rc.d symlink for NFCd"
[ -n "$${IPKG_INSTROOT}" ] || /etc/init.d/NFCd disable || true
exit 0
endef


$(eval $(call BuildPackage,NFCd))

