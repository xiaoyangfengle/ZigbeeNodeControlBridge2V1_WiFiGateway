 #!/bin/sh
APP_NOTE_NAME=JN-AN-1194-Zigbee-Gateway

BASE=$(cd ../../..; pwd)
ZIP=$BASE/Tools/7Zip/7z.exe
EXPORT_PATH=SDK/$APP_NOTE_NAME

make -C ZigbeeNodeControlBridge/ TRACE=1 PDM_BUILD_TYPE=_EEPROM clean
make -C ZigbeeNodeControlBridge/ TRACE=1 PDM_BUILD_TYPE=_EEPROM

mkdir -p SDK

svn export --depth=files ../ $EXPORT_PATH 
mkdir $EXPORT_PATH/Source
svn export ../Source/Common $EXPORT_PATH/Source/Common/
svn export ../Source/ZigbeeNodeControlBridge $EXPORT_PATH/Source/ZigbeeNodeControlBridge 
mkdir $EXPORT_PATH/Build
mkdir $EXPORT_PATH/Build/ZigbeeNodeControlBridge

chmod 777 ZigbeeNodeControlBridge/Makefile
chmod 777 ZigbeeNodeControlBridge/APP_stack_size.ld
cp ZigbeeNodeControlBridge/*.bin $EXPORT_PATH/Build/ZigbeeNodeControlBridge/
cp ZigbeeNodeControlBridge/Makefile $EXPORT_PATH/Build/ZigbeeNodeControlBridge/Makefile
cp ZigbeeNodeControlBridge/APP_stack_size.ld $EXPORT_PATH/Build/ZigbeeNodeControlBridge/APP_stack_size.ld
rm -f $APP_NOTE_NAME.zip
cd SDK
$ZIP a -r ../$APP_NOTE_NAME.zip * 
cd ..
rm -rf SDK