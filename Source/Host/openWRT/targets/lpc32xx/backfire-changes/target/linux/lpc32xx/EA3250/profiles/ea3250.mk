#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/EA3250
	NAME:=Embedded Artists E3250 Development Board
	PACKAGES:=kmod-usb-core kmod-usb2
endef

define Profile/EA3250/Description
	Package set optimized for the Embedded Artists 3250 Development Board
endef

$(eval $(call Profile,EA3250))
