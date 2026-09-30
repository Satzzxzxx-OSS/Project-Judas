#!/usr/bin/env python3
"""Final signoff: existing FTFT9 gate and shipped performance, once, clean Release.

No fixture, assertion, tolerance or engine-path changes. The sole command
adapter explicitly selects Release in the existing runner's configure command.
An existing build is preserved under ignored .cache before configuration.
"""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import time

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
CHECKPOINT = '1b3a134bdb3d7876d8ed8261571311ca7880e386'


def main():
    git = lambda *args: subprocess.check_output(['git', *args], cwd=ROOT, text=True).strip()
    assert git('rev-parse', 'HEAD') == CHECKPOINT
    assert git('rev-parse', 'main') == CHECKPOINT
    assert git('rev-parse', 'origin/main') == CHECKPOINT
    assert not git('diff', 'HEAD', '--name-only'), 'Tracked input changed before gate'
    build = ROOT / 'build'
    backup = ROOT / '.cache/final-gate-prior-build-1b3a134'
    assert not backup.exists(), 'Prior build backup already exists; do not rerun blindly'
    assert not (OUT / 'validation').exists(), 'Gate already executed; review its results'
    if build.exists():
        backup.parent.mkdir(parents=True, exist_ok=True)
        build.rename(backup)
    assert not build.exists(), 'Release build directory must start nonexistent'
    record = {
        'checkpoint': CHECKPOINT,
        'initial_head_main_origin_verified': True,
        'initial_clean_tree_verified_before_evidence_creation': True,
        'build_directory_initially_absent': True,
        'prior_build_preserved': str(backup.relative_to(ROOT)),
        'command_adapter': 'Add -DCMAKE_BUILD_TYPE=Release to existing configure command only',
        'human_visual_validation': 'NOT RUN; existing automated offscreen GL checks',
        'commands': [],
    }
    (OUT / 'RUN_METADATA.json').write_text(json.dumps(record, indent=2) + '\n')
    spec = importlib.util.spec_from_file_location('judas_final_existing_gate', ROOT / 'scripts/ftft9_validation.py')
    gate = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gate)
    original_run = subprocess.run

    def release_configure(command, *args, **kwargs):
        if command == ['cmake', '-S', '.', '-B', 'build']:
            command.append('-DCMAKE_BUILD_TYPE=Release')
            record['commands'].append(list(command))
        return original_run(command, *args, **kwargs)

    started = time.monotonic()
    subprocess.run = release_configure
    sys.argv = ['scripts/ftft9_validation.py', '--output', str(OUT / 'validation'), '--jobs', '4']
    try:
        validation_code = gate.main()
    finally:
        subprocess.run = original_run
    record['validation_exit_code'] = validation_code
    if validation_code == 0:
        command = ['./build/judas_production_fluid_performance', '--output', str(OUT / 'performance')]
        record['commands'].append(command)
        env = {k: v for k, v in os.environ.items() if not k.startswith('JUDAS_')}
        env.update(SDL_VIDEODRIVER='offscreen', LIBGL_ALWAYS_SOFTWARE='1')
        with (OUT / 'performance.log').open('w') as stream:
            performance = original_run(command, cwd=ROOT, env=env, stdout=stream,
                                       stderr=subprocess.STDOUT, timeout=600)
        record['performance_exit_code'] = performance.returncode
    record['elapsed_seconds'] = time.monotonic() - started
    record['overall_pass'] = validation_code == 0 and record.get('performance_exit_code') == 0
    record['runner_sha256'] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    (OUT / 'RUN_METADATA.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2), flush=True)
    return 0 if record['overall_pass'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
