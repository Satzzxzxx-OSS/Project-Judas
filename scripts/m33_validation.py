#!/usr/bin/env python3
"""M33 candidate: one clean Release build and existing production/editor gate.

Only command/output adapters: explicit Release and persistent M33 test output.
All fixtures, assertions, real GL paths and FTFT acceptance policies are reused.
"""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import time
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/evidence/m33/final'

def main():
    assert not OUT.exists(), 'Final gate already exists; do not rerun blindly'
    backup = ROOT / '.cache/m33-before-clean-release'
    assert not backup.exists(), 'Prior build backup already exists'
    backup.parent.mkdir(parents=True, exist_ok=True)
    if (ROOT / 'build').exists(): (ROOT / 'build').rename(backup)
    assert not (ROOT / 'build').exists()
    spec = importlib.util.spec_from_file_location('m33_existing_gate', ROOT / 'scripts/ftft9_validation.py')
    gate = importlib.util.module_from_spec(spec); spec.loader.exec_module(gate)
    gate.OUTPUT_TESTS.add('judas_render_camera_tests')
    original = subprocess.run
    commands = []
    def release(command, *args, **kwargs):
        if command == ['cmake', '-S', '.', '-B', 'build']:
            command.append('-DCMAKE_BUILD_TYPE=Release'); commands.append(list(command))
        return original(command, *args, **kwargs)
    start = time.monotonic(); subprocess.run = release
    sys.argv = ['scripts/ftft9_validation.py', '--output', str(OUT), '--jobs', '4']
    try: code = gate.main()
    finally: subprocess.run = original
    (OUT / 'M33_METADATA.json').write_text(json.dumps({
        'clean_build_directory_initially_absent': True,
        'old_build_preserved': str(backup.relative_to(ROOT)),
        'configure_adapter': commands,
        'test_output_adapter': 'Only the M33 test output directory is redirected; all engine/fixture paths unchanged',
        'existing_validation_script': 'scripts/ftft9_validation.py',
        'exit_code': code, 'seconds': time.monotonic() - start,
        'human_visual_validation': 'NOT RUN',
    }, indent=2)+'\n')
    return code
if __name__ == '__main__': raise SystemExit(main())
