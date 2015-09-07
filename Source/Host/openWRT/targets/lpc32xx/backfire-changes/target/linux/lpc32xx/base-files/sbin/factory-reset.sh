#!/bin/sh
#
# Copyright (C) 2012 www.nxp.com
#

. /lib/lpc32xx.sh
. /etc/diag.sh

local board=$(lpc32xx_board_name)

[ ! -d /etc/defconfig/$board ] && board="generic"

get_status_led

# Flash diag LED fast to show what we're doing.
status_led_set_timer 25 25

# Copy factory defaults
for f in $( ls /etc/defconfig/$board ); do
		cp /etc/defconfig/$board/$f /etc/config/
done

# Keep it flashing a while
sleep 2

# On for a second before rebooting.
status_led_on
sleep 1

reboot
