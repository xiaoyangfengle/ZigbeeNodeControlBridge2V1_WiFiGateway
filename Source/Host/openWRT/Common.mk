

REVISION_OPENWRT = 40431
REVISION_OPENWRT_FEEDS = 40431
REVISION_LUCI = 9964

INSTALL_PACKAGES = rsync luci avahi-dnsconfd mtd-utils mrd6 freeradius2 openssh-sftp-server tcpdump python pyserial libiwinfo picocom gw6c atftp libftdi i2c-tools

PATCHES_TARGET_DIR = ../backfire-patches/

PATCHES_COMMON_DIR = ../../../backfire-patches/

DOWNLOAD_DIR = ../../../downloads/

.PHONY: 

define install_package
	cd backfire && ./scripts/feeds install $(1)
endef

default: all


download_openWRT:
	echo "Checking out openWRT"
	svn co svn://svn.openwrt.org/openwrt/branches/attitude_adjustment@$(REVISION_OPENWRT) backfire
	touch download_openWRT

download_feeds:
	echo "Checking out feeds"
	cd backfire && ./scripts/feeds update

	# Now set to our known revision number
	echo "Getting known good packages dir"
	cd backfire/feeds/packages && svn up -r $(REVISION_OPENWRT_FEEDS)
	cd backfire/feeds/luci && svn up -r $(REVISION_LUCI)
	cd backfire && ./scripts/feeds update -i
	touch download_feeds


install_packages:
	#$(foreach package,$(INSTALL_PACKAGES),$(call install_package,$(package)))
	cd  backfire; ./scripts/feeds install $(INSTALL_PACKAGES)
	touch install_packages

copy_common:
	rsync -av --exclude .svn --exclude "*~" ../../backfire/ backfire/

patch_common:
	cd backfire && for i in $$(ls $(PATCHES_COMMON_DIR)); do echo "Applying common patch $$i"; patch -p1 < $(PATCHES_COMMON_DIR)/$$i; done

copy_target:
	rsync -av --exclude .svn --exclude "*~" backfire-changes/ backfire/

copy_config:
	cp config/config backfire/.config

patch_target:
	cd backfire && for i in $$(ls $(PATCHES_TARGET_DIR)); do echo "Applying target patch $$i"; patch -p1 < $(PATCHES_TARGET_DIR)/$$i; done
	cd backfire && ln -s $(DOWNLOAD_DIR) dl

all: download_openWRT download_feeds install_packages copy_common patch_common copy_target copy_config patch_target
	
