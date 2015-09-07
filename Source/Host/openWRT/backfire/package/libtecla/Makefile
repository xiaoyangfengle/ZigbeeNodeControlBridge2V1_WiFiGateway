# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libtecla
PKG_VERSION:=1.6.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.astro.caltech.edu/~mcs/tecla/
PKG_MD5SUM:=1892c8db9fecd38ed686b3ccf72a569b

# tar.gz doesn't have version embedded
PKG_BUILD_DIR = $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/libtecla
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Command lines edition library
  URL:=http://www.astro.caltech.edu/~mcs/tecla/index.html
  DEPENDS:=+libncurses
endef

define Package/libtecla/description
	The tecla library provides UNIX and LINUX programs with interactive command line 
	editing facilities, similar to those of the UNIX tcsh shell. In addition to simple 
	command-line editing, it supports recall of previously entered command lines, 
	TAB completion of file names or other tokens, and in-line wild-card expansion of 
	filenames. The internal functions which perform file-name completion and wild-card 
	expansion are also available externally for optional use by programs.
endef

#define Build/Configure
#	$(call Build/Configure/Default,)
#endef

TARGET_CPPFLAGS:=-I. -I.. $(TARGET_CPPFLAGS)

TARGET_CFLAGS += $(FPIC)

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		normal install_lib install_inc
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/libtecla.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libtecla.{a,so,so.1,so.1.6.1} $(1)/usr/lib/
endef

define Package/libtecla/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libtecla.{so,so.1,so.1.6.1} $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libtecla))
