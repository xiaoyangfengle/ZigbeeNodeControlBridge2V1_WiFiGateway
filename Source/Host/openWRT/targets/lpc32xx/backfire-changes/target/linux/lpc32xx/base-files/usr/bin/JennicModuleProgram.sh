#!/bin/sh

PROGRAMMER="/usr/bin/JennicModuleProgrammer"

JENNIC_PIN_RST="/sys/devices/platform/jennic-module/rst/value"
JENNIC_PIN_PGM="/sys/devices/platform/jennic-module/pgm/value"
JENNIC_TTY="/dev/ttyTX0"

BAUD_INITIAL=921600
BAUD_PROGRAM=921600

if [ $1 ]
then
    ARGS_PROGRAM="-f $1 -v"
fi


# Reset the chip - briefly assert reset line
chip_reset ()
{
    echo "Resetting chip"
    echo 0 > $JENNIC_PIN_RST
    echo 1 > $JENNIC_PIN_RST
}

# Put the chip into bootloader mode
chip_mode_bootloader ()
{
    echo "Setting bootloader mode"
    echo 0 > $JENNIC_PIN_PGM
    chip_reset
}

# Put the chip into normal run mode
chip_mode_run ()
{
    echo "Setting run mode"
    echo 1 > $JENNIC_PIN_PGM
    chip_reset
}


# Put chip into programming mode
chip_mode_bootloader

# Program device.
$PROGRAMMER -I $BAUD_INITIAL -P $BAUD_PROGRAM -s $JENNIC_TTY $ARGS_PROGRAM
result=$?

# Run
chip_mode_run

# Return the programmin status
return $result
