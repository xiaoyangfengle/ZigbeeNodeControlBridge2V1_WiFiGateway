#
# Copyright (C) 2009-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Build/Clean
	$(MAKE) -C u-boot clean
endef

define Build/Compile
	if [ $(CONFIG_LPC32XX_UBOOT) ]; then \
		$(MAKE) -C u-boot compile; \
	fi
endef

define Image/Prepare
endef


# VMLINUX:=$(BIN_DIR)/$(IMG_PREFIX)-vmlinux
# UIMAGE:=$(BIN_DIR)/$(IMG_PREFIX)-uImage
# fs_squash:=squashfs-only
# fs_all:=all
# fs_4k:=4k
# fs_64k:=64k
# fs_128k:=128k
ifeq ($(CONFIG_TARGET_ROOTFS_INITRAMFS),y)
# 	fs_squash:=initramfs
# 	fs_all:=initramfs
# 	fs_4k:=initramfs
# 	fs_64k:=initramfs
# 	fs_128k:=initramfs
# 	VMLINUX:=$(BIN_DIR)/$(IMG_PREFIX)-vmlinux-initramfs
# 	UIMAGE:=$(BIN_DIR)/$(IMG_PREFIX)-uImage-initramfs
endif


define Image/Prepare
	cp $(LINUX_DIR)/arch/arm/boot/uImage $(BIN_DIR)/$(IMG_PREFIX)-uImage
endef


# Define the name of the image file for the device
# Parameters
# 1 - Rootfs type
# 2 - profile
define imgname
$(BIN_DIR)/$(IMG_PREFIX)-$(2)-$(patsubst jffs2-%,jffs2,$(patsubst squashfs-%,squashfs,$(1)))
endef


# Define the name of the sysupgrade image file for the device
# Parameters
# 1 - Rootfs type
# 2 - profile
define sysupname
$(call imgname,$(1),$(2))-sysupgrade.bin
endef


# Cat files 2 and 3 together, checking the size.
# Parameters
# 1 - Kernel file
# 2 - Kernel space
# 3 - Rootfs file
# 4 - Rootfs space
# 5 - Output file
define CatFiles
	if [ `stat -c%s "$(1)"` -gt $(2) ]; then \
		echo "Warning: $(1) is too big"; \
	else if [ `stat -c%s $(3)` -gt $(4) ]; then \
		echo "Warning: $(3) is too big"; \
	else \
		( dd if=$(1) bs=$(2) conv=sync; dd if=$(3) ) > $(5); \
	fi; fi
endef


# Generate sysupgrade file kernel first, then rootfs image.
# Parameters
# 1 - root image type
# 2 - profile
# 3 - kernel space
# 4 - rootfs space
define Sysupgrade/KR
	$(call CatFiles,$(BIN_DIR)/$(IMG_PREFIX)-uImage,$(3),$(KDIR)/root.$(1),$(4),$(call sysupname,$(1),$(2)))
endef


# Generate images for the JNRD6040 board
# Parameters
# 1 - root image type
# 2 - profile
define Image/Build/JNRD6040
	$(call Sysupgrade/KR,$(1),$(2),4194304,128712704)
endef


# Tempate for building a jffs2-128k image
# Parameters
# 1 - profile
# 2 - profile filename
define Image/Build/Template/jffs2-128k
	$(call Image/Build/$(1),jffs2-128k,$(2))
endef


# Parameters
# 1 - root image type
define Image/Build/Profile/JNRD6040
	$(call Image/Build/Template/$(1),JNRD6040)
endef


# Build image fpr an lpc32xx target
# Parameters
# 1 - root image type
define Image/Build
	$(call Image/Build/$(1))
	dd if=$(KDIR)/root.$(1) of=$(BIN_DIR)/$(IMG_PREFIX)-root.$(1) bs=128k conv=sync
	$(call Image/Build/Profile/$(PROFILE),$(1))
endef

$(eval $(call BuildImage))
