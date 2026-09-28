#!/usr/bin/env bash
set -euo pipefail
# Does not modify engine source. Builds diagnostic executables outside its tree.
repo="$(realpath "${1:?Pass the Judas repository root}")"
pack="$(cd "$(dirname "$0")/.." && pwd)"
work="${2:-$pack/.build-source-probes}"
mkdir -p "$work"
flags=(-std=c++17 -O2 -fno-fast-math -Wall -Wextra -I"$repo/src")
# Optional non-system GLM include parent. No mock GLM is supplied.
if [[ -n "${GLM_INCLUDE_DIR:-}" ]]; then flags+=(-I"$GLM_INCLUDE_DIR"); fi
sources=(RigidBody RigidBodyGravity Contacts ContactSolver RadialTerrain Broadphase Narrowphase PhysicsWorld)
args=(); for s in "${sources[@]}"; do args+=("$repo/src/$s.cpp"); done
"${CXX:-g++}" "${flags[@]}" "$pack/code/world_probe.cpp" "${args[@]}" -o "$work/world_probe"
"${CXX:-g++}" "${flags[@]}" "$pack/code/tree_replay.cpp" "$repo/src/Broadphase.cpp" -o "$work/tree_replay"
"$work/world_probe" | tee "$work/world_probe.csv"
"$work/tree_replay" "$pack/evidence/tree_replay.tsv" | tee "$work/tree_replay.log"
