# Changelog — Warzone 2100 Remasterd

Tutte le modifiche del progetto, dalla più recente. Le regole di aggiornamento sono in [AGENTS.md](AGENTS.md) §5.

## 2026-07-10

### Grafica
- Tutti i 23 cursori di gioco rimasterizzati in alta definizione (32px nativi, hotspot allineati agli originali)
- Sfondi dei menu (backdrop) e schermata crediti rimasterizzati in HD *(locale, non nel repo — asset del submodule texpages)*
- I bersagli nemici distruttibili (unità, strutture e feature) vengono evidenziati con parentesi angolari rosse

### Gameplay/UX
- Gli ordini col tasto destro sono attivi di default
- Il menu ordini dell'unità si apre automaticamente quando si seleziona un'unità propria

### Controlli
- Rotazione camera su `Q`/`E`; *Rispondi al fuoco* spostato su `V` e *Pattuglia* su `X`
- Salvataggio rapido spostato su `F10`, screenshot su `F7`

### Build/Web
- Build web: schermata di avvio minimale (solo pulsante Play) per il deploy su iPad
- La build Emscripten funziona anche da directory con spazi nel percorso

### Documentazione
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
