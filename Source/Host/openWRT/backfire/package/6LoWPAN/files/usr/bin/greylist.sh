#!/bin/sh

# File to add user to
usersfile=users.6LoWPAN
#userspath=/etc/raddb/
userspath=/etc/freeradius2/

greylisttime=`date`

# Lines should be commented out (greylisted) normally.
# If --white is passed in then the line should not be 
# commented and it should be enabled.
white=0


# Parse arguments
for i in $*
do
    case $i in
        --user=*)
        USER=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        ;;
    esac

    case $i in
        --password=*)
        PASS=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        ;;
    esac

    case $i in
        --key=*)
        KEY=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        ;;
    esac

    case $i in
        --white)
        white=1
        ;;
    esac
done

if [ $white -eq 1 ]; then
    LINE_START=""
else
    LINE_START="# "
fi

if [ -z $PASS ]; then
    # Use username as password
    PASS=$USER
fi


if [ -z $KEY ]; then
    # Autogenerate key
    echo "Autogenerating key"
    KEY=` echo $USER | awk '{ p=""; for(i=length($1)-1; i>0;i-=2) { p = p "00"; p = p substr($1,i,2) } print p; }'`
    echo "Key: $KEY"
fi


# See if user is already in file
grep "$USER Cleartext-Password" $userspath$usersfile > /dev/null
if [ $? -eq 0 ]; then
   # User already in file, Remove the existing entry.
    echo "Updating $USER in users file"
    # Use awk to delete the line above and line below the matchine username
    awk '/'"$USER"' Cleartext-Password/{c=1;next} c--<0 && p {print p} {p=$0} END{ if (c--<0 && p ){print p} }' $userspath$usersfile > /tmp/$usersfile
    mv /tmp/$usersfile $userspath$usersfile
echo

else
    # User not in file, add entry.
    echo "Adding $USER to users file"
fi

cat >> $userspath$usersfile << EOF

# Greylist time: $greylisttime
$LINE_START$USER Cleartext-Password := "$PASS"
$LINE_START      Vendor-Specific = "0x00006DE96412$KEY"
EOF

# Force radiusd to reload the configuration if the device was whitelisted
if [ $white -eq 1 ]; then
    killall -HUP radiusd
fi

