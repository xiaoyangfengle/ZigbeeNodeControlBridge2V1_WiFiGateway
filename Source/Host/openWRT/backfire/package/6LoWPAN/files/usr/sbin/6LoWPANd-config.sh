#!/bin/sh

# Script to update UCI configuration with that received from commissioning border router.
# Arguments:
# --channel=24 --pan=0x3a79 --network=0x3a773a79 --prefix=fd04:bd3:80e8:2:: --key=::1

# File created once configuration is complete
CONFIG_NOTIFY_FILE="/tmp/6LoWPANd.configured"

usage()
{
    echo ""
    echo "Usage: $0"
    echo "Required Arguments:"
    echo "--interface   <Interface>     6LoWPANd interface instance to work with"
    echo "--channel     <Channel>       IEEE802.15.4 channel to operate on"
    echo "--pan         <PAN ID>        IEEE802.15.4 pan ID to start"
    echo "--network     <Network>       JenNet network ID to start"
    echo "--prefix      <Prefix>        IPv6 Prefix to operate with"
    echo "Optional Arguments:"
    echo "--key         <Key>           Security key in use"
    echo ""
    echo "E.g: $0 --interface=tun0 --channel=24 --pan=0x3a79 --network=0x3a773a79 --prefix=fd04:bd3:80e8:2:: --key=::1"
    echo ""
}

# Parse arguments
for i in $*
do
    case $i in
        --interface=*)
        INTERFACE=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        ;;
        --channel=*)
        CHANNEL=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        ;;
        --pan=*)
        PANID=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        ;;
        --network=*)
        NETWORK=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        ;;
        --prefix=*)
        PREFIX=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        ;;
        --key=*)
        KEY=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        ;;
        *)
        echo "Unknown option $i"
        ;;
    esac
done

if [ -z $INTERFACE ] || [ -z $CHANNEL ] || [ -z $PANID ] || [ -z $NETWORK ]; then
    usage
    exit 1
fi

echo "Setting configuration of 6LoWPANd for interface $INTERFACE:"
echo " Channel: $CHANNEL"
echo " PAN ID : $PANID"
echo " Network: $NETWORK"
echo " Prefix : $PREFIX"
echo " Key    : $KEY"

# We do not set the in use prefix, as we want the nodes to start using our prefix
# Once the gateway starts with the new configuration.

# Set the parameters in UCI configuration files
uci set 6LoWPAN.$INTERFACE.channel=$CHANNEL
uci set 6LoWPAN.$INTERFACE.pan=$PANID
uci set 6LoWPAN.$INTERFACE.jennet=$NETWORK
if [ -n "$KEY" ]; then
    echo "Security enabled"
    uci set 6LoWPAN.$INTERFACE.secure=1
    uci set 6LoWPAN.$INTERFACE.jennet_key=$KEY
fi
uci commit 6LoWPAN

# Signal that configuration is complete
touch $CONFIG_NOTIFY_FILE
