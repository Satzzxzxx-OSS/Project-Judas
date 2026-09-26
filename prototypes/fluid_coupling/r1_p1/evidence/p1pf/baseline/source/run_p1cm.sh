#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
mkdir -p evidence/p1cm/current
cmake -S . -B .build-jsgt > evidence/p1cm/current/build_output.txt 2>&1
cmake --build .build-jsgt -j2 >> evidence/p1cm/current/build_output.txt 2>&1
.build-jsgt/judas_r1_p1 > evidence/p1cm/current/run_output.txt 2>&1
python3 record_p1cm.py
