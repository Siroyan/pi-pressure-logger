#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/test.sh
git diff --check HEAD
pio_bin="${PIO:-$HOME/.platformio/penv/bin/pio}"
"$pio_bin" run -e validation -e offline
