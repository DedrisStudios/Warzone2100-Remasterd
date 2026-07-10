#!/bin/bash
# Doppio-click nel Finder: AVVIA soltanto il gioco (senza ricompilare).

cd "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)" || exit 1

source "./wz-env.sh"

if [ ! -d "$WZ_APP" ]; then
  echo "App non trovata: $WZ_APP"
  echo "Esegui prima \"Compila e Avvia\" (o ./build-mac.sh) per crearla."
  read -n1 -s -r -p "Premi un tasto per chiudere..."
  exit 1
fi

echo "Avvio il gioco: $WZ_APP"
open "$WZ_APP"
