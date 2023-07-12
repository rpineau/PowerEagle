#!/bin/bash

PACKAGE_NAME="PowerEagle_X2.pkg"
BUNDLE_NAME="org.rti-zone.PowerEagleX2"

if [ ! -z "$app_id_signature" ]; then
    codesign -f -s "$app_id_signature" --verbose  ../build/Release/libPowerEagle.dylib
fi

mkdir -p ROOT/tmp/PowerEagle_X2/
cp "../PowerEagle.ui" ROOT/tmp/PowerEagle_X2/
cp "../focuserlist PowerEagle.txt" ROOT/tmp/PowerEagle_X2/
cp "../build/Release/libPowerEagle.dylib" ROOT/tmp/PowerEagle_X2/

f [ ! -z "$installer_signature" ]; then
	# signed package using env variable installer_signature
	pkgbuild --root ROOT --identifier $BUNDLE_NAME --sign "$installer_signature" --scripts Scripts --version 1.0 $PACKAGE_NAME
	pkgutil --check-signature ./${PACKAGE_NAME}

else
    pkgbuild --root ROOT --identifier $BUNDLE_NAME --scripts Scripts --version 1.0 $PACKAGE_NAME
fi

rm -rf ROOT
