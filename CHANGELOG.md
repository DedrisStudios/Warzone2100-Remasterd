# Changelog — Warzone 2100 Remasterd

Tutte le modifiche del progetto, dalla più recente. Le regole di aggiornamento sono in [AGENTS.md](AGENTS.md) §5.

## 2026-07-11

### Grafica
- HUD: barra dell'energia abbassata di 10 px per un migliore allineamento in alto a destra
- HUD: pulsante impostazioni (gear) in alto a destra abbassato di qualche px per allinearlo alla barra dell'energia
- HUD: il messaggio «Energia insufficiente» sulle barre di produzione/costruzione/ricerca va ora su due righe, così non sborda più dal rettangolo

### Gameplay/UX
- Fabbrica: passando il mouse su un'unità costruibile, il suggerimento mostra ora quante se ne possono costruire con l'energia attuale (oltre al costo)

### Traduzione
- I nuovi testi «Energia insufficiente» (barre di produzione) e «Costruibili con l'energia attuale» (fabbrica) passano ora dal sistema di traduzione: italiano dal file `.po`, inglese come ripiego per le altre lingue

## 2026-07-10

### Grafica
- Redesign dell'interfaccia di gioco (HUD) in stile tattico «lit slab» 2026: i pannelli hanno corpo scuro quasi-nero con un rail verde luminoso sul bordo inferiore e staffe angolari a mirino
- HUD: barra dell'energia ridisegnata come strumento tattico — traccia scura, riempimento a gradiente verde→ciano con testa luminosa pulsante e tacche di scala al 25%
- HUD: le celle-pulsante (liste fabbrica/ricerca/progettazione, gruppi, stato) diventano celle tattiche scure con evidenziazione a underline verde al passaggio del mouse
- HUD: reticolo comandi ridisegnato — esagoni scuri tattici con icone comando **ridisegnate da zero** in stile moderno (martello=costruzione, fabbrica=produzione, atomo=ricerca, matita=progettazione, globo=intel, stella=comandanti, X=annulla), rail verde sui pulsanti attivi/premuti e anello verde al passaggio del mouse (disposizione a nido d'ape invariata)
- HUD: barra superiore (pulsante opzioni «gear», timer di missione e di rinforzi) ricolorata in stile scuro + verde coerente col resto dell'HUD (era blu-viola)
- HUD: contatori a schermo (unità perse/costruite/uccise, FPS) resi in stile «telemetria» — maiuscoletto verde tenue invece del bianco
- HUD: il radar (minimap) è incorniciato da staffe angolari a mirino verdi fisse, coerenti col motivo dei pannelli, per un aspetto da strumento tattico
- HUD: tab piccole e frecce di scorrimento dei pannelli ricolorate in verde (erano viola)
- Grafica di gioco: color-grade cinematografica leggera del campo di battaglia (tonemap filmico + vignettatura tenue) applicata alla copia scena→schermo su entrambi i backend (OpenGL e Vulkan), per un aspetto più moderno; la HUD resta nitida
- Tutti i 23 cursori di gioco rimasterizzati in alta definizione (32px nativi, hotspot allineati agli originali)
- Sfondi dei menu (backdrop) e schermata crediti rimasterizzati in HD *(locale, non nel repo — asset del submodule texpages)*
- Lo sfondo del menu non appare più schiacciato/deformato: viene disegnato nelle proporzioni corrette 16:9 (prima era mostrato stirato in un rapporto 4:3) e riempie tutta la finestra senza bande nere
- I bersagli nemici distruttibili (unità, strutture e feature) vengono evidenziati con parentesi angolari rosse
- Icona dell'app macOS aggiornata col logo Warzone 2100 Reforged
- Overlay scuro semitrasparente dietro i pulsanti del menu principale, per migliorare la leggibilità dei testi sopra lo sfondo
- Interfaccia di gioco (HUD) rimasterizzata in stile «vetro» verde (accento fazione Project): reticolo comandi (con stati normale/attivo/disabilitato/hover coerenti), barra energia, cornici finestra, schede, pulsante chiudi e slot dei menu costruzione/fabbrica/ricerca — icone originali ridisegnate mantenendone il significato
- Pannello ordini unità (attacca/mantieni/pattuglia/ritira…) e schermata di progettazione ricolorati nella stessa tonalità verde, preservando le icone originali e i colori semantici delle barre di confronto statistiche (blu/rosso/giallo)
- Menu di lancio (schermata iniziale e sottomenu) rimasterizzato nello stesso stile «vetro» verde dell'HUD: pannello dei pulsanti con riempimento scuro verdastro e bordo verde luminoso, voci di menu in verde (verde medio a riposo, più acceso al passaggio del mouse, verde-grigio spento quando non disponibili) al posto del vecchio lavanda, e banner laterale del titolo in verde
- Reskin verde esteso a tutte le schermate dei menu: Opzioni (tab selezionata, intestazioni di sezione, sotto-schede, frecce dei menu a tendina, slider, barra di scorrimento e freccia «indietro»), selettore delle campagne, lobby multigiocatore (pannello opzioni, chat, pulsanti) — eliminati blu e viola residui dell'interfaccia. La schermata dei punteggi mantiene il blu «costruite» per distinguerlo dal verde «distrutte»; i colori di squadra dei giocatori restano invariati
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
