#!/bin/bash
#
# Build, install and launch the Warzone 2100 iPad app on a USB-connected device,
# using Xcode automatic signing.
#
# Usage:
#   platforms/ipad/deploy.sh <cordova-project-dir> [device-udid] [development-team]
#     <cordova-project-dir>  the project created by setup-ipad.sh
#     [device-udid]          hardware UDID; auto-detected if omitted
#     [development-team]     Apple Team ID for signing; omit to use whatever the
#                            project already has (Xcode auto-picks a valid one)
#
# Notes:
#   - The iPad MUST be unlocked, or the launch step fails ("Locked").
#   - First run may require trusting the developer on the device:
#       Settings > General > VPN & Device Management.
#
set -uo pipefail
PROJ="${1:?usage: deploy.sh <cordova-project-dir> [udid] [team]}"
UDID="${2:-}"
TEAM="${3:-}"
APPNAME="Warzone 2100"
BUNDLE="com.dedrisproject.wz2100"
IOSDIR="$PROJ/platforms/ios"
DD="$IOSDIR/build"   # deterministic DerivedData location

cd "$IOSDIR" || exit 1

if [ -z "$UDID" ]; then
    UDID=$(xcrun xctrace list devices 2>/dev/null | grep -iE "ipad|iphone" \
        | grep -oE "[0-9A-Fa-f]{8}-[0-9A-Fa-f]{16}" | head -1)
fi
[ -z "$UDID" ] && { echo "ERROR: no connected device found (pass UDID as arg 2)"; exit 1; }
echo "==> device: $UDID"

TEAMARG=()
[ -n "$TEAM" ] && TEAMARG=(DEVELOPMENT_TEAM="$TEAM")

echo "==> build"
xcodebuild -workspace App.xcworkspace -scheme App -configuration Debug \
    -destination "platform=iOS,id=${UDID}" -derivedDataPath "$DD" \
    -allowProvisioningUpdates -allowProvisioningDeviceRegistration \
    "${TEAMARG[@]}" build || { echo "BUILD FAILED"; exit 1; }

APP="$DD/Build/Products/Debug-iphoneos/$APPNAME.app"
[ -d "$APP" ] || { echo "ERROR: built app not found at $APP"; exit 1; }
echo "==> app: $APP"

echo "==> install"
xcrun devicectl device install app --device "$UDID" "$APP" || { echo "INSTALL FAILED"; exit 1; }

echo "==> launch"
xcrun devicectl device process launch --device "$UDID" "$BUNDLE" \
    || echo "NOTE: launch failed (is the iPad unlocked? is the developer trusted?)"

echo "==> done"
