#!/usr/bin/env python3
"""FTFT2: actual async Application loop + existing production regressions.

No fluid prototype execution or modification. Driver watchdogs detect hangs;
they are not responsiveness acceptance thresholds. All evidence is persistent;
compiled output and generated project inputs stay in ignored build/.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
PROTECTED = ['prototypes/fluid_coupling/r1_p1', 'docs/evidence/ftft1',
             'src/WorldState.cpp', 'src/WorldState.h', 'src/SceneFingerprint.cpp',
             'src/SceneFingerprint.h', 'src/RuntimeWorld.cpp', 'src/RuntimeWorld.h']


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT, text=True).strip()


def fingerprints():
    paths = [ROOT / 'CMakeLists.txt']
    for directory in ['src', 'tests', 'scripts']:
        paths += [p for p in (ROOT / directory).rglob('*')
                  if p.is_file() and p.suffix in ['.h', '.cpp', '.py']]
    return {str(p.relative_to(ROOT)): sha(p) for p in sorted(paths)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', default='docs/evidence/ftft2/final')
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args()
    out = (ROOT / args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith('JUDAS_')}
    env.update(SDL_VIDEODRIVER='offscreen', LIBGL_ALWAYS_SOFTWARE='1')
    result = {
        'scope': 'FTFT2; actual asynchronous Application loop; no prototype runs',
        'head': git('rev-parse', 'HEAD'), 'initial_git_status': git('status', '--short'),
        'prototype_tree': git('rev-parse', 'HEAD:prototypes/fluid_coupling/r1_p1'),
        'renderer_environment': {k: env[k] for k in ['SDL_VIDEODRIVER', 'LIBGL_ALWAYS_SOFTWARE']},
        'cleared_environment_prefix': 'JUDAS_',
        'source_sha256_before': fingerprints(), 'runs': [], 'checks': {},
        'human_visual_validation': 'NOT RUN; headless GL/readback and machine assertions only',
    }
    begin = time.monotonic()

    def save():
        (out / 'results.json').write_text(json.dumps(result, indent=2) + '\n')

    def run(name, command, extra_env=None, timeout=600, scope='acceptance'):
        actual_env = dict(env, **(extra_env or {}))
        start = time.monotonic()
        log = out / (name + '.log')
        with log.open('w') as stream:
            try:
                process = subprocess.run(command, cwd=ROOT, env=actual_env,
                                         stdout=stream, stderr=subprocess.STDOUT, timeout=timeout)
                code = process.returncode
            except subprocess.TimeoutExpired:
                code = 'TIMEOUT'
        text = log.read_text(errors='replace')
        entry = {'name': name, 'command': [str(x) for x in command], 'exit_code': code,
                 'seconds': time.monotonic() - start, 'scope': scope,
                 'environment_overrides': extra_env or {}, 'log': str(log.relative_to(ROOT)),
                 'passing_lines': len(re.findall(r'^\s*(?:PASS|OK)\s+', text, re.M)),
                 'failing_lines': len(re.findall(r'^\s*FAIL\s+', text, re.M))}
        executable = Path(command[0])
        if not executable.is_absolute():
            executable = ROOT / executable
        if executable.is_file():
            entry['executable_sha256'] = sha(executable)
        result['runs'].append(entry)
        save()
        print(f"{name}: exit={code}, {entry['seconds']:.3f}s", flush=True)
        return entry, text

    protected_before = git('status', '--porcelain', '--', *PROTECTED)
    if protected_before:
        raise SystemExit('Protected source/evidence has local changes; refusing validation: ' + protected_before)
    for name, command in [('configure', ['cmake', '-S', '.', '-B', 'build']),
                          ('build', ['cmake', '--build', 'build', '-j', str(args.jobs)])]:
        item, _ = run(name, command, timeout=1200)
        if item['exit_code'] != 0:
            raise SystemExit('Build failed; no stale binaries are accepted. See ' + item['log'])

    integration = out / 'integration'
    integration.mkdir(exist_ok=True)
    item, text = run('async_application', ['./build/judas_async_application_tests', '--output', str(integration)])
    result['integration_cases'] = [
        {'name': m[0], 'status': m[1], 'frames': int(m[2]), 'seconds': float(m[3])}
        for m in re.findall(r'^CASE (\S+) (PASS|FAIL) frames=(\d+) seconds=([\d.]+)$', text, re.M)]
    result['checks']['integration_all_12_cases'] = (
        item['exit_code'] == 0 and len(result['integration_cases']) == 12
        and all(c['status'] == 'PASS' for c in result['integration_cases']))
    summary = re.search(r'FTFT2 integration: (\d+) checks, (\d+) failures', text)
    result['integration_assertions'] = {'checks': int(summary[1]), 'failures': int(summary[2])} if summary else None

    targets = re.findall(r'add_executable\((judas_\w+_tests)\b', (ROOT / 'CMakeLists.txt').read_text())
    targets = [t for t in targets if t != 'judas_async_application_tests']
    result['production_suite_count'] = len(targets)
    for target in targets:
        run(target, ['./build/' + target])

    # Existing editor automation, real editor loop/default async. Generated
    # assets are copied so its authoring/saving never touches shipped content.
    project_root = ROOT / 'build/ftft2-validation/project'
    if project_root.exists():
        shutil.rmtree(project_root)
    shutil.copytree(ROOT / 'build/ftft2-fixtures/shared-progress/a', project_root)
    project = project_root / 'main.judasproj'
    scene = project_root / 'Scenes/main.judas'
    before = sha(scene)
    prefix = out / 'editor'
    item, text = run('editor_async', ['./build/judas_editor', str(project)],
                     {'JUDAS_EDITOR_AUTOTEST': str(prefix)}, scope='existing editor default-async Play/Stop integration')
    body = re.search(r'play frame: .*? (\d+) bodies,', text)
    resources = re.search(r'resources: (\d+) meshes, .*? uploads (\d+),', text)
    jobs = re.search(r'jobs: (\d+) workers, (\d+) completed,', text)
    saved = Path(str(prefix) + '.saved.judas')
    result['checks']['editor_play_stop_async'] = bool(
        item['exit_code'] == 0 and body and int(body[1]) > 0
        and resources and int(resources[1]) > 0 and int(resources[2]) > 0
        and jobs and int(jobs[1]) > 0 and int(jobs[2]) > 0
        and 'undo restores: IDENTICAL' in text
        and 'authored scene after play/stop is IDENTICAL' in text
        and '[editor autotest] save ok' in text
        and before == sha(scene) and saved.exists() and sha(saved) == before)

    # Blocking reference intentionally retained. No async claim for this run.
    script = out / 'three_steps.txt'
    script.write_text('STEPS 3\nLOG_EVERY 1\n')
    item, text = run('runtime_blocking_reference', ['./build/judas', str(project)],
                     {'JUDAS_TEST_SCRIPT': str(script)}, scope='legacy scripted blocking reference; NOT async evidence')
    result['checks']['blocking_reference_three_steps'] = (
        item['exit_code'] == 0 and len(re.findall(r'^[012],', text, re.M)) == 3
        and before == sha(scene))

    # Reuse M31 load/performance evidence. Its unconditional success return is
    # deliberately NOT accepted as an invariant proof; the new suite supplies it.
    run('m31_resource_stress', ['./build/judas_resource_stress', str(out / 'm31_stress.json')],
        scope='supporting M31 real-GL load/performance reference; NOT deterministic race acceptance')

    result['source_sha256_after'] = fingerprints()
    result['checks']['tested_sources_unchanged_during_run'] = result['source_sha256_before'] == result['source_sha256_after']
    result['checks']['protected_paths_unchanged'] = not git('status', '--porcelain', '--', *PROTECTED)
    result['final_git_status'] = git('status', '--short')
    result['seconds'] = time.monotonic() - begin
    result['overall_pass'] = all(result['checks'].values()) and all(
        r['exit_code'] == 0 and r['failing_lines'] == 0 for r in result['runs'])
    result['evidence_sha256'] = {str(p.relative_to(out)): sha(p) for p in sorted(out.rglob('*'))
                                if p.is_file() and p != out / 'results.json'}
    save()
    print(json.dumps({'overall_pass': result['overall_pass'], 'checks': result['checks'],
                      'seconds': result['seconds']}, indent=2), flush=True)
    raise SystemExit(0 if result['overall_pass'] else 1)


if __name__ == '__main__':
    main()
