# AGENTS.md — Warzone 2100 Remasterd (DedrisStudios)

Istruzioni operative per agenti che lavorano su questo repo. Nate da errori realmente commessi: leggile prima di build, commit o modifiche ai keybinding.

## 1. Filesystem: il sorgente sta su exFAT → file spazzatura `._*`

Il repo vive su un volume **exFAT** (`/Volumes/HD 1000 Gb/...`) che non supporta gli xattr macOS: ogni scrittura genera un file gemello `._nome`. Questi rompono la build:
- vengono raccolti dal glob dei sorgenti CMake → `error: null character ignored [-Werror,-Wnull-character]` (es. `src/input/._keyconfig.cpp`);
- finiscono dentro i pacchetti `.wz` (voci morte).

**Regola:** prima di ogni build esegui `find . -name '._*' -not -path './.git/*' -delete`. Lo script di build lo fa già (vedi §2). I `._*.o` dentro la build dir sono innocui.

## 2. Build: sorgente su exFAT, BUILD su immagine APFS

- Sorgente: `/Volumes/HD 1000 Gb/workspace-gaming/warzone2100` (exFAT).
- Build: immagine APFS `wzbuild.sparseimage` montata su **`/Volumes/wzbuild`**, build dir `/Volumes/wzbuild/build`.
- **Usa sempre lo script** `/Volumes/HD 1000 Gb/workspace-gaming/build-wz.sh`: pulisce i `._`, compila le risorse PRIMA dell'eseguibile (obbligatorio, vedi sotto), poi copia le traduzioni nel bundle.
- Avvio: `open "/Volumes/wzbuild/build/src/Warzone 2100.app"`.

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
