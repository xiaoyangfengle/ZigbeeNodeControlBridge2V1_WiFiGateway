#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/JNRD6040
	NAME:=NXP JNRD6040 JenNet-IP border router reference design
	PACKAGES:=kmod-usb-core kmod-usb2
endef

define Profile/JNRD6040/Description
	Package set optimized for the NXP JenNet-IP border router reference design
endef

$(eval $(call Profile,JNRD6040))
