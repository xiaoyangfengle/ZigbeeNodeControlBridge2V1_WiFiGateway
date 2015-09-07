#
# Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=zigbee-control-bridge
PKG_RELEASE:=1v4

zigbee-jip-daemon_FILENAME:=zigbee-jip-daemon-$(PKG_RELEASE).tar.gz

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/zigbee-control-bridge/Default
  SECTION:=nxp
  CATEGORY:=NXP
  URL:=http://www.jennic.com
endef

define Package/zigbee-control-bridge
$(call Package/zigbee-control-bridge/Default)
  TITLE:=Zigbee Control Bridge
  MENU:=1
endef

define Package/zigbee-control-bridge/description
 Support for NXP Zigbee Control Bridge
endef

# Jip Bindings

define Package/zigbee-control-bridge-JIP
$(call Package/zigbee-control-bridge/Default)
  TITLE:=Zigbee - JIP daemon
  DEPENDS:=zigbee-control-bridge +6LoWPAN-libJIP +libavahi-client +libdbus +libsqlite3
endef

define Package/zigbee-control-bridge-JIP/description
 Support for Zigbee Control bridge / JenNet-IP Interface
endef

define Build/zigbee-control-bridge-JIP/Prepare
	tar -xzf $(DL_DIR)/$(zigbee-jip-daemon_FILENAME) -C $(PKG_BUILD_DIR)
endef

define Build/zigbee-control-bridge-JIP/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/zigbee-jip-daemon/JIP/Build $(MAKE_FLAGS) $(MAKE_VARS) PROFILE=$(PROFILE) all
endef

define Build/Install
	# Nothing to do here.
endef

define Package/zigbee-control-bridge-JIP/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/zigbee-jip-daemon $(1)/etc/init.d/zigbee-jip-daemon
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/zigbee-jip-daemon $(1)/etc/config/zigbee-jip-daemon
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/zigbee-jip-daemon/JIP/Build/zigbee-jip-daemon $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/usr/share/zigbee-jip-daemon/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/zigbee-jip-daemon/JIP/Build/jip_cache_definitions.xml $(1)/usr/share/zigbee-jip-daemon/

# Luci integration
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller/
	$(INSTALL_BIN) ./files/usr/lib/lua/luci/controller/zigbee.lua $(1)/usr/lib/lua/luci/controller/zigbee.lua
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi/
	$(INSTALL_BIN) ./files/usr/lib/lua/luci/model/cbi/zigbee.lua $(1)/usr/lib/lua/luci/model/cbi/zigbee.lua
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi/zigbee/
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/model/cbi/zigbee/zigbee-jip-daemon.lua $(1)/usr/lib/lua/luci/model/cbi/zigbee/zigbee-jip-daemon.lua
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/model/cbi/zigbee/zigbee-jip-daemon-edit.lua $(1)/usr/lib/lua/luci/model/cbi/zigbee/zigbee-jip-daemon-edit.lua

endef


define Package/zigbee-control-bridge-JIP/postinst
#!/bin/sh
# check if we are on real system
echo "Enabling rc.d symlink for zigbee-jip-daemon"
[ -n "$${IPKG_INSTROOT}" ] || /etc/init.d/zigbee-jip-daemon enable || true
endef

define Package/zigbee-control-bridge-JIP/prerm
#!/bin/sh
# check if we are on real system
echo "Removing rc.d symlink for zigbee-jip-daemon"
[ -n "$${IPKG_INSTROOT}" ] || /etc/init.d/zigbee-jip-daemon disable || true
exit 0
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(if $(CONFIG_PACKAGE_zigbee-control-bridge-JIP),$(call Build/zigbee-control-bridge-JIP/Prepare),)
endef



define Build/Compile
	$(if $(CONFIG_PACKAGE_zigbee-control-bridge-JIP),$(call Build/zigbee-control-bridge-JIP/Compile),)
endef


$(eval $(call BuildPackage,zigbee-control-bridge))
$(eval $(call BuildPackage,zigbee-control-bridge-JIP))

