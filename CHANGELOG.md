# Changelog — Warzone 2100 Remasterd

Tutte le modifiche del progetto, dalla più recente. Le regole di aggiornamento sono in [AGENTS.md](AGENTS.md) §5.

## 2026-07-10

### Grafica
- Tutti i 23 cursori di gioco rimasterizzati in alta definizione (32px nativi, hotspot allineati agli originali)
- Sfondi dei menu (backdrop) e schermata crediti rimasterizzati in HD *(locale, non nel repo — asset del submodule texpages)*
- I bersagli nemici distruttibili (unità, strutture e feature) vengono evidenziati con parentesi angolari rosse
- Icona dell'app macOS aggiornata col logo Warzone 2100 Reforged
- Overlay scuro semitrasparente dietro i pulsanti del menu principale, per migliorare la leggibilità dei testi sopra lo sfondo
- Interfaccia di gioco (HUD) rimasterizzata: pulsanti del reticolo comandi, cornici, schede, barre di avanzamento e icone (costruzione, progetta, fabbrica, ricerca, mappa intel, annulla)
- Nuovo logo del menu principale

### Gameplay/UX
- Gli ordini col tasto destro sono attivi di default
- Il menu ordini dell'unità si apre automaticamente quando si seleziona un'unità propria

### Controlli
- Rotazione camera su `Q`/`E`; *Rispondi al fuoco* spostato su `V` e *Pattuglia* su `X`
- Salvataggio rapido spostato su `F10`, screenshot su `F7`
- `SPACE` non attiva più la camera di inseguimento: ora è scorciatoia secondaria per *Seleziona tutte le unità da combattimento*

### Build/Web
- Versione iPad: wrapper Cordova con server HTTP locale (per i thread WASM in WKWebView), script di setup/deploy e icona app
- Build web: schermata di avvio minimale con logo Reforged e tasti Play / Play again, pensata per il deploy su iPad
- La build Emscripten funziona anche da directory con spazi nel percorso
- iPad/touch: un solo tocco (tap) impartisce direttamente gli ordini di movimento/attacco (ordini col tasto destro disattivati nella build touch)
- iPad/touch: la build applica sempre i tasti di default aggiornati, ignorando eventuali keymap.json obsoleti (i pulsanti touch a schermo funzionano sempre)
- Script di build macOS portabile nel repo (`build-mac.sh` + `wz-env.sh`) con configurazione automatica al primo avvio, e launcher cliccabili dal Finder `Compila e Avvia.command` / `Avvia Gioco.command`
- Build affidabile su volumi exFAT: il glob dei sorgenti CMake ignora i file AppleDouble `._*`

### Documentazione
- Aggiunta la guida rapida alla build macOS (`BUILD-RAPIDA.md`); AGENTS.md aggiornato con lo script di build in-repo, i launcher cliccabili e il filtro dei file `._*`
- Aggiunto Bartolo Illiano (Software Engineer, project Reforge) alla sezione DedrisStudios del file AUTHORS
- Aggiunto CLAUDE.md che carica le istruzioni di AGENTS.md nelle sessioni Claude Code
- AGENTS.md ampliato con l'esito dell'audit del sorgente: mappa zone rosse/verdi del motore (§9), tabella dei limiti hardcoded (§10), override asset via PhysFS/mod (§11), punti da toccare per il rebranding (§12), appunti su SDL3/MoltenVK/Emscripten (§13)
- Riscritto il README come presentazione del progetto Remasterd
- Aggiunto AGENTS.md con le istruzioni operative del repo
- Aggiunto questo changelog e la regola di tenerlo aggiornato (AGENTS.md §5)

## 2026-07-09

### Controlli
- `W`/`A`/`S`/`D` come tasti alternativi per muovere la camera; *Stop* spostato su `K` e *Unità non assegnate* su `J`

### Documentazione
- Rimosse le immagini di copertina dal README e dal repo per possibili vincoli di copyright

### Alpha v1.00 — base del progetto
- Fork di Warzone 2100 (GPL-2.0-or-later)
- Menu principale personalizzato: link esterni rimossi, pannello pulsanti trasparente
- Traduzione italiana completa al 100% (interfaccia + guida in-gioco)
- Fix ARC per la build con Ninja su macOS (`cocoa_*.mm`)
