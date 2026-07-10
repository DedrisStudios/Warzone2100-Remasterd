# Warzone 2100 on iPad (Cordova / WKWebView)

This wraps the **Emscripten Web Edition** of Warzone 2100 in a thin native iOS
app (Apache Cordova) so it can be installed on an iPad. The game itself is the
exact same WebAssembly build that runs in the browser — this folder only holds
the iPad-specific bits and the scripts to regenerate everything from scratch.

Everything here is committed to the repo on purpose: the generated Cordova
project and the local build image are disposable; **this folder is the source of
truth for the iPad app.**

## Why a native app can't just load `app://`

The WZ WebAssembly build is a `-pthread` build: it needs **WASM threads**, which
require `SharedArrayBuffer`, which requires the page to be **cross-origin
isolated** (`COOP: same-origin` + `COEP: require-corp`).

WKWebView **never** makes the custom `app://` scheme cross-origin isolated, so
Cordova's default loading shows *"Unsupported Browser — Missing features:
Wasm:Threads"*. The fix (in [`App/ViewController.swift`](App/ViewController.swift))
is to run a tiny **embedded HTTP server on `http://localhost:8642`** (a secure
context) that serves the bundled `www/` with the isolation headers, and point the
WebView there instead. That's the whole trick.

## Contents

| File | Purpose |
| --- | --- |
| `App/ViewController.swift` | Embedded loopback HTTP server + loads `http://localhost:8642`. The key file. |
| `config.xml` | Cordova config: landscape, fullscreen, `allow-navigation` for localhost. |
| `setup-ipad.sh` | Regenerates the whole Cordova iOS project and applies the above. |
| `deploy.sh` | Build + install + launch on a USB-connected device (auto signing). |
| `serve.js` | Optional desktop test server (COOP/COEP) to try the build in a browser. |

The landing page (single **Play** button, no other chrome) lives in the shared
source at [`platforms/emscripten/shell.html`](../emscripten/shell.html) — see the
`#wz-ipad-minimal` style block — so it is produced by the normal web build.

## Prerequisites (build Mac)

- Node.js and Cordova CLI: `npm install -g cordova`
- Xcode + Command Line Tools, CocoaPods
- Emscripten **4.0.10** (the CI-pinned version) + a checkout of `vcpkg`
  — see [`platforms/emscripten/README-build.md`](../emscripten/README-build.md).
  On an exFAT drive, do the build inside an APFS disk image (macOS `._*`
  AppleDouble files break vcpkg/git otherwise).
- A paid-ish Apple Developer account signed into Xcode (Apple Development cert is
  enough for on-device sideloading).

## Build & deploy — from a clean machine

```sh
# 1. Build the Web Edition (Emscripten). Produces an install dir with
#    index.html, warzone2100.wasm, warzone2100.data, ...  (call it $WEB)
#    Follow platforms/emscripten/README-build.md.

# 2. Generate the Cordova iOS project + apply iPad customizations:
platforms/ipad/setup-ipad.sh ~/wz-ipad "$WEB"

# 3. Connect the iPad by USB, unlock it, then:
platforms/ipad/deploy.sh ~/wz-ipad
#    (optionally pass a UDID and/or a Team ID as 2nd/3rd args)
```

Then tap **Play**. First launch may need the developer trusted on the device
(Settings › General › VPN & Device Management).

## Updating the app after a source change

The iPad app bundles a *frozen snapshot* of the web build — it does not track the
source live. To push a source change (C++, data, translations, `shell.html`):

```sh
# rebuild the web target (see emscripten README), then re-sync + redeploy:
cp -R "$WEB/." ~/wz-ipad/platforms/ios/www/
platforms/ipad/deploy.sh ~/wz-ipad
```

## Gotchas

- **iPad must be unlocked** or launch fails with `FBSOpenApplicationErrorDomain 7 (Locked)`.
- Install the `.app` from `.../Build/Products/Debug-iphoneos/`, never the one in
  `.../Index.noindex/...` (that bundle is invalid — CoreDeviceError 3000).
- Service workers don't run in this WKWebView (no app-bound-domains config), so
  there's no stale-cache problem on device. In a desktop browser the SW *does*
  cache aggressively — unregister it to see changes.
- Public **App Store** distribution is blocked by the GPL-2.0 licence (the VLC
  problem). This flow is for on-device sideloading / development only.
