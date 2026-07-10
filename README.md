<h1 align="center">
  <img src="icons/warzone2100.large.png" alt="Warzone 2100">
  <br />
  Warzone 2100 Remasterd
</h1>

<p align="center"><b>Un progetto DedrisStudios</b> — remaster non ufficiale di <a href="https://wz2100.net/">Warzone 2100</a>, lo storico RTS 3D open source del 1999.</p>

---

**Warzone 2100 Remasterd** è un fork di [Warzone 2100](https://github.com/Warzone2100/warzone2100) che ammoderna il gioco originale mantenendone intatti gameplay e bilanciamento: asset grafici rimasterizzati in alta definizione, traduzione italiana completa, controlli moderni e una build web pensata anche per iPad.

Il gioco resta **100% libero e gratuito**, con licenza GPL-2.0-or-later come l'originale.

# Cosa cambia rispetto all'originale

### Grafica
- **23 cursori di gioco rimasterizzati** in alta definizione, ridisegnati a 32px nativi con hotspot allineati agli originali
- **Bersagli nemici evidenziati**: parentesi angolari rosse attorno a unità, strutture nemiche e feature distruttibili visibili
- **Menu principale personalizzato**: pannello pulsanti trasparente, link esterni rimossi

### Lingua
- **Traduzione italiana completa al 100%**: interfaccia, messaggi e guida in-gioco

### Controlli moderni
| Tasto | Funzione |
|---|---|
| `W` `A` `S` `D` | Movimento camera (in alternativa alle frecce) |
| `Q` / `E` | Rotazione camera destra / sinistra |
| `F10` | Salvataggio rapido |
| `F7` | Screenshot |
| Tasto destro | Ordini alle unità (attivo di default) |

Il menu ordini dell'unità si apre automaticamente quando selezioni un'unità, senza dover passare dal pannello.

I comandi spostati per fare spazio ai nuovi binding: *Stop* su `K`, *Unità non assegnate* su `J`, *Rispondi al fuoco* su `V`, *Pattuglia* su `X`.

### Build web e iPad
- Build WebAssembly (Emscripten) con schermata di avvio minimale, pensata per il deploy su iPad tramite Cordova
- Fix alla toolchain: build funzionante anche da directory con spazi nel percorso

# Remaster in corso

Il lavoro sugli asset continua. In pipeline:

- **Sfondi dei menu** rimasterizzati in HD
- **Texture di gioco in HD**: upscale degli atlas a 2048² con generazione di normal e specular map (le 7 pagine principali coprono da sole l'81% dei modelli 3D)
- **Cinematiche della campagna** rimasterizzate

La geometria dei modelli non viene toccata: si ammodernano solo le texture, così hitbox, UV e gameplay restano identici all'originale.

# Compilare dai sorgenti

Il progetto si compila come l'upstream, con CMake:

```shell
git clone --recurse-submodules https://github.com/DedrisStudios/Warzone2100-Remasterd.git
```

Le istruzioni complete per Linux, Windows e macOS sono nella sezione [How to build](https://github.com/Warzone2100/warzone2100#how-to-build) del progetto originale. Le note operative specifiche di questo repo (build su macOS, keybinding, mappa degli asset) sono in [AGENTS.md](AGENTS.md).

I video della campagna si scaricano a parte, come per l'originale, da [wz-sequences](https://github.com/Warzone2100/wz-sequences/releases).

# Origini e crediti

Warzone 2100 è stato sviluppato da **Pumpkin Studios** e pubblicato da Eidos Interactive nel 1999. Nel 2004 il codice sorgente è stato rilasciato sotto licenza GNU GPL e da allora il gioco è mantenuto e migliorato dalla community del [Warzone 2100 Project](https://wz2100.net/), su cui questo fork si basa.

- Gioco originale e sviluppo continuativo: [Warzone 2100 Project](https://github.com/Warzone2100/warzone2100)
- Remaster: **DedrisStudios**

# Licenza

Warzone 2100 Remasterd è distribuito sotto licenza [GPL-2.0-or-later](COPYING). Per i dettagli su dati, mod e componenti di terze parti vedi [COPYING.README](COPYING.README) e [COPYING.NONGPL](COPYING.NONGPL).
