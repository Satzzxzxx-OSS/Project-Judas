#!/usr/bin/env python3
"""FTFT3 scene-gravity regression evidence, preserving accepted FTFT1/2 evidence.

--baseline verifies the existing runtime against FTFT2's passing fingerprints,
then captures classic/terrain fixed-step walks without rebuilding anything.
--final reuses the FTFT2 suite and FTFT1 smoke logic with redirected output,
runs the new construction suite explicitly, and compares the unchanged walks.
No fluid prototype is built, executed, or changed by this runner.
"""
import argparse
from contextlib import redirect_stderr, redirect_stdout
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

# Imported accepted runners must not create caches inside protected evidence.
sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/evidence/ftft3'
WALK = OUT / 'walk.txt'
PROTECTED = [
    'prototypes/fluid_coupling/r1_p1', 'docs/evidence/ftft1', 'docs/evidence/ftft2',
    'src/WorldState.cpp', 'src/WorldState.h', 'src/SceneFingerprint.cpp',
    'src/SceneFingerprint.h', 'src/SceneSerialization.cpp', 'src/RuntimeWorld.h',
    'tests/WorldStateTests.cpp', 'tests/SceneFingerprintTests.cpp', 'tests/LifecycleTests.cpp',
    'src/Application.cpp', 'src/Application.h', 'src/AsyncFile.cpp', 'src/AsyncFile.h',
    'src/EngineHost.cpp', 'src/EngineHost.h', 'src/Renderer.cpp', 'src/Renderer.h',
    'src/ResourceManager.cpp', 'src/ResourceManager.h', 'src/ResourceTrace.h',
    'tests/AsyncApplicationTests.cpp', 'tests/JobTests.cpp', 'scripts/ftft2_validation.py',
]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT, text=True).strip()


def fingerprints():
    paths = [ROOT / 'CMakeLists.txt', OUT / 'generate_fixtures.py', WALK]
    paths += [p for p in (OUT / 'fixtures').rglob('*') if p.is_file()]
    for directory in ['src', 'tests', 'scripts']:
        paths += [p for p in (ROOT / directory).rglob('*')
                  if p.is_file() and p.suffix in ['.h', '.cpp', '.py']]
    return {str(p.relative_to(ROOT)): sha(p) for p in sorted(paths)}


def protection():
    files = {}
    for name in PROTECTED:
        path = ROOT / name
        for item in sorted(path.rglob('*')) if path.is_dir() else [path]:
            if item.is_file():
                files[str(item.relative_to(ROOT))] = sha(item)
    return {
        'paths': PROTECTED,
        'working_sha256': files,
        'head_objects': {p: git('rev-parse', 'HEAD:' + p) for p in PROTECTED},
        'git_status': git('status', '--porcelain', '--', *PROTECTED),
    }


def environment():
    env = {k: v for k, v in os.environ.items() if not k.startswith('JUDAS_')}
    env.update(SDL_VIDEODRIVER='offscreen', LIBGL_ALWAYS_SOFTWARE='1')
    return env


def save(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2) + '\n')


def execute(name, command, directory, extra_env=None, timeout=600):
    directory.mkdir(parents=True, exist_ok=True)
    log = directory / (name + '.log')
    start = time.monotonic()
    with log.open('w') as output:
        try:
            code = subprocess.run(command, cwd=ROOT, env=dict(environment(), **(extra_env or {})),
                                  stdout=output, stderr=subprocess.STDOUT, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            code = 'TIMEOUT'
    record = {'name': name, 'command': [str(x) for x in command], 'exit_code': code,
              'seconds': time.monotonic() - start, 'environment_overrides': extra_env or {},
              'log': str(log.relative_to(ROOT)), 'executable_sha256': sha(ROOT / command[0])}
    print(f'{name}: exit={code}, {record["seconds"]:.3f}s', flush=True)
    return record, log.read_text(errors='replace')


def harness(directory):
    records = []
    payloads = {}
    for scene in ['classic', 'terrain']:
        for origin in ['near', 'far']:
            name = scene + '_' + origin
            overrides = {'JUDAS_TEST_SCRIPT': str(WALK), 'JUDAS_WORLD_STATE': 'none',
                         'JUDAS_WORLD_OFFSET': '0,0,0' if origin == 'near' else 'far'}
            record, text = execute(name, ['./build/judas', 'assets/scenes/' + scene + '.judas'],
                                   directory, overrides)
            headers = [line for line in text.splitlines() if line.startswith('step,time,')]
            rows = [line for line in text.splitlines() if re.match(r'^\d+,', line)]
            expected_steps = list(range(0, 900, 10))
            steps = [int(line.split(',', 1)[0]) for line in rows]
            record.update(scope='legacy scripted blocking physical regression; NOT async evidence',
                          rows=len(rows), complete=record['exit_code'] == 0 and len(headers) == 1
                          and steps == expected_steps)
            payload = '\n'.join(headers + rows) + '\n'
            (directory / (name + '.csv')).write_text(payload)
            record['physical_payload_sha256'] = hashlib.sha256(payload.encode()).hexdigest()
            payloads[name] = payload
            records.append(record)
    checks = {scene + '_near_far_identical': payloads[scene + '_near'] == payloads[scene + '_far']
              for scene in ['classic', 'terrain']}
    checks['four_complete_900_step_walks'] = all(r['complete'] for r in records)
    data = {'runs': records, 'checks': checks, 'walk_sha256': sha(WALK),
            'scene_sha256': {scene: sha(ROOT / ('assets/scenes/' + scene + '.judas'))
                             for scene in ['classic', 'terrain']}}
    save(directory / 'results.json', data)
    return data, payloads


def baseline():
    metadata_path = OUT / 'pre-fix/harness/baseline.json'
    if metadata_path.exists():
        raise SystemExit('Baseline already exists; refusing to overwrite the pre-fix witness.')
    accepted = json.loads((ROOT / 'docs/evidence/ftft2/final/results.json').read_text())
    runtime = next(r for r in accepted['runs'] if r['name'] == 'runtime_blocking_reference')
    production = {p: value for p, value in accepted['source_sha256_after'].items() if p.startswith('src/')}
    current = {p: sha(ROOT / p) for p in production}
    mismatches = [p for p in production if current[p] != production[p]]
    executable_hash = sha(ROOT / 'build/judas')
    if mismatches or executable_hash != runtime['executable_sha256']:
        raise SystemExit('Cannot establish accepted pre-fix executable/source provenance: '
                         + json.dumps({'source_mismatches': mismatches, 'runtime_sha256': executable_hash,
                                       'accepted_runtime_sha256': runtime['executable_sha256']}))
    protected = protection()
    if protected['git_status']:
        raise SystemExit('Protected paths changed before baseline: ' + protected['git_status'])
    result = {'head': git('rev-parse', 'HEAD'), 'git_status': git('status', '--short'),
              'scope': 'FTFT3 pre-fix physical harness; accepted FTFT2 runtime, no rebuild',
              'accepted_evidence': 'docs/evidence/ftft2/final/results.json',
              'accepted_evidence_sha256': sha(ROOT / 'docs/evidence/ftft2/final/results.json'),
              'runtime_sha256': executable_hash, 'accepted_runtime_sha256': runtime['executable_sha256'],
              'production_sources_match_accepted_validation': not mismatches,
              'source_sha256_before': fingerprints(), 'protected_before': protected}
    save(metadata_path, result)
    run, _ = harness(OUT / 'pre-fix/harness')
    result['harness'] = run
    result['source_sha256_after'] = fingerprints()
    result['protected_after'] = protection()
    result['runtime_sha256_after'] = sha(ROOT / 'build/judas')
    # New test/script files may legitimately be authored concurrently. Check
    # existing production source and the accepted runtime stayed fixed here.
    result['production_sources_unchanged_during_baseline'] = all(sha(ROOT / p) == h for p, h in current.items())
    result['runtime_unchanged_during_baseline'] = executable_hash == result['runtime_sha256_after']
    result['overall_pass'] = (all(run['checks'].values())
                              and result['production_sources_unchanged_during_baseline']
                              and result['runtime_unchanged_during_baseline']
                              and result['protected_before'] == result['protected_after'])
    save(metadata_path, result)
    print(json.dumps({'baseline_pass': result['overall_pass'], 'checks': run['checks']}, indent=2), flush=True)
    return 0 if result['overall_pass'] else 1


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def final(jobs):
    pre = json.loads((OUT / 'pre-fix/harness/baseline.json').read_text())
    if not pre.get('overall_pass'):
        raise SystemExit('A passing, proven pre-fix baseline is required.')
    protected_before = protection()
    if protected_before['working_sha256'] != pre['protected_before']['working_sha256']:
        raise SystemExit('Protected FTFT1/FTFT2/fluid work changed since the baseline.')
    result = {'head': git('rev-parse', 'HEAD'), 'git_status_before': git('status', '--short'),
              'source_sha256_before': fingerprints(), 'protected_before': protected_before,
              'human_visual_validation': 'NOT RUN; offscreen real-GL automation only',
              'checks': {}, 'runs': []}
    start = time.monotonic()
    # This FTFT2 reuse changes output/protection bookkeeping, not executable
    # algorithms, synchronization, acceptance checks, fixtures, or tolerances.
    ftft2 = load_module('ftft2_reuse', ROOT / 'scripts/ftft2_validation.py')
    original_protected = list(ftft2.PROTECTED)
    ftft2.PROTECTED = [p for p in ftft2.PROTECTED if p != 'src/RuntimeWorld.cpp']
    result['ftft2_reuse'] = {'script_sha256': sha(ROOT / 'scripts/ftft2_validation.py'),
                            'protection_removed': ['src/RuntimeWorld.cpp'],
                            'reason': 'FTFT3 may change only the scene-gravity construction in this file; '
                                      'persistence guarantees are revalidated, all other protected paths stay protected.',
                            'original_protected': original_protected,
                            'effective_protected': ftft2.PROTECTED,
                            'output': 'docs/evidence/ftft3/regressions'}
    argv = sys.argv
    try:
        sys.argv = ['scripts/ftft2_validation.py', '--output', 'docs/evidence/ftft3/regressions', '--jobs', str(jobs)]
        with (OUT / 'ftft2_reuse.log').open('w') as stream, redirect_stdout(stream), redirect_stderr(stream):
            try:
                ftft2.main()
                code = 0
            except SystemExit as error:
                code = error.code
    finally:
        sys.argv = argv
    result['ftft2_reuse']['exit_code'] = code
    regressions = json.loads((OUT / 'regressions/results.json').read_text())
    result['checks']['ftft2_and_all_production_suites_pass'] = code == 0 and regressions['overall_pass']
    save(OUT / 'results.json', result)
    if not result['checks']['ftft2_and_all_production_suites_pass']:
        raise SystemExit('FTFT2/current production regression failure; inspect redirected evidence.')

    construction, text = execute('uniform_gravity_scene', ['./build/judas_uniform_gravity_scene_tests',
                                   '--output', str(OUT / 'construction-explicit')], OUT)
    result['runs'].append(construction)
    counts = json.loads((OUT / 'construction-explicit/results.json').read_text())
    result['construction_counts'] = counts
    result['checks']['construction_pass'] = construction['exit_code'] == 0 and counts['failures'] == 0
    result['checks']['complete_construction_family'] = all(
        counts[key] == expected for key, expected in
        [('scenes', 136), ('runtime_constructions', 272), ('fixed_steps', 32640)])

    smoke = load_module('ftft1_smoke_reuse', ROOT / 'docs/evidence/ftft1/runtime_smoke/run.py')
    smoke.OUT = OUT / 'ftft1_runtime_smoke'
    smoke.OUT.mkdir(exist_ok=True)
    shutil.copyfile(ROOT / 'docs/evidence/ftft1/runtime_smoke/fingerprint_tool.cpp', smoke.OUT / 'fingerprint_tool.cpp')
    with (OUT / 'ftft1_smoke_reuse.log').open('w') as stream, redirect_stdout(stream), redirect_stderr(stream):
        smoke_code = smoke.main()
    smoke_result = json.loads((smoke.OUT / 'results.json').read_text())
    result['ftft1_smoke_reuse'] = {
        'script_sha256': sha(ROOT / 'docs/evidence/ftft1/runtime_smoke/run.py'),
        'output': str(smoke.OUT.relative_to(ROOT)), 'exit_code': smoke_code,
        'source_logic': 'unchanged; OUT redirected and identical fingerprint tool copied',
        'status_visual_review': 'NOT RUN; inherited REQUIRED fields are not claimed as human acceptance',
    }
    result['checks']['ftft1_six_process_smoke'] = smoke_code == 0 and smoke_result['automated_acceptance']

    post, payloads = harness(OUT / 'harness')
    result['harness'] = post
    result['checks'].update(post['checks'])
    result['checks']['harness_fixture_script_unchanged'] = post['walk_sha256'] == pre['harness']['walk_sha256']
    result['checks']['harness_scene_sources_unchanged'] = post['scene_sha256'] == pre['harness']['scene_sha256']
    for name, payload in payloads.items():
        result['checks'][name + '_pre_post_identical'] = payload == (OUT / ('pre-fix/harness/' + name + '.csv')).read_text()
    result['source_sha256_after'] = fingerprints()
    result['protected_after'] = protection()
    result['checks']['tested_sources_unchanged_during_run'] = result['source_sha256_before'] == result['source_sha256_after']
    result['checks']['protected_work_unchanged'] = result['protected_before'] == result['protected_after']
    result['seconds'] = time.monotonic() - start
    result['git_status_after'] = git('status', '--short')
    result['overall_pass'] = all(result['checks'].values())
    # The wrapper's stdout may still be open, and review documentation is
    # written after validation. REVIEW_SHA256.json fingerprints those final
    # artifacts separately; this manifest covers completed run evidence.
    deferred = {'results.json', 'validation-progress.log', 'README.md',
                'observations.json', 'REVIEW_SHA256.json'}
    result['evidence_sha256'] = {str(p.relative_to(OUT)): sha(p) for p in sorted(OUT.rglob('*'))
                                if p.is_file() and str(p.relative_to(OUT)) not in deferred}
    save(OUT / 'results.json', result)
    print(json.dumps({'overall_pass': result['overall_pass'], 'checks': result['checks'],
                      'seconds': result['seconds']}, indent=2), flush=True)
    return 0 if result['overall_pass'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--baseline', action='store_true')
    mode.add_argument('--final', action='store_true')
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    return baseline() if args.baseline else final(args.jobs)


if __name__ == '__main__':
    raise SystemExit(main())
