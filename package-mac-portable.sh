#!/bin/bash
# Crea una copia PORTABLE dell'app macOS: copia le dylib Homebrew dentro
# Contents/Frameworks e riscrive i riferimenti (install_name_tool), così l'app
# gira anche su Mac SENZA Homebrew e senza l'immagine di build montata.
#
# Prerequisito: build già fatta con ./build-mac.sh (e binario SENZA segmento
# __RESTRICT, vedi src/CMakeLists.txt: con __RESTRICT dyld rifiuta i path @).
# Output:  <build-parent>/dist/Warzone 2100.app   (autonoma, ricollocabile)
#          + zip accanto (o nella dir passata come argomento), con hash git.
# Uso:     ./package-mac-portable.sh [dir-output-zip]
set -euo pipefail

source "$(dirname "$0")/wz-env.sh"

if [ ! -d "$WZ_APP" ]; then
  echo "ERRORE: app non trovata in $WZ_APP — esegui prima ./build-mac.sh" >&2
  exit 1
fi

DIST="${WZ_DIST_DIR:-$(dirname "$WZ_BUILD")/dist}"
APP="$DIST/Warzone 2100.app"
BIN="$APP/Contents/MacOS/Warzone 2100"
FW="$APP/Contents/Frameworks"

echo "== Copia del bundle in $DIST =="
rm -rf "$APP"
mkdir -p "$DIST"
ditto "$WZ_APP" "$APP"
mkdir -p "$FW"

# Dipendenze da bundlare di un binario Mach-O: righe otool che iniziano per
# tab e puntano a Homebrew oppure a @rpath/@loader_path (dylib Homebrew che
# si referenziano tra loro così, es. libprotobuf → @rpath/libutf8_validity).
# La riga con l'id della dylib stessa è innocua: ha lo stesso basename del
# file già copiato e viene saltata.
list_deps() {
  otool -L "$1" | awk '/^\t(\/opt\/homebrew\/|@rpath\/|@loader_path\/)/{print $1}'
}

# Risolve un riferimento nel path reale su disco.
#   $1 = riferimento (path o @rpath/...)   $2 = dir di origine del binario
#        che lo referenzia (per cercare le dylib "sorelle" dello stesso keg)
resolve_dep() {
  local ref="$1" origindir="$2" base
  case "$ref" in
    /opt/homebrew/*) echo "$ref" ;;
    @rpath/*|@loader_path/*)
      base="$(basename "$ref")"
      if [ -n "$origindir" ] && [ -f "$origindir/$base" ]; then
        echo "$origindir/$base"
      elif [ -f "/opt/homebrew/lib/$base" ]; then
        echo "/opt/homebrew/lib/$base"
      fi ;;
  esac
}

echo "== Raccolta ricorsiva delle dylib Homebrew =="
# Due code parallele: file da scansionare + dir di origine per risolvere @rpath.
queue=("$BIN"); originq=("")
while [ ${#queue[@]} -gt 0 ]; do
  cur="${queue[0]}";    queue=("${queue[@]:1}")
  curorig="${originq[0]}"; originq=("${originq[@]:1}")
  while IFS= read -r dep; do
    [ -z "$dep" ] && continue
    base="$(basename "$dep")"
    [ -f "$FW/$base" ] && continue
    src="$(resolve_dep "$dep" "$curorig")"
    if [ -z "$src" ] || [ ! -f "$src" ]; then
      echo "ERRORE: impossibile risolvere la dipendenza '$dep' (da $cur)" >&2
      exit 1
    fi
    cp -L "$src" "$FW/$base"   # cp -L segue i symlink (opt/ -> Cellar/)
    chmod u+w "$FW/$base"
    queue+=("$FW/$base"); originq+=("$(dirname "$src")")
    echo "  + $base"
  done < <(list_deps "$cur")
done

echo "== Riscrittura dei riferimenti =="
# Id delle dylib copiate → @loader_path (si trovano tutte nella stessa dir).
for lib in "$FW"/*.dylib; do
  install_name_tool -id "@loader_path/$(basename "$lib")" "$lib" 2>/dev/null
done
# Riferimenti Homebrew/@rpath → percorso relativo al bundle.
fix_refs() { # $1=binario  $2=prefisso sostitutivo
  local dep
  while IFS= read -r dep; do
    [ -z "$dep" ] && continue
    install_name_tool -change "$dep" "$2$(basename "$dep")" "$1" 2>/dev/null
  done < <(list_deps "$1")
}
fix_refs "$BIN" "@executable_path/../Frameworks/"
for lib in "$FW"/*.dylib; do
  fix_refs "$lib" "@loader_path/"
done

echo "== Verifica: nessun riferimento Homebrew residuo =="
leftover="$(otool -L "$BIN" "$FW"/*.dylib | grep '/opt/homebrew' || true)"
if [ -n "$leftover" ]; then
  echo "ERRORE: restano riferimenti a /opt/homebrew:" >&2
  echo "$leftover" >&2
  exit 1
fi

echo "== Firma ad-hoc (obbligatoria su Apple Silicon) =="
for lib in "$FW"/*.dylib; do
  codesign --force -s - "$lib" >/dev/null 2>&1
done
codesign --force -s - "$APP" >/dev/null 2>&1

echo "== Smoke test (--version) =="
"$BIN" --version

GITREV="$(git -C "$WZ_SRC" rev-parse --short HEAD 2>/dev/null || echo local)"
ZIPDIR="${1:-$DIST}"
mkdir -p "$ZIPDIR"
ZIP="$ZIPDIR/Warzone-2100-Remasterd-portable-macOS-$GITREV.zip"
echo "== Creazione zip: $ZIP =="
rm -f "$ZIP"
ditto -c -k --keepParent "$APP" "$ZIP"

echo ""
echo "OK. App portable: $APP"
echo "    Zip:          $ZIP"
echo "Avvia con:  open \"$APP\""
