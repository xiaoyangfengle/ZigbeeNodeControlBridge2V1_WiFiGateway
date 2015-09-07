#!/bin/sh

do_lpc32xx() {
	. /lib/lpc32xx.sh
}

boot_hook_add preinit_main do_lpc32xx
