#
# Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=6LoWPAN
PKG_RELEASE:=1v0_RC13

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

PKG_INSTALL:=1

# Package versions
# 6LoWPANd
6LoWPANd_VERSION:=v0_15_RC1
6LoWPANd_FILENAME:=6LoWPANd-$(6LoWPANd_VERSION).tar.gz

# JIPd
JIPd_VERSION:=v0_7_RC3
JIPd_FILENAME:=JIPd-$(JIPd_VERSION).tar.gz

# libJIP
libJIP_VERSION:=v2_3
libJIP_FILENAME:=libJIP-$(libJIP_VERSION).tar.gz

# JIPweb
JIPweb_VERSION:=v2_2_RC1
JIPweb_FILENAME:=JIPweb-$(JIPweb_VERSION).tar.gz

# FWDistribution
FWDistribution_VERSION:=v0_11
FWDistribution_FILENAME:=FWDistribution-$(FWDistribution_VERSION).tar.gz



include $(INCLUDE_DIR)/package.mk

define Package/6LoWPAN/Default
  SECTION:=nxp
  CATEGORY:=NXP
  URL:=http://www.jennic.com
endef

define Package/6LoWPAN
$(call Package/6LoWPAN/Default)
  TITLE:=6LoWPAN - NXP IEEE 802.15.4 Networking Support
  MENU:=1
endef

define Package/6LoWPAN/description
 6LoWPAN is an IPv6 networking layer on top of a IEEE 802.15.4 low power wireless network
endef

# 6LoWPANd

define Package/6LoWPAN-6LoWPANd
$(call Package/6LoWPAN/Default)
  DEPENDS:=6LoWPAN +kmod-tun +libavahi-client
  TITLE:=6LoWPANd
endef

define Package/6LoWPAN-6LoWPANd/description
 6LoWPAN userspace routing daemon
endef

# JIPd

define Package/6LoWPAN-JIPd
$(call Package/6LoWPAN/Default)
  DEPENDS:=6LoWPAN +libavahi-client
  TITLE:=JIPd
endef

define Package/6LoWPAN-JIPd/description
 JIP application level IPv4 gateway
endef

# libJIP

define Package/6LoWPAN-libJIP
$(call Package/6LoWPAN/Default)
  DEPENDS:=6LoWPAN +libxml2 +libtecla
  TITLE:=libJIP
endef

define Package/6LoWPAN-libJIP/description
 JIP library
endef

define Package/6LoWPAN-libJIP-CLI
$(call Package/6LoWPAN/Default)
  DEPENDS:=6LoWPAN +6LoWPAN-libJIP +libtecla
  TITLE:=JIP Command Line Interface
endef

define Package/6LoWPAN-libJIP-CLI/description
  Command line application to interact with JIP devices
endef

# JIP web interfaces

define Package/6LoWPAN-JIPweb
$(call Package/6LoWPAN/Default)
  DEPENDS:=6LoWPAN +6LoWPAN-libJIP +uhttpd +libavahi-client +libjson
  TITLE:=JIP web interfaces
endef

define Package/6LoWPAN-JIPweb/description
 JIP web interfaces
endef

# JIP Testing support

define Package/6LoWPAN-libJIP-test
$(call Package/6LoWPAN/Default)
  DEPENDS:=6LoWPAN +6LoWPAN-libJIP +python
  TITLE:=Support for libJIP Testing
endef

define Package/6LoWPAN-libJIP-test/description
 Add support for testing libJIP
endef

# Firmware distribution daemon / client

define Package/6LoWPAN-FirmwareDistribution
$(call Package/6LoWPAN/Default)
  DEPENDS:=6LoWPAN +libdaemon +libpthread
  TITLE:=Support for Firmware Distribution
endef

define Package/6LoWPAN-FirmwareDistribution/description
 Firmware distribution daemon and client programs
endef


# 6LoWPANd
define Build/6LoWPAN-6LoWPANd/Prepare
	tar -xzf $(DL_DIR)/$(6LoWPANd_FILENAME) -C $(PKG_BUILD_DIR)
endef

define Build/6LoWPAN-6LoWPANd/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/6LoWPANd/Build        $(MAKE_FLAGS) $(MAKE_VARS) all
endef


# JIPd
define Build/6LoWPAN-JIPd/Prepare
	tar -xzf $(DL_DIR)/$(JIPd_FILENAME) -C $(PKG_BUILD_DIR)
endef

define Build/6LoWPAN-JIPd/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/JIPd/Build        $(MAKE_FLAGS) $(MAKE_VARS) all
endef


# libJIP
define Build/6LoWPAN-libJIP/Prepare
	tar -xzf $(DL_DIR)/$(libJIP_FILENAME) -C $(PKG_BUILD_DIR)
endef

define Build/6LoWPAN-libJIP/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/libJIP/Build        $(MAKE_FLAGS) $(MAKE_VARS) all
endef


define Build/6LoWPAN-libJIP/InstallDev
# Copy include and library to host for linking against.
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/libJIP/Include/* $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libJIP/Library/libJIP.so* $(1)/usr/lib/
endef


# JIPweb
define Build/6LoWPAN-JIPweb/Prepare
	tar -xzf $(DL_DIR)/$(JIPweb_FILENAME) -C $(PKG_BUILD_DIR)
endef

# Add include path for libjson to CFLAGS
TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include/json

define Build/6LoWPAN-JIPweb/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/JIPweb/Build        $(MAKE_FLAGS) $(MAKE_VARS) all
endef


# FirmwareDistribution
define Build/6LoWPAN-FirmwareDistribution/Prepare
	tar -xzf $(DL_DIR)/$(FWDistribution_FILENAME) -C $(PKG_BUILD_DIR)
endef

define Build/6LoWPAN-FirmwareDistribution/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/FWDistribution/Build        $(MAKE_FLAGS) $(MAKE_VARS) all
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(if $(CONFIG_PACKAGE_6LoWPAN-6LoWPANd),$(call Build/6LoWPAN-6LoWPANd/Prepare),)
	$(if $(CONFIG_PACKAGE_6LoWPAN-JIPd),$(call Build/6LoWPAN-JIPd/Prepare),)
	$(if $(CONFIG_PACKAGE_6LoWPAN-libJIP),$(call Build/6LoWPAN-libJIP/Prepare),)
	$(if $(CONFIG_PACKAGE_6LoWPAN-JIPweb),$(call Build/6LoWPAN-JIPweb/Prepare),)
	$(if $(CONFIG_PACKAGE_6LoWPAN-FirmwareDistribution),$(call Build/6LoWPAN-FirmwareDistribution/Prepare),)
endef



define Build/Compile
	$(if $(CONFIG_PACKAGE_6LoWPAN-6LoWPANd),$(call Build/6LoWPAN-6LoWPANd/Compile),)
	$(if $(CONFIG_PACKAGE_6LoWPAN-JIPd),$(call Build/6LoWPAN-JIPd/Compile),)
	$(if $(CONFIG_PACKAGE_6LoWPAN-libJIP),$(call Build/6LoWPAN-libJIP/Compile),)
	$(if $(CONFIG_PACKAGE_6LoWPAN-JIPweb),$(call Build/6LoWPAN-JIPweb/Compile),)
	$(if $(CONFIG_PACKAGE_6LoWPAN-FirmwareDistribution),$(call Build/6LoWPAN-FirmwareDistribution/Compile),)
endef

define Build/Install
	# Nothing to do here.
endef


define Build/InstallDev
	echo $(1)
	$(if $(CONFIG_PACKAGE_6LoWPAN-libJIP),$(call Build/6LoWPAN-libJIP/InstallDev,$(1)),)
endef

define Package/6LoWPAN/conffiles
/etc/config/6LoWPAN
/etc/config/radvd
endef

# 6LoWPANd

define Package/6LoWPAN-6LoWPANd/install
# Base files
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/6LoWPAN $(1)/etc/init.d/6LoWPAN
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/6LoWPAN $(1)/etc/config/6LoWPAN
	$(INSTALL_DIR) $(1)/etc/rc.d
	$(INSTALL_BIN) ./files/etc/rc.d/S996LoWPAN $(1)/etc/rc.d/S996LoWPAN
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/6LoWPANd/Build/6LoWPANd $(1)/sbin/

# Security
	$(INSTALL_DIR) $(1)/etc/freeradius2
	$(INSTALL_CONF) ./files/etc/freeradius2/users.6LoWPAN $(1)/etc/freeradius2/users.6LoWPAN
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/greylist.sh $(1)/usr/bin/greylist.sh

# Standalone -> Gateway
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) ./files/usr/sbin/6LoWPANd-config.sh $(1)/usr/sbin/6LoWPANd-config.sh
	$(INSTALL_BIN) ./files/usr/sbin/standalone-gateway.sh $(1)/usr/sbin/standalone-gateway.sh

# Luci Integration
	$(INSTALL_DIR) $(1)/usr/lib/lua/
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller/
	$(INSTALL_BIN) ./files/usr/lib/lua/luci/controller/6LoWPAN.lua $(1)/usr/lib/lua/luci/controller/6LoWPAN.lua
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi/
	$(INSTALL_BIN) ./files/usr/lib/lua/luci/model/cbi/6LoWPAN.lua $(1)/usr/lib/lua/luci/model/cbi/6LoWPAN.lua
	$(INSTALL_BIN) ./files/usr/lib/lua/luci/model/cbi/6LoWPAN_whitelist.lua $(1)/usr/lib/lua/luci/model/cbi/6LoWPAN_whitelist.lua
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi/6LoWPAN/
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/model/cbi/6LoWPAN/6LoWPANd.lua $(1)/usr/lib/lua/luci/model/cbi/6LoWPAN/6LoWPANd.lua
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/model/cbi/6LoWPAN/6LoWPANd_edit.lua $(1)/usr/lib/lua/luci/model/cbi/6LoWPAN/6LoWPANd_edit.lua

# Luci Theme
	$(INSTALL_DIR) $(1)/www/luci-static/nxp/
	$(INSTALL_DATA) ./files/www/luci-static/nxp/cascade.css $(1)/www/luci-static/nxp/cascade.css
	$(INSTALL_DATA) ./files/www/luci-static/nxp/ie6.css $(1)/www/luci-static/nxp/ie6.css
	$(INSTALL_DATA) ./files/www/luci-static/nxp/ie7.css $(1)/www/luci-static/nxp/ie7.css
	$(INSTALL_DATA) ./files/www/luci-static/nxp/ie8.css $(1)/www/luci-static/nxp/ie8.css
	$(INSTALL_DATA) ./files/www/luci-static/nxp/tabbg.png $(1)/www/luci-static/nxp/tabbg.png

	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/view/themes/nxp/
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/view/themes/nxp/footer.htm $(1)/usr/lib/lua/luci/view/themes/nxp/footer.htm
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/view/themes/nxp/header.htm $(1)/usr/lib/lua/luci/view/themes/nxp/header.htm

	$(INSTALL_DIR) $(1)/etc/uci-defaults/
	$(INSTALL_DATA) ./files/etc/uci-defaults/luci-theme-nxp $(1)/etc/uci-defaults/luci-theme-nxp

# Whitelist support
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/view/6LoWPAN
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/view/6LoWPAN/Whitelist.htm $(1)/usr/lib/lua/luci/view/6LoWPAN/Whitelist.htm

endef

# JIPd

define Package/6LoWPAN-JIPd/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/JIPd $(1)/etc/init.d/JIPd
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/6LoWPAN $(1)/etc/config/6LoWPAN
	$(INSTALL_DIR) $(1)/etc/rc.d
	$(INSTALL_BIN) ./files/etc/rc.d/S99JIPd $(1)/etc/rc.d/S99JIPd
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/JIPd/Build/JIPd $(1)/sbin/
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi/6LoWPAN/
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/model/cbi/6LoWPAN/JIPd.lua $(1)/usr/lib/lua/luci/model/cbi/6LoWPAN/JIPd.lua
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/model/cbi/6LoWPAN/JIPd_edit.lua $(1)/usr/lib/lua/luci/model/cbi/6LoWPAN/JIPd_edit.lua
endef

# libJIP

define Package/6LoWPAN-libJIP/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libJIP/Library/libJIP.so* $(1)/usr/lib/
endef

define Package/6LoWPAN-libJIP-CLI/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/libJIP/Build/JIP $(1)/usr/bin/
endef

# JIPweb

define Package/6LoWPAN-JIPweb/install
	$(INSTALL_DIR) $(1)/www/
	$(INSTALL_DIR) $(1)/www/cgi-bin
	$(INSTALL_DIR) $(1)/www/img
	$(INSTALL_DIR) $(1)/www/js
	$(INSTALL_DIR) $(1)/www/bootstrap
	$(INSTALL_DIR) $(1)/www/bootstrap/js
	$(INSTALL_DIR) $(1)/www/bootstrap/css
	$(INSTALL_DIR) $(1)/www/bootstrap/fonts

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/JIPweb/Build/JIP.cgi $(1)/www/cgi-bin/JIP.cgi

	$(INSTALL_DATA) $(PKG_BUILD_DIR)/JIPweb/www/Browser.html $(1)/www/Browser.html
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/JIPweb/www/SmartDevices.html $(1)/www/SmartDevices.html

	$(INSTALL_DATA) $(PKG_BUILD_DIR)/JIPweb/www/img/* $(1)/www/img/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/JIPweb/www/js/*.js $(1)/www/js/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/JIPweb/www/bootstrap/js/* $(1)/www/bootstrap/js/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/JIPweb/www/bootstrap/css/* $(1)/www/bootstrap/css/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/JIPweb/www/bootstrap/fonts/* $(1)/www/bootstrap/fonts/

	$(INSTALL_DATA) $(PKG_BUILD_DIR)/JIPweb/www/style.css $(1)/www/style.css
endef

# JIP Testing

define Package/6LoWPAN-libJIP-test/install
	$(INSTALL_DIR) $(1)/usr/lib/python2.7/site-packages/
	$(CP) $(PKG_BUILD_DIR)/libJIP/Python/libJIP.py $(1)/usr/lib/python2.7/site-packages/
	$(INSTALL_DIR) $(1)/root/libJIPTesting
	$(CP) $(PKG_BUILD_DIR)/libJIP/Test/Test_Case_1.py $(1)/root/libJIPTesting
	$(CP) $(PKG_BUILD_DIR)/libJIP/Test/Test_Case_2.py $(1)/root/libJIPTesting
endef

#Firmware Distribution tools

define Package/6LoWPAN-FirmwareDistribution/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/FWDistribution/Build/FWDistributiond $(1)/sbin/
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/FWDistribution/Build/FWDistributionControl $(1)/usr/bin/FWDistributionControl
	$(INSTALL_DIR) $(1)/usr/lib/lua
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/FWDistribution/Build/FWDistribution.so $(1)/usr/lib/lua/FWDistribution.so
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/view/6LoWPAN
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/view/6LoWPAN/FWDistribution.htm $(1)/usr/lib/lua/luci/view/6LoWPAN/FWDistribution.htm
	
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/FWDistributiond $(1)/etc/init.d/FWDistributiond
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/FWDistributiond $(1)/etc/config/FWDistributiond
	$(INSTALL_DIR) $(1)/etc/rc.d
	$(INSTALL_BIN) ./files/etc/rc.d/S99FWDistributiond $(1)/etc/rc.d/S99FWDistributiond
endef

# Misc configuration stuff

define Package/6LoWPAN/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/radvd $(1)/etc/config/radvd
endef


$(eval $(call BuildPackage,6LoWPAN))
$(eval $(call BuildPackage,6LoWPAN-6LoWPANd))
$(eval $(call BuildPackage,6LoWPAN-JIPd))
$(eval $(call BuildPackage,6LoWPAN-libJIP))
$(eval $(call BuildPackage,6LoWPAN-libJIP-CLI))
$(eval $(call BuildPackage,6LoWPAN-JIPweb))
$(eval $(call BuildPackage,6LoWPAN-libJIP-test))
$(eval $(call BuildPackage,6LoWPAN-FirmwareDistribution))

