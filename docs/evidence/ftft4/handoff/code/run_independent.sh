#!/usr/bin/env bash
set -euo pipefail
pack="$(cd "$(dirname "$0")/.." && pwd)"
work="${1:-$pack/.build-independent}"
mkdir -p "$work"
python3 - "$pack" "$work" <<'PY'
import pathlib,sys,zipfile
p=pathlib.Path(sys.argv[1]);w=pathlib.Path(sys.argv[2])
with zipfile.ZipFile(p/'reference/Judas_FTFT4_Contact_Research.zip') as z:z.extractall(w/'reference')
PY
python3 "$work/reference/code/verify_manifest.py"
python3 "$work/reference/code/run_all.py"
"${CXX:-g++}" -std=c++17 -O2 -fno-fast-math "$pack/code/source_target_probe.cpp" -o "$work/target"
"$work/target" | tee "$work/source_target_probe.csv"
