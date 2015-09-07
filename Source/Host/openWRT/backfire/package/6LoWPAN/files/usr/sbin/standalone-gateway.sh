#!/bin/sh

LoWPANd="6LoWPANd"
LoWPANd_path="/sbin/$LoWPANd"

# File created once configuration is complete
CONFIG_NOTIFY_FILE="/tmp/6LoWPANd.configured"

SERIAL=$1
NETWORK=$2

usage()
{
    echo ""
    echo "Usage: $0 <BR Serial port> <Network>"
    echo "BR Serial Port - Serial port connected to border router node e.g. /dev/ttyUSB0 or /dev/ttyACM0"
    echo "Network - ID of network to try to join e.g. 0x87654321"
    echo ""
}


if [ -z $SERIAL ] || [ -z $NETWORK ]; then
    usage
    exit 1
fi

# Build arguments for running 6LoWPANd in commisioning mode
ARGS="--reset --serial $SERIAL --network $NETWORK --key=:: --mode=commissioning --confignotify /usr/sbin/6LoWPANd-config.sh"

# Delete the notification file
rm -f $CONFIG_NOTIFY_FILE

# Ensure 6LoWPANd is not already running
killall $LoWPANd

while [ `pidof $LoWPANd` ]; do
    sleep 1
    killall -9 $LoWPANd
done

# Run 6LoWPANd
$LoWPANd_path $ARGS

# Wait for configuration to be received
while ! [ -f $CONFIG_NOTIFY_FILE ]; do
    sleep 1
done

if [ -f $CONFIG_NOTIFY_FILE ]; then
    echo "Configuration received. Restarting 6LoWPANd"

    /etc/init.d/6LoWPAN restart
fi
