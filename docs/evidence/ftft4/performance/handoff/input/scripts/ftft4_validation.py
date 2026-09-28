#!/usr/bin/env python3
"""FTFT4A production regression runner; existing acceptance logic stays unchanged.

Baseline/final evidence is written outside protected FTFT1--3/prototype trees.
The FTFT2 runner is imported unchanged. A command adapter supplies an explicit
output directory to its discovered FTFT3 gravity test; it changes no test,
fixture, threshold, synchronization, resource mode, or production mechanism.
No fluid prototype is built or run. Final contact trajectories are compared
and reported, not silently rebaselined or required to be byte-identical.
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
import statistics
import subprocess
import sys
import time

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/evidence/ftft4'
BASELINE_HEAD = '3404388f1af48419505c802fc11dbf8a0b577f38'
PROTECTED = [
    'prototypes/fluid_coupling/r1_p1',
    'docs/evidence/ftft1', 'docs/evidence/ftft2', 'docs/evidence/ftft3',
    'src/WorldState.cpp', 'src/WorldState.h', 'src/SceneFingerprint.cpp',
    'src/SceneFingerprint.h', 'src/SceneSerialization.cpp',
    'src/RuntimeWorld.cpp', 'src/RuntimeWorld.h',
    'tests/WorldStateTests.cpp', 'tests/SceneFingerprintTests.cpp',
    'tests/LifecycleTests.cpp', 'tests/UniformGravitySceneTests.cpp',
    'src/Application.cpp', 'src/Application.h', 'src/AsyncFile.cpp', 'src/AsyncFile.h',
    'src/EngineHost.cpp', 'src/EngineHost.h', 'src/Renderer.cpp', 'src/Renderer.h',
    'src/ResourceManager.cpp', 'src/ResourceManager.h', 'src/ResourceTrace.h',
    'src/FaithfulGravity.cpp', 'src/FaithfulGravity.h',
    'src/RadicalGravity.cpp', 'src/RadicalGravity.h', 'src/UniformGravity.h',
    'tests/AsyncApplicationTests.cpp', 'tests/JobTests.cpp',
    'scripts/ftft2_validation.py', 'scripts/ftft3_validation.py',
]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT, text=True).strip()


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + '\n')


def environment():
    env = {k: v for k, v in os.environ.items() if not k.startswith('JUDAS_')}
    env.update(SDL_VIDEODRIVER='offscreen', LIBGL_ALWAYS_SOFTWARE='1', PYTHONDONTWRITEBYTECODE='1')
    return env


def fingerprints():
    paths = [ROOT / 'CMakeLists.txt']
    for name in ['src', 'tests', 'scripts']:
        paths += [p for p in (ROOT / name).rglob('*') if p.is_file()
                  and p.suffix in ['.cpp', '.h', '.py']]
    return {str(p.relative_to(ROOT)): sha(p) for p in sorted(paths)}


def protection():
    hashes = {}
    for name in PROTECTED:
        path = ROOT / name
        for p in sorted(path.rglob('*')) if path.is_dir() else [path]:
            if p.is_file():
                hashes[str(p.relative_to(ROOT))] = sha(p)
    return {'paths': PROTECTED, 'sha256': hashes,
            'head_objects': {p: git('rev-parse', 'HEAD:' + p) for p in PROTECTED},
            'status': git('status', '--porcelain', '--', *PROTECTED)}


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def execute(name, command, directory, extra_env=None, timeout=600):
    directory.mkdir(parents=True, exist_ok=True)
    log = directory / (name + '.log')
    start = time.monotonic()
    with log.open('w') as stream:
        try:
            code = subprocess.run(command, cwd=ROOT, env=dict(environment(), **(extra_env or {})),
                                  stdout=stream, stderr=subprocess.STDOUT, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            code = 'TIMEOUT'
    executable = Path(command[0])
    if not executable.is_absolute():
        executable = ROOT / executable if (ROOT / executable).is_file() else Path(shutil.which(command[0]) or command[0])
    record = {'name': name, 'command': [str(x) for x in command], 'exit_code': code,
              'seconds': time.monotonic() - start, 'environment_overrides': extra_env or {},
              'log': str(log.relative_to(ROOT)), 'executable_sha256': sha(executable) if executable.is_file() else None}
    text = log.read_text(errors='replace')
    record['failing_lines'] = len(re.findall(r'^\s*FAIL\s+', text, re.M))
    print(f'{name}: exit={code}, {record["seconds"]:.3f}s', flush=True)
    return record, text


def reuse_ftft2(directory, jobs):
    module = load_module('ftft4_ftft2_reuse', ROOT / 'scripts/ftft2_validation.py')
    original_run = subprocess.run
    adapters = []

    def redirected_run(command, *args, **kwargs):
        if isinstance(command, list) and Path(command[0]).name == 'judas_uniform_gravity_scene_tests':
            if '--output' in command:
                raise RuntimeError('Unexpected existing gravity output option; review adapter before reuse.')
            # The original runner records this same list after execution.
            command.extend(['--output', str(directory / 'gravity-discovered')])
            adapters.append(list(command))
        return original_run(command, *args, **kwargs)

    before_argv = sys.argv
    try:
        subprocess.run = redirected_run
        sys.argv = ['scripts/ftft2_validation.py', '--output', str(directory / 'production'), '--jobs', str(jobs)]
        with (directory / 'ftft2_reuse.log').open('w') as stream, redirect_stdout(stream), redirect_stderr(stream):
            try:
                module.main()
                code = 0
            except SystemExit as error:
                code = error.code
    finally:
        subprocess.run = original_run
        sys.argv = before_argv
    path = directory / 'production/results.json'
    result = json.loads(path.read_text()) if path.exists() else {}
    return {'script_sha256': sha(ROOT / 'scripts/ftft2_validation.py'),
            'exit_code': code, 'output_adapters': adapters,
            'scope': 'unchanged assertions and production paths; only gravity evidence output redirected',
            'results': str(path.relative_to(ROOT)), 'overall_pass': code == 0 and result.get('overall_pass', False)}, result


def reuse_ftft1(directory):
    module = load_module('ftft4_ftft1_smoke', ROOT / 'docs/evidence/ftft1/runtime_smoke/run.py')
    module.OUT = directory / 'ftft1_runtime_smoke'
    module.OUT.mkdir(exist_ok=True)
    shutil.copyfile(ROOT / 'docs/evidence/ftft1/runtime_smoke/fingerprint_tool.cpp', module.OUT / 'fingerprint_tool.cpp')
    with (directory / 'ftft1_reuse.log').open('w') as stream, redirect_stdout(stream), redirect_stderr(stream):
        code = module.main()
    result = json.loads((module.OUT / 'results.json').read_text())
    return {'script_sha256': sha(ROOT / 'docs/evidence/ftft1/runtime_smoke/run.py'),
            'exit_code': code, 'output': str(module.OUT.relative_to(ROOT)),
            'logic': 'unchanged; OUT redirected; identical fingerprint tool copied',
            'human_visual_validation': 'NOT RUN; inherited visual-review labels are not visual acceptance',
            'overall_pass': code == 0 and result['automated_acceptance']}


def walks(directory):
    records, payloads = [], {}
    walk = ROOT / 'docs/evidence/ftft3/walk.txt'
    for scene in ['classic', 'terrain']:
        for origin in ['near', 'far']:
            name = scene + '_' + origin
            item, text = execute(name, ['./build/judas', 'assets/scenes/' + scene + '.judas'], directory,
                                 {'JUDAS_TEST_SCRIPT': str(walk), 'JUDAS_WORLD_STATE': 'none',
                                  'JUDAS_WORLD_OFFSET': '0,0,0' if origin == 'near' else 'far'})
            headers = [line for line in text.splitlines() if line.startswith('step,time,')]
            rows = [line for line in text.splitlines() if re.match(r'^\d+,', line)]
            item['scope'] = 'unchanged scripted blocking physical regression; NOT async evidence'
            item['rows'] = len(rows)
            item['complete'] = item['exit_code'] == 0 and len(headers) == 1 and [int(r.split(',', 1)[0]) for r in rows] == list(range(0, 900, 10))
            payload = '\n'.join(headers + rows) + '\n'
            (directory / (name + '.csv')).write_text(payload)
            item['physical_payload_sha256'] = hashlib.sha256(payload.encode()).hexdigest()
            records.append(item)
            payloads[name] = payload
    checks = {'four_complete_900_step_walks': all(r['complete'] for r in records)}
    checks.update({scene + '_near_far_identical': payloads[scene + '_near'] == payloads[scene + '_far'] for scene in ['classic', 'terrain']})
    result = {'runs': records, 'checks': checks, 'walk_sha256': sha(walk),
              'scene_sha256': {s: sha(ROOT / ('assets/scenes/' + s + '.judas')) for s in ['classic', 'terrain']}}
    save(directory / 'results.json', result)
    return result, payloads


def compare_walks(payloads):
    result = {}
    for name, after in payloads.items():
        before = (OUT / 'baseline/harness' / (name + '.csv')).read_text()
        old_rows, new_rows = before.splitlines(), after.splitlines()
        columns = old_rows[0].split(',') if old_rows else []
        differences = {}
        if old_rows and new_rows and old_rows[0] == new_rows[0] and len(old_rows) == len(new_rows):
            for old, new in zip(old_rows[1:], new_rows[1:]):
                for column, a, b in zip(columns, old.split(','), new.split(',')):
                    try:
                        delta = abs(float(a) - float(b))
                    except ValueError:
                        continue
                    differences[column] = max(differences.get(column, 0.0), delta)
        result[name] = {'byte_identical': before == after,
                        'changed_rows': sum(a != b for a, b in zip(old_rows[1:], new_rows[1:])),
                        'max_absolute_difference_by_numeric_column': differences,
                        'acceptance': 'informational: physical explanation required for changes, no automatic rebaselining'}
    return result


def performance(directory, repetitions, require_geometry_counts=False):
    # Exactly TestScaleStatistics in the unchanged current broadphase suite.
    expression = re.compile(r'(\d+) bodies, (\d+) possible pairs, (\d+) candidates, (\d+) colliding pairs, (\d+) points, tree height (\d+), ([\d.]+) ms/step \(broadphase ([\d.]+), narrowphase ([\d.]+), solver ([\d.]+)\)')
    rows = []
    for repetition in range(repetitions):
        item, text = execute('broadphase_' + str(repetition + 1), ['./build/judas_broadphase_tests'], directory)
        matches = expression.findall(text)
        item['benchmark_rows'] = len(matches)
        geometry = re.findall(r'geometry predicates (\d+), exact fallbacks (\d+), unresolved (\d+)', text)
        item['geometry_diagnostic_rows'] = len(geometry)
        item['geometry_diagnostics'] = (dict(zip(['predicates', 'exact_fallbacks', 'unresolved'], map(int, geometry[0])))
                                        if len(geometry) == 1 else None)
        if len(matches) == 1:
            row = matches[0]
            item['stats'] = dict(zip(['bodies', 'possible_pairs', 'candidates', 'colliding_pairs', 'points', 'tree_height'], map(int, row[:6])))
            item['stats'].update(dict(zip(['total_ms', 'broadphase_ms', 'narrowphase_ms', 'solver_ms'], map(float, row[6:]))))
        item['pass'] = item['exit_code'] == 0 and item['failing_lines'] == 0 and len(matches) == 1
        if require_geometry_counts:
            item['pass'] = item['pass'] and len(geometry) == 1 and item['geometry_diagnostics']['unresolved'] == 0
        rows.append(item)
    summaries = {}
    for key in ['total_ms', 'broadphase_ms', 'narrowphase_ms', 'solver_ms']:
        values = [r['stats'][key] for r in rows if 'stats' in r]
        summaries[key] = {'median': statistics.median(values), 'min': min(values), 'max': max(values)} if values else None
    result = {'fixture': 'tests/BroadphaseTests.cpp::TestScaleStatistics; 1500 crates; 30 fixed steps; final-step timer',
              'fixture_source_sha256': sha(ROOT / 'tests/BroadphaseTests.cpp'),
              'repetitions': repetitions, 'runs': rows, 'timings': summaries,
              'timing_precision': 'existing suite prints 0.001 ms; process timing includes all broadphase tests and exhaustive comparison',
              'fallback_counts': ('actual final-step production geometry counters, per run above' if require_geometry_counts
                                  else 'not exposed by the baseline suite; no baseline fallback implementation existed'),
              'overall_pass': len(rows) == repetitions and all(r['pass'] for r in rows)}
    save(directory / 'results.json', result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--baseline', action='store_true')
    mode.add_argument('--final', action='store_true')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--performance-runs', type=int, default=7)
    args = parser.parse_args()
    if args.performance_runs < 3:
        raise SystemExit('At least three same-machine performance repetitions are required.')
    phase = 'baseline' if args.baseline else 'final'
    directory = OUT / phase
    result_path = directory / 'results.json'
    if result_path.exists():
        raise SystemExit('Refusing to overwrite existing ' + phase + ' evidence: ' + str(result_path))
    if args.baseline and git('rev-parse', 'HEAD') != BASELINE_HEAD:
        raise SystemExit('Baseline HEAD differs; inspect before accepting new source provenance.')
    protected = protection()
    if protected['status']:
        raise SystemExit('Protected paths changed: ' + protected['status'])
    if args.final:
        baseline = json.loads((OUT / 'baseline/results.json').read_text())
        if not baseline['overall_pass']:
            raise SystemExit('A passing baseline is required.')
        if protected != baseline['protected_before']:
            raise SystemExit('Protected paths changed since baseline.')
    directory.mkdir(parents=True, exist_ok=True)
    result = {'phase': phase, 'head': git('rev-parse', 'HEAD'),
              'git_status_before': git('status', '--short'), 'source_sha256_before': fingerprints(),
              'protected_before': protected, 'checks': {},
              'human_visual_validation': 'NOT RUN; offscreen software real-GL automation only',
              'scope': 'FTFT4A production regressions; independent geometry/source probes recorded separately; temporal FTFT4B remains OPEN'}
    save(result_path, result)
    start = time.monotonic()
    reuse, production = reuse_ftft2(directory, args.jobs)
    result['ftft2_reuse'] = reuse
    result['checks']['full_production_and_async'] = reuse['overall_pass']
    result['production_suite_count'] = production.get('production_suite_count')
    result['integration_assertions'] = production.get('integration_assertions')
    save(result_path, result)
    if not reuse['overall_pass']:
        raise SystemExit('Production regression failed; see ' + str(directory / 'production/results.json'))
    if args.final:
        player, text = execute('independent_player_sweeps', ['./build/judas_contact_geometry_tests', '--player'], directory)
        cases = [json.loads(line) for line in text.splitlines() if line.startswith('{')]
        summary = re.search(r'Independent analytical player sweeps: (\d+) cases, (\d+) failures', text)
        player['cases'] = cases
        player['summary'] = {'cases': int(summary[1]), 'failures': int(summary[2])} if summary else None
        result['independent_player_sweeps'] = player
        result['checks']['independent_21_player_sweeps'] = (player['exit_code'] == 0 and len(cases) == 21
            and all(case['pass'] for case in cases) and player['summary'] == {'cases': 21, 'failures': 0})
        geometry_log = directory / 'production/judas_contact_geometry_tests.log'
        geometry = [json.loads(line) for line in geometry_log.read_text().splitlines() if line.startswith('{')]
        # Adapter counters are cumulative across the whole executable. Do not
        # sum cumulative per-case counters or confuse them with per-step stats.
        counters = [row['diagnostics'] for row in geometry]
        expected = json.loads((OUT / 'geometry/fixture_summary.json').read_text())['cases']
        monotone = all(all(a[key] <= b[key] for key in a) for a, b in zip(counters, counters[1:]))
        result['geometry_predicate_diagnostics'] = {'scope': 'independent fixture executable, cumulative counters',
            'cases': len(geometry), 'expected_cases': expected,
            'final_counters': counters[-1] if counters else None, 'counters_monotone': monotone,
            'source_log': str(geometry_log.relative_to(ROOT))}
        result['checks']['geometry_predicate_diagnostics'] = (len(geometry) == expected and monotone
            and all(d['unresolved'] == 0 and d['invalid_inputs'] == 0 for d in counters))
    result['ftft1_smoke'] = reuse_ftft1(directory)
    result['checks']['ftft1_smoke'] = result['ftft1_smoke']['overall_pass']
    item, _ = execute('gravity_explicit', ['./build/judas_uniform_gravity_scene_tests', '--output', str(directory / 'gravity-explicit')], directory)
    result['gravity_explicit'] = item
    counts = json.loads((directory / 'gravity-explicit/results.json').read_text())
    result['gravity_counts'] = counts
    result['checks']['ftft3_construction'] = item['exit_code'] == 0 and counts['failures'] == 0 and counts['scenes'] == 136 and counts['runtime_constructions'] == 272 and counts['fixed_steps'] == 32640
    result['harness'], payloads = walks(directory / 'harness')
    result['checks'].update(result['harness']['checks'])
    if args.final:
        result['harness_pre_post_comparison'] = compare_walks(payloads)
        result['checks']['walk_fixture_unchanged'] = result['harness']['walk_sha256'] == baseline['harness']['walk_sha256'] and result['harness']['scene_sha256'] == baseline['harness']['scene_sha256']
    result['performance'] = performance(directory / 'performance', args.performance_runs, require_geometry_counts=args.final)
    result['checks']['performance_fixtures_pass'] = result['performance']['overall_pass']
    if args.final:
        baseline_fixture = git('show', BASELINE_HEAD + ':tests/BroadphaseTests.cpp') + '\n'
        current_fixture = (ROOT / 'tests/BroadphaseTests.cpp').read_text()
        # Only the new diagnostic printf is omitted from this comparison.
        # Geometry, body count, layout, step count and measured operation remain exact.
        without_diagnostic = re.sub(r'    std::printf\("    geometry predicates .*?;\n', '', current_fixture, flags=re.S)
        def scale_fixture(text):
            start = text.index('void TestScaleStatistics() {')
            return text[start:text.index('\n}\n', start) + 3]
        fixture_unchanged = scale_fixture(baseline_fixture) == scale_fixture(without_diagnostic)
        result['performance_pre_post'] = {'baseline': baseline['performance']['timings'], 'final': result['performance']['timings'],
                                         'source_file_identical': baseline['performance']['fixture_source_sha256'] == result['performance']['fixture_source_sha256'],
                                         'fixture_unchanged_except_geometry_counter_print': fixture_unchanged}
        result['checks']['benchmark_fixture_unchanged_except_diagnostics'] = fixture_unchanged
    result['source_sha256_after'] = fingerprints()
    result['protected_after'] = protection()
    result['checks']['sources_unchanged_during_run'] = result['source_sha256_before'] == result['source_sha256_after']
    result['checks']['protected_unchanged'] = result['protected_before'] == result['protected_after']
    result['git_status_after'] = git('status', '--short')
    result['seconds'] = time.monotonic() - start
    result['overall_pass'] = all(result['checks'].values())
    result['evidence_sha256'] = {str(p.relative_to(directory)): sha(p) for p in sorted(directory.rglob('*')) if p.is_file() and p != result_path}
    save(result_path, result)
    print(json.dumps({'phase': phase, 'overall_pass': result['overall_pass'], 'checks': result['checks'],
                      'seconds': result['seconds'], 'performance': result['performance']['timings']}, indent=2), flush=True)
    return 0 if result['overall_pass'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
