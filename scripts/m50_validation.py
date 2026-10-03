#!/usr/bin/env python3
"""Focused JudasJS documentation/example verification; no production-suite run.

Node and TypeScript are developer tools, not dependencies of Judas games.
Output and disposable ordinary projects stay in the ignored build directory.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def run(output, name, command, extra=None):
    env = {k: v for k, v in os.environ.items() if not k.startswith('JUDAS_')}
    env.update(SDL_VIDEODRIVER='offscreen', LIBGL_ALWAYS_SOFTWARE='1',
               SDL_AUDIODRIVER='dummy', JUDAS_WORLD_STATE='none')
    env.update(extra or {})
    start = time.monotonic()
    result = subprocess.run([str(p) for p in command], cwd=ROOT, env=env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, timeout=240)
    (output / (name + '.log')).write_text(result.stdout)
    if result.returncode:
        raise RuntimeError(f'{name} failed ({result.returncode}); see {output / (name + ".log")}')
    return {'name': name, 'exit_code': result.returncode,
            'seconds': time.monotonic() - start}, result.stdout


def application_smokes(output):
    steps = output / 'steps.txt'
    steps.write_text('STEPS 3\nLOG_EVERY 1\n')
    startup, text = run(output, 'current-project',
                        [ROOT / 'build/judas', ROOT / 'projects/character_demo/character_demo.judasproj'],
                        {'JUDAS_TEST_SCRIPT': str(steps)})
    if 'script asset=' in text:
        raise RuntimeError('Current project script fault; inspect log')

    # The copyable source stays byte-for-byte unchanged. This normal relative
    # subclass only reports completion and quits the disposable application.
    fixture = output / 'scene-project'
    shutil.copytree(ROOT / 'projects/character_demo', fixture)
    scripts = fixture / 'Assets/scripts'
    shutil.copyfile(ROOT / 'docs/judasjs/examples/scene-session.js', scripts / 'scene-session.js')
    (scripts / 'scene-session.js.judasmeta').write_text(
        'JudasAssetMeta 1\nid "50505050505050505050505050505001"\ntype script\nsource ""\n')
    (scripts / 'controller.js').write_text('''import Base from './scene-session.js';
import {session, console, ui} from 'judas';
export const properties = {controlled:{type:'boolean',default:true}};
export default class extends Base {
  constructor(context){super();this.props=context.properties;}
  start(){if(this.props.controlled){super.start();console.log('M50_VISITS',session.get('example_visits'));}}
  uiUpdate(){if(!this.props.controlled)return;
    if(session.get('example_visits')===2){console.log('M50_SCENE_RELOAD_SESSION_PASS');ui.quit();}
    else super.uiUpdate();
  }
}
''')
    # Run the ordinary outer frame loop: the deterministic step runner does not
    # call uiUpdate or apply scene transitions. subprocess timeout bounds this.
    reload, text = run(output, 'scene-reload',
                       [ROOT / 'build/judas', fixture / 'character_demo.judasproj'])
    if 'M50_SCENE_RELOAD_SESSION_PASS' not in text or 'script asset=' in text:
        raise RuntimeError('Scene/session example did not reconstruct and retain session state; inspect log')
    return [startup, reload]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--node', default=shutil.which('node'), help='Node executable')
    parser.add_argument('--typescript', default=os.environ.get('JUDAS_TYPESCRIPT'),
                        help='Optional path to installed/temporary TypeScript lib/typescript.js')
    parser.add_argument('--output', type=Path, default=ROOT / 'build/m50-validation')
    args = parser.parse_args()
    if not args.node:
        parser.error('Node is unavailable; supply --node. Games do not require Node.')
    output = args.output.resolve()
    if output.exists():
        parser.error('Output already exists; choose a fresh --output to preserve earlier results.')
    output.mkdir(parents=True)
    results = []
    results.append(run(output, 'configure', ['cmake', '-S', ROOT, '-B', ROOT / 'build',
                                           '-DCMAKE_BUILD_TYPE=Release'])[0])
    results.append(run(output, 'build', ['cmake', '--build', ROOT / 'build', '--target',
                                       'judasjs_examples_tests', 'judas', '-j4'])[0])
    example_output = output / 'examples'
    results.append(run(output, 'examples', [ROOT / 'build/judasjs_examples_tests', example_output])[0])
    command = [args.node, ROOT / 'scripts/check_judasjs_api.mjs', '--runtime', example_output / 'surface.json']
    if args.typescript:
        command += ['--typescript', args.typescript]
    coverage, text = run(output, 'api-coverage', command)
    results.append(coverage)
    results += application_smokes(output)
    summary = {'pass': True, 'head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
               'coverage': json.loads(text), 'commands': results,
               'scope': 'Focused developer tooling/examples and ordinary application startup/reload; no production suite.'}
    (output / 'RESULTS.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
