#!/usr/bin/env python3
"""FTFT5: verify fixed-origin scope through unchanged production paths."""
import argparse
import importlib.util
import json
from pathlib import Path
import sys
import time
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('ftft5_existing', ROOT/'scripts/ftft4_validation.py')
BASE = importlib.util.module_from_spec(spec)
spec.loader.exec_module(BASE)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    start = time.monotonic()
    before = BASE.fingerprints()
    protected = BASE.protection()
    BASE.save(out/'source-sha256.json', before)
    records = []
    for name, command in [
        ('build', ['cmake', '--build', 'build', '--target', 'judas_world_coordinates_tests', 'judas_uniform_gravity_scene_tests', 'judas', '-j4']),
        ('world-coordinates', ['./build/judas_world_coordinates_tests']),
        ('scene-gravity', ['./build/judas_uniform_gravity_scene_tests', '--output', str(out/'scene-gravity')])
    ]:
        record, _ = BASE.execute(name, command, out)
        records.append(record)
        if record['exit_code'] != 0:
            BASE.save(out/'results.json', {'overall_pass': False, 'commands': records})
            return 1
    walks, _ = BASE.walks(out/'harness')
    checks = dict(walks['checks'], source_unchanged=before==BASE.fingerprints(),
                  protected_unchanged=protected==BASE.protection(),
                  no_production_changes=not BASE.git('diff', '--name-only', 'HEAD', '--', 'src', 'tests'))
    result = {'overall_pass': all(checks.values()), 'checks': checks, 'head': BASE.git('rev-parse', 'HEAD'),
              'commands': records, 'walks': walks, 'gravity_counts': json.loads((out/'scene-gravity/results.json').read_text()),
              'seconds': time.monotonic()-start, 'scope': 'Fixed double origin with local float simulation; no live rebase implemented or tested.',
              'human_visual_validation': 'NOT RUN; real offscreen GL scripted paths'}
    BASE.save(out/'results.json', result)
    print(json.dumps({k:result[k] for k in ['overall_pass', 'checks', 'seconds']}))
    return 0 if result['overall_pass'] else 1
if __name__ == '__main__':
    raise SystemExit(main())
