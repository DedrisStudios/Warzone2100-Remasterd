#!/bin/bash
# Doppio-click nel Finder: COMPILA la build e AVVIA il gioco.
# (Equivalente macOS di un .bat cliccabile.)

cd "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)" || exit 1

./build-mac.sh
rc=$?
if [ $rc -ne 0 ]; then
  echo ""
  echo "Build FALLITA (codice $rc). Leggi i messaggi qui sopra."
  read -n1 -s -r -p "Premi un tasto per chiudere..."
  exit $rc
fi

source "./wz-env.sh"
echo ""
echo "Avvio il gioco: $WZ_APP"
open "$WZ_APP"
