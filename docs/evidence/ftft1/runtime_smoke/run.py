#!/usr/bin/env python3
"""Real executable FTFT1 startup/Play smoke. No engine or editor code substitutes.

The existing editor autotest reports an exit status even when Play fails. This
script therefore records its frame-140 body/step diagnostics and screenshot,
and labels visual status review separately from automated checks. Scripted
standalone runs intentionally use the existing blocking resource reference path;
these are persistence evidence, not FTFT2 asynchronous integration evidence.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
SOURCE_SCENE = ROOT / 'projects/tiny_game/Scenes/main.judas'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def execute(command, env, log):
    start = time.monotonic()
    with log.open('w') as output:
        try:
            result = subprocess.run(command, cwd=ROOT, env=env, stdout=output,
                                    stderr=subprocess.STDOUT, timeout=60)
            code = result.returncode
        except subprocess.TimeoutExpired:
            code = 'TIMEOUT'
    return {'command': [str(x) for x in command], 'exit_code': code,
            'seconds': time.monotonic() - start, 'log': str(log.relative_to(ROOT))}

def main():
    generated_tool = ROOT / 'build/ftft1_smoke_fingerprint'
    compile_cmd = ['c++', '-std=c++17', '-Isrc', str(OUT/'fingerprint_tool.cpp'),
                   'src/SceneFingerprint.cpp', 'src/Scene.cpp', 'src/SceneSerialization.cpp',
                   '-o', str(generated_tool)]
    compiled = execute(compile_cmd, os.environ.copy(), OUT/'compile.log')
    if compiled['exit_code'] != 0:
        raise RuntimeError(compiled)
    fingerprint = subprocess.check_output([str(generated_tool), str(SOURCE_SCENE)], cwd=ROOT, text=True).strip()
    base = 'JudasWorldState 2\ncompatibility 1 sha256 {}\nbaseline "Tiny game"\nnext-runtime-id 4611686018427387904\n'
    variants = {
        'valid': base.format(fingerprint),
        'mismatch': base.format('0'*64),
        'legacy': 'JudasWorldState 1\nbaseline "Tiny game"\nnext-runtime-id 4611686018427387904\n',
    }
    env = os.environ.copy()
    for key in list(env):
        if key.startswith('JUDAS_'):
            del env[key]
    env.update(SDL_VIDEODRIVER='offscreen', LIBGL_ALWAYS_SOFTWARE='1', JUDAS_RESOURCE_MODE='blocking')
    script = OUT/'three_steps.txt'
    script.write_text('STEPS 3\nLOG_EVERY 1\n')
    result = {'scope': 'FTFT1 real application persistence startup/Play; no asynchronous claim',
              'renderer_environment': {k: env[k] for k in ['SDL_VIDEODRIVER', 'LIBGL_ALWAYS_SOFTWARE', 'JUDAS_RESOURCE_MODE']},
              'scene_source': str(SOURCE_SCENE.relative_to(ROOT)), 'source_scene_before': sha(SOURCE_SCENE),
              'fingerprint': fingerprint, 'compile': compiled,
              'binary_sha256': {exe: sha(ROOT/'build'/exe) for exe in ['judas', 'judas_editor']}, 'cases': []}
    for name, save_text in variants.items():
        case = OUT/name
        for directory in ['Scenes','Saves','Assets']:
            (case/directory).mkdir(parents=True, exist_ok=True)
        scene = case/'Scenes/main.judas'
        shutil.copyfile(SOURCE_SCENE, scene)
        project = case/'smoke.judasproj'
        project.write_text('JudasProject 1\nname "FTFT1 smoke"\nstartup-scene "Scenes/main.judas"\nassets-dir "Assets"\nscenes-dir "Scenes"\nsaves-dir "Saves"\n')
        state = case/'Saves/main.judasstate'
        state.write_text(save_text)
        before = {'scene':sha(scene), 'save':sha(state), 'project':sha(project)}
        runtime_env = env.copy()
        runtime_env['JUDAS_TEST_SCRIPT'] = str(script)
        runtime = execute([str(ROOT/'build/judas'), str(project)], runtime_env, case/'runtime.log')
        runtime_text = (case/'runtime.log').read_text()
        expected_code = 0 if name == 'valid' else 1
        expected_message = '(loaded)' if name == 'valid' else ('authored baseline fingerprint mismatch' if name == 'mismatch' else 'legacy saves have no verifiable baseline fingerprint')
        runtime['acceptance'] = runtime['exit_code'] == expected_code and expected_message in runtime_text
        editor_env = env.copy()
        editor_env['JUDAS_EDITOR_AUTOTEST'] = str(case/'editor')
        editor = execute([str(ROOT/'build/judas_editor'), str(project)], editor_env, case/'editor.log')
        editor_text = (case/'editor.log').read_text()
        match = re.search(r'play frame: (\d+) steps, step [^,]+ ms, (\d+) bodies', editor_text)
        editor['frame_140_steps'] = int(match.group(1)) if match else None
        editor['frame_140_bodies'] = int(match.group(2)) if match else None
        editor['authored_restored'] = 'authored scene after play/stop is IDENTICAL' in editor_text
        editor['diagnostic_acceptance'] = editor['exit_code'] == 0 and editor['authored_restored'] and match is not None and ((int(match.group(2)) > 0) if name == 'valid' else (int(match.group(2)) == 0))
        editor['status_visual_review'] = 'REQUIRED: inspect editor.play.png; exit 0 alone does not establish Play acceptance/rejection'
        after = {'scene':sha(scene), 'save':sha(state), 'project':sha(project)}
        result['cases'].append({'case':name, 'before':before, 'after':after, 'files_unchanged':before==after,
                                'runtime':runtime, 'editor':editor})
        print(name, 'runtime=',runtime['acceptance'], 'editor diagnostics=',editor['diagnostic_acceptance'], 'files unchanged=',before==after, flush=True)
    result['binary_sha256_after'] = {exe: sha(ROOT/'build'/exe) for exe in ['judas', 'judas_editor']}
    result['binaries_unchanged_during_runs'] = result['binary_sha256_after'] == result['binary_sha256']
    result['source_scene_after'] = sha(SOURCE_SCENE)
    result['source_scene_unchanged'] = result['source_scene_before'] == result['source_scene_after']
    result['automated_acceptance'] = result['source_scene_unchanged'] and result['binaries_unchanged_during_runs'] and all(c['files_unchanged'] and c['runtime']['acceptance'] and c['editor']['diagnostic_acceptance'] for c in result['cases'])
    (OUT/'results.json').write_text(json.dumps(result, indent=2)+'\n')
    print('Automated acceptance:', result['automated_acceptance'], flush=True)
    return 0 if result['automated_acceptance'] else 1

if __name__ == '__main__':
    raise SystemExit(main())
