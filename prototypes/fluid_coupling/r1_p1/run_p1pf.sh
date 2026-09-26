#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
export OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1
mkdir -p evidence/p1pf/current
# The original transport gate runs through its original target and script.
bash run_p1cm.sh
cmake -S . -B .build-p1pf -DJUDAS_BUILD_P1PF=ON > evidence/p1pf/current/build_output.txt 2>&1
cmake --build .build-p1pf --target judas_r1_p1pf -j2 >> evidence/p1pf/current/build_output.txt 2>&1
.build-p1pf/judas_r1_p1pf > evidence/p1pf/current/run_output.txt 2>&1
python3 record_p1pf.py
