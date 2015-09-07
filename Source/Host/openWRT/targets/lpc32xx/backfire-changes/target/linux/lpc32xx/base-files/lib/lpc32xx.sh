#!/bin/sh
#
# Copyright (C) 2009 OpenWrt.org
#

lpc32xx_board_name() {
	local machine
	local name

	machine=$(awk 'BEGIN{FS="[ \t]+:[ \t]"} /Hardware/ {print $2}' /proc/cpuinfo)

	case "$machine" in
	*JNRD6040*)
		name="JNRD6040"
		;;
	*)
		name="generic"
		;;
	esac

	echo $name
}
