#
# Copyright (C) 2009 OpenWrt.org
#

. /lib/lpc32xx.sh

PART_NAME=firmware
RAMFS_COPY_DATA=/lib/lpc32xx.sh

CI_BLKSZ=65536
CI_LDADR=0x80100000

platform_find_partitions() {
	local first dev size erasesize name
	while read dev size erasesize name; do
		name=${name#'"'}; name=${name%'"'}
		case "$name" in
			vmlinux.bin.l7|kernel|linux|rootfs)
				if [ -z "$first" ]; then
					first="$name"
				else
					echo "$erasesize:$first:$name"
					break
				fi
			;;
		esac
	done < /proc/mtd
}

platform_find_kernelpart() {
	local part
	for part in "${1%:*}" "${1#*:}"; do
		case "$part" in
			vmlinux.bin.l7|kernel|linux)
				echo "$part"
				break
			;;
		esac
	done
}

platform_do_upgrade_combined() {
	local partitions=$(platform_find_partitions)
	local kernelpart=$(platform_find_kernelpart "${partitions#*:}")
	local erase_size=$((0x${partitions%%:*})); partitions="${partitions#*:}"
	local kern_length=0x$(dd if="$1" bs=2 skip=1 count=4 2>/dev/null)
	local kern_blocks=$(($kern_length / $CI_BLKSZ))
	local root_blocks=$((0x$(dd if="$1" bs=2 skip=5 count=4 2>/dev/null) / $CI_BLKSZ))

	if [ -n "$partitions" ] && [ -n "$kernelpart" ] && \
	   [ ${kern_blocks:-0} -gt 0 ] && \
	   [ ${root_blocks:-0} -gt ${kern_blocks:-0} ] && \
	   [ ${erase_size:-0} -gt 0 ];
	then
		local append=""
		[ -f "$CONF_TAR" -a "$SAVE_CONFIG" -eq 1 ] && append="-j $CONF_TAR"

		( dd if="$1" bs=$CI_BLKSZ skip=1 count=$kern_blocks 2>/dev/null; \
		  dd if="$1" bs=$CI_BLKSZ skip=$((1+$kern_blocks)) count=$root_blocks 2>/dev/null ) | \
			mtd -r $append -F$kernelpart:$kern_length:$CI_LDADR,rootfs write - $partitions
	fi
}

platform_check_image() {
	local board=$(lpc32xx_board_name)
	local magic="$(get_magic_word "$1")"
	local magic_long="$(get_magic_long "$1")"

	[ "$ARGC" -gt 1 ] && return 1

	case "$board" in
	JNRD6040)
		[ "$magic" != "2705" ] && {
			echo "Invalid image type."
			return 1
		}
		return 0
		;;
	esac

	echo "Sysupgrade is not yet supported on $board."
	return 1
}

platform_do_upgrade() {
	local board=$(lpc32xx_board_name)

	case "$board" in
	*)
		default_do_upgrade "$ARGV"
		;;
	esac
}

disable_watchdog() {
	killall watchdog
	( ps | grep -v 'grep' | grep '/dev/watchdog' ) && {
		echo 'Could not disable watchdog'
		return 1
	}
}

append sysupgrade_pre_upgrade disable_watchdog
