#!/bin/bash
# Build Warzone 2100 (DedrisStudios Remasterd) su macOS — script portabile.
#
# Vive DENTRO il repo e ricava la cartella sorgente dalla propria posizione,
# quindi funziona ovunque venga clonato il repo: clona → ./build-mac.sh → app.
#
# Cosa fa:
#   0. configura la build la prima volta (Ninja, Release, SQLite via Homebrew);
#   1. pulisce i file AppleDouble "._*" (li crea macOS sui volumi exFAT e rompono
#      la build: il glob CMake li raccoglie come sorgenti → errori sui byte nulli);
#   2. genera le risorse (.wz, docs, traduzioni, mod campagna, terrain) PRIMA
#      dell'eseguibile — obbligatorio col generatore Ninja su macOS;
#   3. compila l'eseguibile;
#   4. copia le traduzioni nel bundle (Ninja/macOS non le copia da solo).
#
# Cartella di build:
#   - default: <repo>/build (già in .gitignore);
#   - override con la variabile d'ambiente WZ_BUILD_DIR;
#   - se il sorgente è su un volume che non può ospitare la build (es. exFAT, che
#     genera i "._*" e non regge bene il bundle .app), tienila su un volume APFS.
#     Su questa postazione c'è un'immagine APFS montata su /Volumes/wzbuild: se
#     esiste, viene usata in automatico.
#
# Uso:
#   ./build-mac.sh                              # build (configura se serve)
#   WZ_BUILD_DIR=/percorso/build ./build-mac.sh # build dir esplicita

set -uo pipefail

# Variabili condivise SRC/BUILD/APP (incl. montaggio dell'immagine APFS). Vedi wz-env.sh.
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/wz-env.sh"
SRC="$WZ_SRC"
BUILD="$WZ_BUILD"
BUNDLE_RES="$WZ_APP/Contents/Resources"

echo "Sorgente: $SRC"
echo "Build:    $BUILD"

# ---------------------------------------------------------------------------
# [0/4] Primo configure (solo se la build dir non è ancora configurata).
# ---------------------------------------------------------------------------
if [ ! -f "$BUILD/build.ninja" ]; then
  echo "[0/4] Configuro la build (prima volta)..."
  # SQLite: l'SDK Xcode non ha sqlite3_load_extension → puntiamo a Homebrew.
  # brew --prefix funziona sia su Apple Silicon che Intel (niente path hardcodati).
  SQLITE_PREFIX="$(brew --prefix sqlite 2>/dev/null || true)"
  SQLITE_ARGS=()
  if [ -n "$SQLITE_PREFIX" ]; then
    SQLITE_ARGS=(-DSQLite3_INCLUDE_DIR="$SQLITE_PREFIX/include" -DSQLite3_LIBRARY="$SQLITE_PREFIX/lib/libsqlite3.dylib")
  fi
  find "$SRC" -name '._*' -not -path '*/.git/*' -delete 2>/dev/null
  cmake -G Ninja -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release "${SQLITE_ARGS[@]}" \
    || { echo "FALLITO: configure" >&2; exit 1; }
fi

echo "[1/4] Pulizia file AppleDouble ._* dai sorgenti (exFAT)..."
find "$SRC" -name '._*' -not -path '*/.git/*' -delete 2>/dev/null
echo "      ._ rimasti (fuori da .git/build): $(find "$SRC" -name '._*' -not -path '*/.git/*' -not -path '*/build/*' 2>/dev/null | wc -l | tr -d ' ')"

echo "[2/4] Genero le risorse (dati .wz, docs, traduzioni, mod campagna, terrain)..."
# Ninja NON traccia i singoli file dentro i .wz (usa un glob), quindi non ricostruisce
# base.wz/mp.wz quando modifichi asset (es. i PNG dei cursori in data/base/images/intfac).
# Forziamo il rebuild se un file sorgente è più recente del .wz pacchettizzato.
for pair in "base:$SRC/data/base" "mp:$SRC/data/mp"; do
  wzname="${pair%%:*}"; srcdir="${pair#*:}"; wzfile="$BUILD/data/$wzname.wz"
  if [ -f "$wzfile" ] && [ -d "$srcdir" ] && [ -n "$(find "$srcdir" -type f -not -name '._*' -newer "$wzfile" -print -quit 2>/dev/null)" ]; then
    echo "      (asset in $srcdir modificati → forzo la ricostruzione di $wzname.wz)"
    rm -f "$wzfile" "$BUILD/data/._$wzname.wz"
  fi
done
ninja -C "$BUILD" data/all doc/all po/all data/mods_staging/data_mods data_terrain_overrides_classic || { echo "FALLITO: risorse" >&2; exit 1; }

echo "[3/4] Compilo l'eseguibile..."
ninja -C "$BUILD" warzone2100 || { echo "FALLITO: eseguibile" >&2; exit 1; }

echo "[4/4] Copio le traduzioni nel bundle (workaround copia-directory di Ninja/macOS)..."
rsync -a --exclude='._*' "$BUILD/po/locale/" "$BUNDLE_RES/locale/"

echo ""
echo "OK. App aggiornata: $BUILD/src/Warzone 2100.app"
echo "Avvia con:  open \"$BUILD/src/Warzone 2100.app\""
