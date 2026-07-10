# Build rapida — Warzone 2100 Remasterd (macOS)

Cheat-sheet operativo per compilare e avviare la build su macOS.
Per il **perché** dietro ogni passo (exFAT, `._*`, SQLite, ordine risorse Ninja)
vedi [`AGENTS.md`](AGENTS.md) §1–§2.

Tutto ciò che serve è **nel repo**: lo script `build-mac.sh` è portabile
(ricava il sorgente dalla propria posizione) e, al primo giro, configura la
build da solo. Clona → `./build-mac.sh` → app.

## Doppio-click (come un .bat)

Nella root del repo ci sono due file cliccabili dal Finder (macOS li apre nel
Terminale):

- **`Compila e Avvia.command`** — compila la build e avvia il gioco.
- **`Avvia Gioco.command`** — avvia soltanto il gioco (nessuna ricompilazione).

> Se dopo un `git clone` il doppio-click non parte, ridai il permesso di
> esecuzione una volta: `chmod +x *.command build-mac.sh`.

## TL;DR — build di tutti i giorni (da terminale)

Dalla root del repo:

```sh
./build-mac.sh
```

Lo script: configura la build se serve, pulisce i `._*`, ricostruisce le risorse
nell'ordine giusto, compila l'eseguibile e copia le traduzioni nel bundle.

Poi avvia (il percorso esatto lo stampa lo script a fine build):

```sh
open "<build>/src/Warzone 2100.app"
```

> Se hai modificato asset in `data/base` (cursori, texpages, logo…) lo script
> se ne accorge (mtime) e forza la ricostruzione di `base.wz`/`mp.wz`.

## Dove finisce la build

| Cosa | Percorso |
|------|----------|
| Sorgente | root del repo (qui: `/Volumes/HD 1000 Gb/workspace-gaming/warzone2100`, exFAT) |
| Script build | `./build-mac.sh` (nel repo, versionato) |
| Build dir (default) | `<repo>/build` — già in `.gitignore` |
| Build dir (override) | variabile `WZ_BUILD_DIR=/percorso/build` |

**Su questa postazione** il sorgente è su exFAT (non regge la build), quindi la
build va su un'immagine APFS montata su `/Volumes/wzbuild`: lo script la rileva
in automatico se montata. Se non lo è (lo script scrive comunque dove sta
buildando, controlla la riga `Build:` a inizio output):

```sh
hdiutil attach "/Volumes/HD 1000 Gb/wzbuild.sparseimage"
```

## Cosa serve installato (una tantum)

Toolchain via Homebrew: `cmake`, `ninja` e le dipendenze del gioco. SQLite deve
venire da Homebrew (l'SDK Xcode non ha `sqlite3_load_extension`): lo script lo
trova da solo con `brew --prefix sqlite`. Il primo `./build-mac.sh` esegue il
`cmake` di configurazione; non serve lanciarlo a mano.

## Sotto il cofano (se devi fare a mano)

```sh
BUILD="${WZ_BUILD_DIR:-/Volumes/wzbuild/build}"
# 1. risorse PRIMA dell'eseguibile (obbligatorio su Ninja/macOS)
ninja -C "$BUILD" data/all doc/all po/all data/mods_staging/data_mods data_terrain_overrides_classic
# 2. eseguibile
ninja -C "$BUILD" warzone2100
# 3. traduzioni nel bundle (Ninja non le copia da solo)
rsync -a --exclude='._*' "$BUILD/po/locale/" \
  "$BUILD/src/Warzone 2100.app/Contents/Resources/locale/"
```

## Se qualcosa va storto

- **`error: null character ignored` / `._<file>.cpp` mancante:** un file
  AppleDouble `._*` è finito nel grafo di build durante un reconfigure. La glob
  in `src/CMakeLists.txt` ora li filtra, ma se un `build.ninja` vecchio li ha già
  "congelati":
  ```sh
  find . -name '._*' -not -path './.git/*' -delete
  cmake "${WZ_BUILD_DIR:-/Volumes/wzbuild/build}"     # riconfigura → build.ninja pulito
  grep -c '\._[^ ]*\.cpp\.o' "${WZ_BUILD_DIR:-/Volumes/wzbuild/build}/build.ninja"   # deve dare 0
  ```
  Un reconfigure **azzera** `po/locale` e la mod di campagna: dopo, rilancia
  `./build-mac.sh` (o almeno lo step risorse) prima di `ninja warzone2100`.
- **Italiano/traduzioni assenti nel menu:** `Contents/Resources/locale` vuoto →
  rilancia lo script (fa il passo `rsync` in automatico).
- **Video campagna "mancanti":** metti `sequences.wz` in
  `~/Library/Application Support/Warzone 2100 master/` (la pref dir dipende dalla
  versione della build: questa build è `master`).
