#!/bin/bash
# Variabili condivise (WZ_SRC / WZ_BUILD / WZ_APP) usate da build-mac.sh e dai
# launcher .command. Va "sourced", non eseguito direttamente.
#
# WZ_BUILD (dove sta la build) si risolve così:
#   1. variabile d'ambiente WZ_BUILD_DIR, se impostata;
#   2. /Volumes/wzbuild/build, se quel volume APFS è montato (setup di questa
#      postazione: il sorgente è su exFAT e non regge la build);
#   3. <repo>/build (default standard, già in .gitignore).

WZ_SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Best-effort: se l'immagine APFS dedicata non è montata, montala.
# Su altre macchine i path non esistono → blocco no-op.
if [ ! -d "/Volumes/wzbuild" ] && [ -f "/Volumes/HD 1000 Gb/wzbuild.sparseimage" ]; then
  hdiutil attach "/Volumes/HD 1000 Gb/wzbuild.sparseimage" >/dev/null 2>&1 || true
fi

if [ -n "${WZ_BUILD_DIR:-}" ]; then
  WZ_BUILD="$WZ_BUILD_DIR"
elif [ -d "/Volumes/wzbuild" ]; then
  # Immagine APFS montata: la build vive qui (cmake crea la sotto-cartella).
  WZ_BUILD="/Volumes/wzbuild/build"
else
  WZ_BUILD="$WZ_SRC/build"
fi

WZ_APP="$WZ_BUILD/src/Warzone 2100.app"
