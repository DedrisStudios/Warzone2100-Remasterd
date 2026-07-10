#!/bin/bash
#
# Regenerate the Warzone 2100 iPad (Cordova/WKWebView) project from scratch and
# apply the iPad-specific customizations kept in this folder.
#
# Everything the iPad app needs beyond the shared source tree lives here:
#   config.xml            - Cordova config (orientation, allow-navigation localhost)
#   App/ViewController.swift - embedded loopback HTTP server + loads http://localhost
#                              (this is what makes WASM threads work in WKWebView)
#
# Prerequisites on the build Mac:
#   - Node.js + `cordova` CLI  (npm i -g cordova)
#   - Xcode + command line tools, CocoaPods
#   - A finished Emscripten web build (see platforms/emscripten/README-build.md)
#
# Usage:
#   platforms/ipad/setup-ipad.sh <dest-dir> <web-install-dir>
#     <dest-dir>          where to create the Cordova project (e.g. ~/wz-ipad)
#     <web-install-dir>   the Emscripten install output containing index.html,
#                         warzone2100.wasm, warzone2100.data, etc.
#
# Afterwards deploy with:  platforms/ipad/deploy.sh <dest-dir> [device-udid]
#
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
DEST="${1:?usage: setup-ipad.sh <dest-dir> <web-install-dir>}"
WEB="${2:?usage: setup-ipad.sh <dest-dir> <web-install-dir>}"
APPID="com.dedrisproject.wz2100"
APPNAME="Warzone 2100"

[ -f "$WEB/index.html" ] || { echo "ERROR: no index.html in web-install-dir '$WEB' (build the web target first)"; exit 1; }

echo "==> cordova create $DEST"
cordova create "$DEST" "$APPID" "$APPNAME"

echo "==> apply our config.xml"
cp "$HERE/config.xml" "$DEST/config.xml"

echo "==> cordova platform add ios"
( cd "$DEST" && cordova platform add ios )

echo "==> install custom ViewController.swift (embedded loopback server)"
cp "$HERE/App/ViewController.swift" "$DEST/platforms/ios/App/ViewController.swift"

echo "==> app icon (WARZONE 2100 REFORGED)"
cp "$HERE/App/icon-1024.png" \
   "$DEST/platforms/ios/App/Assets.xcassets/AppIcon.appiconset/icon.png"

echo "==> ATS: allow plain-HTTP to localhost"
plutil -insert NSAppTransportSecurity -json '{"NSAllowsLocalNetworking": true}' \
    "$DEST/platforms/ios/App/App-Info.plist" 2>/dev/null || \
    echo "   (NSAppTransportSecurity already present - skipped)"

echo "==> copy web build into www (project + platform)"
mkdir -p "$DEST/www" "$DEST/platforms/ios/www"
rm -rf "${DEST:?}/www"/* "${DEST:?}/platforms/ios/www"/*
cp -R "$WEB/." "$DEST/www/"
cp -R "$WEB/." "$DEST/platforms/ios/www/"

echo ""
echo "==> DONE. Cordova iOS project ready at: $DEST"
echo "    Open in Xcode:  open \"$DEST/platforms/ios/App.xcworkspace\""
echo "    Or deploy:      $HERE/deploy.sh \"$DEST\""
