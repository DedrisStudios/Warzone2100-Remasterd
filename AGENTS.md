# AGENTS.md — Warzone 2100 Remasterd (DedrisStudios)

Istruzioni operative per agenti che lavorano su questo repo. Nate da errori realmente commessi: leggile prima di build, commit o modifiche ai keybinding.

## 1. Filesystem: il sorgente sta su exFAT → file spazzatura `._*`

Il repo vive su un volume **exFAT** (`/Volumes/HD 1000 Gb/...`) che non supporta gli xattr macOS: ogni scrittura genera un file gemello `._nome`. Questi rompono la build:
- vengono raccolti dal glob dei sorgenti CMake → `error: null character ignored [-Werror,-Wnull-character]` (es. `src/input/._keyconfig.cpp`);
- finiscono dentro i pacchetti `.wz` (voci morte).

**Regola:** prima di ogni build esegui `find . -name '._*' -not -path './.git/*' -delete`. Lo script di build lo fa già (vedi §2). I `._*.o` dentro la build dir sono innocui.

**Difesa a livello CMake (già in place):** `src/CMakeLists.txt` filtra i `._*` subito dopo la glob dei sorgenti (`list(FILTER SRC/HEADERS EXCLUDE REGEX "/\._[^/]*$")`), così nemmeno un reconfigure può reintrodurli nel grafo di build. Non rimuovere quel filtro. Se un `build.ninja` vecchio li aveva già "congelati" (errore `._<file>.cpp ... missing`), forza un reconfigure: `cmake /Volumes/wzbuild/build` e verifica con `grep -c '\._[^ ]*\.cpp\.o' /Volumes/wzbuild/build/build.ninja` (deve dare `0`).

## 2. Build: sorgente su exFAT, BUILD su immagine APFS

> Cheat-sheet dei comandi (build quotidiana, prima configurazione, fix rapidi): [`BUILD-RAPIDA.md`](BUILD-RAPIDA.md). Questa sezione spiega il *perché*.

- Sorgente: la root del repo (qui è su exFAT, `/Volumes/HD 1000 Gb/workspace-gaming/warzone2100`).
- Build: default `<repo>/build` (già in `.gitignore`); override con `WZ_BUILD_DIR`. Qui il sorgente è su exFAT (non regge la build) → si usa un'immagine APFS `wzbuild.sparseimage` montata su **`/Volumes/wzbuild`**: lo script la rileva da solo se montata, altrimenti monta con `hdiutil attach "/Volumes/HD 1000 Gb/wzbuild.sparseimage"`.
- **Usa sempre lo script in-repo** `./build-mac.sh` (portabile, versionato): al primo giro configura la build (Ninja, Release, SQLite via Homebrew), pulisce i `._`, compila le risorse PRIMA dell'eseguibile (obbligatorio, vedi sotto), poi copia le traduzioni nel bundle. NON dipendere da script esterni al repo: un deploy che ripulisce il workspace li cancella.
- **Launcher cliccabili (doppio-click nel Finder)**: `Compila e Avvia.command` (build + avvio) e `Avvia Gioco.command` (solo avvio). Insieme a `build-mac.sh` condividono `wz-env.sh` (risolve `WZ_SRC`/`WZ_BUILD`/`WZ_APP` e monta l'immagine APFS se serve). Non duplicare la logica dei path: modifica `wz-env.sh`.
- Avvio: `open "<build>/src/Warzone 2100.app"` (qui `<build>` = `/Volumes/wzbuild/build`).

Dettagli che mordono:
- **SQLite**: deve puntare a Homebrew, non all'SDK Xcode (che non ha `sqlite3_load_extension`). Configure con `-DSQLite3_INCLUDE_DIR=/opt/homebrew/opt/sqlite/include -DSQLite3_LIBRARY=/opt/homebrew/opt/sqlite/lib/libsqlite3.dylib`.
- **Ninja su macOS** non è pienamente supportato: le copie-risorse del bundle referenziano path relativi mentre i produttori scrivono path assoluti → `ninja: error: '<x>' ... missing and no known rule`. Rimedio: costruire prima i target risorse così i file esistono fisicamente, poi l'eseguibile — `ninja data/all doc/all po/all data/mods_staging/data_mods data_terrain_overrides_classic` poi `ninja warzone2100`.
- **`locale` non finisce nel bundle** (`Contents/Resources/locale` resta vuoto → italiano assente). Rimedio: `rsync -a --exclude='._*' <build>/po/locale/ "<app>/Contents/Resources/locale/"` (lo script lo fa).

## 3. Branch: esiste SOLO `master`

**Usiamo esclusivamente il branch `master`. Non esistono altri branch** (niente `main`, niente branch di feature): tutto il lavoro — feature, fix, commit — va su `master`. Non creare, non cambiare checkout, non pushare altri branch.

## 4. Commit/push: MAI riferimenti a Claude/Anthropic

Su questo repo i messaggi di commit non devono contenere `Claude`, `Anthropic`, né trailer `Co-Authored-By`. Push delle feature va su **`dedris`** (il fork), NON su `origin` (upstream ufficiale Warzone2100).

## 5. Changelog: OGNI modifica va registrata in `CHANGELOG.md`

Nella root del repo c'è `CHANGELOG.md`: va tenuto aggiornato con **tutte** le modifiche fatte, nessuna esclusa. Regole:

- **Aggiorna il changelog nello stesso commit della modifica** (o comunque prima del push): un push senza la voce corrispondente nel changelog è un errore.
- Voce sotto la data odierna (`## AAAA-MM-GG`, data più recente in alto), nella categoria giusta: **Grafica**, **Gameplay/UX**, **Controlli**, **Traduzione**, **Build/Web**, **Documentazione**.
- In italiano, una riga per modifica, descrivendo l'effetto per il giocatore/utente (non il dettaglio implementativo). Come per i commit (§4): niente riferimenti a Claude/Anthropic.
- Vanno registrate anche le modifiche solo locali non versionabili (es. asset del submodule texpages), marcate con `(locale, non nel repo)`.

## 6. Keybinding: i default non bastano, c'è `keymap.json`

I default dei tasti sono in `src/input/keyconfig.cpp`. A runtime vengono sovrascritti da `~/Library/Application Support/Warzone 2100 <ver>/keymap.json`. Dopo aver cambiato un default, il vecchio `keymap.json` continua a vincere → la modifica non si vede. Rimedio: rigenerare `keymap.json` (rimuoverlo se non ci sono rebind utente `source:"user"`) o farlo resettare dal menu Opzioni.

## 7. Cartella utente dipende dalla versione della build

La pref dir è `~/Library/Application Support/Warzone 2100 <versione>`. La build `wzbuild` riporta versione `master` → usa **"Warzone 2100 master"**. Metti lì `config` (`language=it`), `keymap.json` e `sequences.wz` (video campagna, download separato ~964MB). Attenzione a non confonderla con "Warzone 2100 main" di build precedenti.

## 8. Asset 3D e ammodernamento texture (progetto "remaster HD")

Mappa di dove vivono gli asset 3D e cosa conviene toccare con l'AI. **Regola d'oro: si ammodernano le TEXTURE, non la geometria.**

**Dove stanno:**
- **Geometria** = 1049 file `.pie` (testo, low-poly, vertici scritti a mano). `data/base/components/` (bodies/prop/weapons, 247), `structs/` (edifici, 164), `features/` (104), `effects/` (130). Un `.pie` referenzia UNA texture page e, opzionalmente, `NORMALMAP`/`SPECULARMAP`.
- **Texture** = 114 PNG in `data/base/texpages/`. Sono **atlas**: decine di modelli condividono una pagina, con UV normalizzate 0–1.

**Perché l'upscale è sicuro (UV-safe):** essendo atlas con UV normalizzate, un upscale pulito 2×/4× dell'intera pagina **non richiede alcuna modifica a geometria, UV, hitbox o `.pie`**. Zero rischio gameplay. NON rigenerare la geometria con AI image-to-3D: cambierebbe formato/UV/silhouette → progetto di re-modeling, fuori scope.

**Priorità (7 texture = 81% dei modelli).** I riferimenti sono concentratissimi. Fai queste in ordine:
1. `page-17-droid-weapons.png` (1024², **252** PIE — tutte le armi/torrette)
2. `page-12-player-buildings.png` (1024², 136 — edifici)
3. `page-16-droid-drives.png` (1024², 111 — corpi/cingoli)
4. `page-14-droid-hubs.png` (1024², 45) · 5. `page-10-laboratories.png` (1024², 29) · 6. `page-34-buildings.png` (già **2048²**, 27) · 7. `page-6.png` (1024², 22)

Poi ~20 pagine Tier 2 (fazioni Nexus/Collective/scavenger, 4–13 PIE l'una) e coda lunga di 17 pagine minori. Le 9 pagine `*-fx*`/`sky` (particellari) e i placeholder hanno ROI basso: lasciare per ultime.

**Vincoli che mordono:**
- **PBR quasi assente sulle unità.** Nessun modello oggetto ha normal/specular veri: esistono solo placeholder `znull_norm.png`/`znull_spec.png` da **1×1 pixel**. Il motore però legge già `NORMALMAP`/`SPECULARMAP` per-pagina. Il precedente pronto è `data/terrain_overrides/high/` (terreno HD con suffissi `_nm`/`_sm`): 29/41 texture di terreno sono **già** rifatte lì → NON rifarle. Generare normal+spec con AI e agganciarli ai `.pie` è il salto visivo maggiore.
- **Team-color a coppie.** 20 pagine oggetto hanno una `_tcmask` gemella (abbinata per numero: `page-12-player-buildings.png` ↔ `page-12_tcmask.png`). Vanno scalate **insieme e alla STESSA risoluzione**. ⚠️ La `_tcmask` è una MASCHERA, non una texture: scalarla nearest/edge-aware, **mai** con upscaler generativo (altrimenti i colori squadra sbavano). Incongruenza preesistente da sistemare: `page-10` e `page-123` hanno base 1024 ma mask 256.
- **Target risoluzione:** 2048² per gli oggetti (allineato a `page-34` e al terreno high). Verificare il cap texture del motore prima di spingersi a 4096.

Pipeline AI di riferimento: la stessa già usata per cursori e frame cinematici (Gemini/Cloudflare) — vedi memoria `cloudflare-gemini-image-remaster` e `cursor-remaster`. Prototipo consigliato: le 3 texture top → upscale + normal/spec → aggancio `.pie` → verifica in-game, poi batch.

## 9. Mappa del motore: zona ROSSA (determinismo) e zona VERDE (presentazione)

Il gioco è una **simulazione lockstep deterministica a 10 Hz** (`GAME_TICKS_PER_UPDATE=100`, `lib/gamelib/gtime.h`): tutti i client eseguono la stessa simulazione, in rete viaggiano solo gli ordini. Ne segue una divisione netta:

**Zona ROSSA — non toccare senza sapere cosa si fa (rischio desync/replay rotti):**
- `gameStateUpdate()` in `src/loop.cpp:515` e il suo **ordine di fasi** (script → grid → visibilità → path → power → droid → structure → proiettili → objmem). Riordinare o saltare condizionalmente una fase = desync.
- Matematica **fixed-point/intera**: `lib/framework/fixedpoint.h` (angoli uint16, `DEG_360=65536`), `trig.h` (`iSin/iCos/iSqrt/iAtan2`). Nella simulazione MAI `float`/`double`, MAI `<cmath>`.
- RNG condiviso: solo `gameRand*` (`src/random.h`). MAI `rand()`, `std::random_device`, seed basati sul tempo.
- Formato wire dei messaggi `GAME_*` (`lib/netplay/netplay.h`, serializzazione in `nettypes.cpp`) e ordine di iterazione/rimozione delle liste oggetti (`objmem.cpp`).
- Contratti pubblici: API scripting `wzapi` (`src/wzapi.h`, usata da campagna QuickJS e AI), scala versioni savegame (`src/game.cpp`, attuale `VERSION_39`), costanti `MAX_PLAYERS=11` (definita in **3 posti**: `lib/framework/frame.h` + due copie in `lib/wzmaplib`), mappa max 256×256, `TILE_UNITS=128`.
- Ogni modifica in zona rossa va accompagnata da marcatori `syncDebug(...)` come il codice circostante.

**Zona VERDE — libera per il remaster (gira fuori dal tick):** `lib/ivis_opengl` (renderer), `lib/widget` (GUI), `src/display*.cpp`, `titleui/`, `screens/`, `hci/`, audio (`lib/sound`), video (`lib/sequence`), shader (`data/base/shaders/`).

Nota rete: essendo un branch non-master, il fork riceve automaticamente `NETCODE_VERSION 0x1000/1` (`lib/netplay/autorevision_netplay.cmake`) → non può connettersi a partite upstream. Isolamento già garantito, nessuna azione necessaria.

## 10. Limiti hardcoded che il remaster incontra (file:riga)

| Limite | Valore | Dove |
|---|---|---|
| Risoluzione tile terreno | max **512** | `MIPMAP_MAX`, `src/texture.cpp:48` (selezione per nome cartella `tertilescXhw-<res>`, vedi memoria) |
| Texture size utente (terreno) | default **2048** | `src/texture.cpp:59`; tile 512 richiedono texture GPU 8192 (packing 16×16) |
| Vertici per modello `.pie` | **65.535** (indici u16) | `lib/ivis_opengl/imdload.cpp:55`; poligoni max 8192 (`:58`) — alzare = migrare a u32 in `gfx_api`/`piedraw` |
| Video cinematici | **1024×1024** | `lib/sequence/sequence.cpp:377-378` (Theora+Vorbis; `sequences.wz` monta sopra base) |
| Luci puntuali dinamiche | **128** (bucket 8×8) | `gfx_api.h:751-753`; **spente di default** (`WZ_POINT_LIGHT_ENABLED=0`) |
| Ombre | 3 cascate CSM, depth map 2048 | `lib/ivis_opengl/shadows.h:25` (3 hardcoded anche negli shader) |
| Anti-aliasing | solo MSAA, cap 16× | nessun FXAA/TAA/bloom/tonemap |
| Cursori da file | **23** (5 restano hardcoded nel sorgente) | `lib/sdl/cursors_sdl.cpp` — freccia, mirino, target, bordo-mappa, menu sono XPM embedded; hotspot dei PNG auto-centrato (`:1359`) |

**Post-processing: non esiste ma il gancio c'è.** La scena 3D va su un framebuffer offscreen e viene copiata a schermo da `data/base/shaders/world_to_screen.frag` (copia pura, con uniform `gamma` mai usato). È il punto d'inserimento naturale per tonemap/bloom/FXAA.

**Terreno HD:** la qualità `NORMAL_MAPPING` (`src/terrain_defs.h`) usa `terrain_overrides/high` con `_nm`/`_sm`/`_hm` auto-rilevati — il precedente da imitare per il PBR delle unità (§8).

## 11. Override asset senza toccare `data/base` (PhysFS)

Ordine di mount PhysFS (`src/init.cpp:569`): **maps > mods > base > base.wz**, e la dir utente vince su tutto (`PHYSFS_PREPEND`, `init.cpp:768`). Quindi un pacchetto in `mods/autoload/<nome>.wz` (zip **non compresso**, `COMPRESSION_LEVEL 0`, come base.wz) che replica i path relativi (`texpages/...`, `images/intfac/...`) **shadowa base.wz senza modificarlo**. È la via preferita per distribuire il remaster: aggiornamenti dall'upstream quasi indolori, pacchetto versionabile e rimovibile. Musica → `mods/music/<album>/album.json`; campagne alternative → `mods/campaign/<nome>/mod-info.json` (modello: mod Reclamation). I `.wz` sono zip normali: cartella estratta e archivio si comportano identici.

## 12. Rebranding: NON ancora fatto, e non esiste una variabile unica

La build produce ancora "Warzone 2100" con bundle id `net.wz2100.Warzone2100`. Quando si deciderà il rebrand definitivo, i punti da toccare: `src/CMakeLists.txt:452-455` (OUTPUT_NAME + bundle id macOS), `platforms/macos/Resources/Warzone.icns` + `Warzone-Info.plist.in`, `icons/`, asset PWA in `platforms/emscripten/assets/` + `manifest.json` + `shell.html`, `pkg/appstream/net.wz2100.*`, e il flag CMake `WZ_DISTRIBUTOR`. La cartella `dedrisremasterd/` non è collegata alla build (contiene solo arte).

## 13. Dipendenze e piattaforme: appunti che mordono

- Il progetto usa **SDL3** (non SDL2: `lib/sdl/CMakeLists.txt` richiede SDL3 ≥3.2.12; i riferimenti "SDL2" nei commenti sono stale). Dipendenze via vcpkg manifest (`vcpkg.json`); su macOS solo il generatore **Xcode** è pienamente supportato dal progetto (la nostra build Ninja funziona con gli accorgimenti di §2).
- **macOS/Metal:** nessun renderer Metal nativo — solo Vulkan via MoltenVK (bloccato sui Mac Intel → fallback OpenGL). Le feature grafiche vanno sempre verificate anche sul path OpenGL/GLES: è quello di Emscripten (WebGL2) e dei Mac Intel.
- **Emscripten/iPad:** pthreads attivi (pool 8), memoria iniziale 256 MB con growth e backoff post-OOM, persistenza IDBFS. `PROXY_TO_PTHREAD` e `OFFSCREENCANVAS_SUPPORT` sono **disattivati di proposito** (freeze Firefox, flicker Safari): non riabilitarli. Il quoting dei path nei `--preload-file` di `src/CMakeLists.txt` è necessario perché il disco ha spazi nel nome.
