#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/test.sh
git diff --check
pio_bin="${PIO:-$HOME/.platformio/penv/bin/pio}"
"$pio_bin" run -e validation
