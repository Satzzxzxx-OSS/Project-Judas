#!/usr/bin/env python3
"""FTFT4A-P: source-matched sequential engine benchmarks and protected regressions.

Run modes separately so production edits/builds never overlap timing trials:
  --build-references   (HEAD and persistent correct-but-slow snapshot)
  --components         (supplied scalar checks; supporting evidence only)
  --build-optimized    (after candidate source is frozen)
  --build-scale-wrappers (unchanged TestScaleStatistics + whole-fixture wall timer)
  --benchmarks         (7 cyclic-interleaved OLD/SLOW/OPT trials, no workers)
  --regressions        (unchanged production acceptance + near/far traces)
Each mode refuses to overwrite its evidence. No source checkout/reset, binary
in evidence, physics substitute, profiler build, or protected-evidence output.
"""
import argparse
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
import zipfile

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/evidence/ftft4/performance'
BUILD = ROOT / 'build/ftft4p'
HEAD = '3404388f1af48419505c802fc11dbf8a0b577f38'
COMPILER = '/usr/bin/c++'
FLAGS = ['-O3', '-DNDEBUG', '-std=gnu++17', '-Wall', '-Wextra', '-ffp-contract=off']
UNITS = ['RigidBody.cpp', 'RigidBodyGravity.cpp', 'Contacts.cpp', 'ContactSolver.cpp',
         'RadialTerrain.cpp', 'Broadphase.cpp', 'Narrowphase.cpp', 'PhysicsWorld.cpp']
LABELS = ['old', 'slow', 'optimized']
MIXED = ROOT / 'tests/ContactPerformanceTests.cpp'
SCALE = re.compile(r'(\d+) bodies, (\d+) possible pairs, (\d+) candidates, (\d+) colliding pairs, (\d+) points, tree height (\d+), ([\d.]+) ms/step \(broadphase ([\d.]+), narrowphase ([\d.]+), solver ([\d.]+)\)')


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + '\n')


def load(path):
    return json.loads(Path(path).read_text())


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


BASE = module('ftft4p_base', ROOT / 'scripts/ftft4_validation.py')


def checked_protection():
    result = BASE.protection()
    snapshot = load(OUT / 'pre-optimization.json')
    expected = dict(snapshot['protected_sha256'])
    # The new snapshot stores protected evidence separately from source; the
    # prior runner protects both. Compare the full union without omitting either.
    for name in result['sha256']:
        if name not in expected:
            if name in snapshot['source_sha256']:
                expected[name] = snapshot['source_sha256'][name]
            else:
                expected[name] = hashlib.sha256(subprocess.check_output(['git', 'show', HEAD + ':' + name], cwd=ROOT)).hexdigest()
    if result['status'] or result['sha256'] != expected:
        raise RuntimeError('Protected FTFT1--3/prototype files differ from pre-optimization snapshot')
    return result


def execute(name, command, directory, timeout=1200):
    return BASE.execute(name, [str(x) for x in command], directory, timeout=timeout)


def required(name, command, directory, timeout=1200):
    entry, text = execute(name, command, directory, timeout)
    if entry['exit_code'] != 0:
        save(directory / (name + '-failure.json'), entry)
        raise RuntimeError('Command failed: ' + entry['log'])
    return entry, text


def source_hashes(root):
    paths = sorted((root / 'src').glob('*.h')) + [root / 'src' / n for n in UNITS]
    paths += [root / 'tests/BroadphaseTests.cpp']
    return {str(p.relative_to(root)): sha(p) for p in paths}


def extract(archive, destination):
    destination.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as z:
        for name in z.namelist():
            if name.startswith('/') or '..' in Path(name).parts:
                raise RuntimeError('Unsafe archive member: ' + name)
        z.extractall(destination)


def scale_body(text):
    start = text.index('void TestScaleStatistics() {')
    body = text[start:text.index('\n}\n', start) + 3]
    # Only independent diagnostic output can differ. Setup, updates, timer
    # reads and assertions are otherwise required to match byte-for-byte.
    body = re.sub(r'    std::printf\("    geometry predicates .*?;\n', '', body, flags=re.S)
    body = re.sub(r'    std::printf\(" +cache .*?;\n', '', body, flags=re.S)
    return body


def build_variant(label, root, directory):
    destination = BUILD / label
    destination.mkdir(parents=True, exist_ok=True)
    before = source_hashes(root)
    entry = {'label': label, 'source_root': str(root), 'source_sha256': before,
             'mixed_driver_sha256': sha(MIXED), 'compiler': COMPILER, 'flags': FLAGS,
             'commands': [], 'binaries': {}}
    objects = []
    for unit in UNITS:
        obj = destination / (unit + '.o')
        record, _ = required(label + '-compile-' + unit, [COMPILER, *FLAGS, '-I' + str(root / 'src'),
                              '-c', root / 'src' / unit, '-o', obj], directory)
        entry['commands'].append(record)
        objects.append(obj)
    for name, driver in [('broadphase', root / 'tests/BroadphaseTests.cpp'), ('mixed', MIXED)]:
        binary = destination / name
        record, _ = required(label + '-link-' + name, [COMPILER, *FLAGS, '-I' + str(root / 'src'),
                              driver, *objects, '-o', binary], directory)
        entry['commands'].append(record)
        entry['binaries'][name] = {'path': str(binary), 'sha256': sha(binary)}
    entry['sources_unchanged'] = before == source_hashes(root) and entry['mixed_driver_sha256'] == sha(MIXED)
    if not entry['sources_unchanged']:
        raise RuntimeError('Source changed during ' + label + ' build; results unusable')
    (directory / (label + '-scale-fixture.cpp.txt')).write_text(scale_body((root / 'tests/BroadphaseTests.cpp').read_text()))
    return entry


def build_references(directory):
    if BASE.git('rev-parse', 'HEAD') != HEAD:
        raise RuntimeError('HEAD differs from reviewed historical baseline')
    archive = OUT / 'old-production-source.zip'
    records = []
    if not archive.exists():
        record, _ = required('archive-old', ['git', 'archive', '--format=zip', '--output=' + str(archive),
                                             HEAD, 'src', 'tests/BroadphaseTests.cpp'], directory)
        records.append(record)
    extract(archive, BUILD / 'old-source')
    extract(OUT / 'pre-optimization-source.zip', BUILD / 'slow-source')
    versions = {label: build_variant(label, BUILD / (label + '-source'), directory) for label in ['old', 'slow']}
    same = (directory / 'old-scale-fixture.cpp.txt').read_text() == (directory / 'slow-scale-fixture.cpp.txt').read_text()
    return {'versions': versions, 'archive_commands': records,
            'archive_sha256': {'old': sha(archive), 'slow': sha(OUT / 'pre-optimization-source.zip')},
            'original_scale_fixture_unchanged': same, 'overall_pass': same}


def build_optimized(directory):
    result = build_variant('optimized', ROOT, directory)
    same = (directory / 'optimized-scale-fixture.cpp.txt').read_text() == (OUT / 'reference-builds/old-scale-fixture.cpp.txt').read_text()
    return {'versions': {'optimized': result}, 'original_scale_fixture_unchanged': same, 'overall_pass': same}


def build_scale_wrappers(directory):
    versions = dict(load(OUT / 'reference-builds/results.json')['versions'])
    versions.update(load(OUT / 'optimized-build/results.json')['versions'])
    driver = ROOT / 'tests/ContactScalePerformance.cpp'
    records = {}
    for label, version in versions.items():
        root = Path(version['source_root'])
        if version['source_sha256'] != source_hashes(root):
            raise RuntimeError('Engine source changed before scale wrapper: ' + label)
        binary = BUILD / label / 'scale'
        objects = [BUILD / label / (unit + '.o') for unit in UNITS]
        record, _ = required(label + '-scale-wrapper', [COMPILER, *FLAGS, '-I' + str(root / 'src'),
                              '-DFTFT4P_BROADPHASE_SOURCE="' + str(root / 'tests/BroadphaseTests.cpp') + '"',
                              driver, *objects, '-o', binary], directory)
        records[label] = {'command': record, 'path': str(binary), 'sha256': sha(binary),
                          'source_sha256': version['source_sha256']}
    return {'versions': records, 'wrapper_sha256': sha(driver), 'flags': FLAGS, 'overall_pass': True}


def components(directory):
    handoff = OUT / 'handoff/work/code'
    fixtures = ROOT / 'docs/evidence/ftft4/geometry/fixtures.txt'
    records = []
    destination = BUILD / 'components'; destination.mkdir(parents=True, exist_ok=True)
    for name in ['arithmetic_review', 'sat_review', 'factored_fixture_check', 'bounds_cache_review']:
        binary = destination / name
        compile_record, _ = required(name + '-build', [COMPILER, *FLAGS, handoff / (name + '.cpp'), '-o', binary], directory)
        command = [binary] + ([fixtures] if name in ['arithmetic_review', 'factored_fixture_check'] else [])
        run_record, text = required(name, command, directory)
        data = [json.loads(line) for line in text.splitlines() if line.startswith('{')]
        records.append({'name': name, 'compile': compile_record, 'run': run_record, 'records': data})
    return {'scope': 'SUPPORTING scalar method references only; storage value_carriers stays inside supplied research, never engine dependency',
            'checks': records, 'overall_pass': True}


def summary(values):
    return {'median': statistics.median(values), 'min': min(values), 'max': max(values), 'trials': len(values)}


def benchmark(directory, repetitions):
    versions = dict(load(OUT / 'reference-builds/results.json')['versions'])
    versions.update(load(OUT / 'optimized-build/results.json')['versions'])
    if len({v['mixed_driver_sha256'] for v in versions.values()}) != 1:
        raise RuntimeError('Mixed driver differs among builds')
    for label, v in versions.items():
        if v['flags'] != FLAGS or v['compiler'] != COMPILER:
            raise RuntimeError('Compiler/flags differ: ' + label)
        for binary in v['binaries'].values():
            if sha(binary['path']) != binary['sha256']:
                raise RuntimeError('Built executable changed: ' + binary['path'])
    if versions['optimized']['source_sha256'] != source_hashes(ROOT):
        raise RuntimeError('Current engine source differs from optimized executable')
    wrappers = load(OUT / 'scale-builds/results.json')
    if wrappers['wrapper_sha256'] != sha(ROOT / 'tests/ContactScalePerformance.cpp') or wrappers['flags'] != FLAGS:
        raise RuntimeError('Scale wrapper source/flags changed')
    for label, wrapper in wrappers['versions'].items():
        if sha(wrapper['path']) != wrapper['sha256'] or wrapper['source_sha256'] != versions[label]['source_sha256']:
            raise RuntimeError('Scale wrapper executable/source mismatch: ' + label)
    records = []; traces = {}
    for trial in range(repetitions):
        # Rotating which variant runs first reduces persistent order bias.
        order = LABELS[trial % 3:] + LABELS[:trial % 3]
        for label in order:
            v = versions[label]
            item, text = required(f'{trial+1:02d}-{label}-scale', [wrappers['versions'][label]['path']], directory)
            matches = SCALE.findall(text)
            if len(matches) != 1 or item['failing_lines']:
                raise RuntimeError('Original broadphase acceptance or timer row failed')
            row = matches[0]
            item.update(label=label, trial=trial+1, fixture='1500_crates',
                        workload=dict(zip(['bodies','possible_pairs','candidates','colliding_pairs','points','tree_height'], map(int,row[:6]))),
                        timings=dict(zip(['total_ms','broadphase_ms','narrowphase_ms','solver_ms'],map(float,row[6:]))))
            wall = re.findall(r'scale fixture wall ([\d.]+) ms', text)
            if len(wall) != 1:
                raise RuntimeError('Missing whole scale-fixture timer')
            item['timings']['whole_fixture_ms'] = float(wall[0])
            g = re.findall(r'geometry predicates (\d+), exact fallbacks (\d+), unresolved (\d+)',text)
            item['geometry'] = dict(zip(['predicates','exact_fallbacks','unresolved'],map(int,g[0]))) if len(g)==1 else None
            item['cache_diagnostic_lines'] = [line.strip() for line in text.splitlines() if line.strip().startswith('cache ')]
            if label != 'old' and (item['geometry'] is None or item['geometry']['unresolved']):
                raise RuntimeError('Missing or unresolved production geometry counters')
            cache = re.findall(r'cache orientation (\d+) hits (\d+) rebuilds; bounds (\d+) hits (\d+) rebuilds; shapes (\d+); solver frames (\d+); bytes (\d+); allocations (\d+)', text)
            item['cache'] = dict(zip(['orientation_hits','orientation_rebuilds','bound_hits','bound_rebuilds','shape_rebuilds','solver_frames','cache_bytes','cache_allocations'],map(int,cache[0]))) if len(cache)==1 else None
            if label == 'optimized' and item['cache'] is None:
                raise RuntimeError('Missing optimized scale cache diagnostics')
            records.append(item)
            command = [v['binaries']['mixed']['path']] + (['--trace'] if trial == 0 else [])
            item, text = required(f'{trial+1:02d}-{label}-mixed', command, directory)
            rows = [json.loads(line) for line in text.splitlines() if line.startswith('{')]
            states = [r for r in rows if r['kind']=='state']
            workloads = [r for r in rows if r['kind']=='workload']
            if len(workloads)!=4 or not all(r['pass'] for r in workloads):
                raise RuntimeError('Mixed engine fixture failed')
            if label!='old' and not all(r['geometry_available'] for r in workloads):
                raise RuntimeError('Missing mixed geometry counters')
            if trial == 0: traces[label]=states
            item.update(label=label,trial=trial+1,fixture='mixed',workloads=workloads)
            records.append(item)
            save(directory / 'partial.json', {'records': records})
    timings = {}; stable = {}
    keys=['total_ms','broadphase_ms','narrowphase_ms','solver_ms']
    for label in LABELS:
        scale=[r for r in records if r['label']==label and r['fixture']=='1500_crates']
        mixed=[w for r in records if r['label']==label and r['fixture']=='mixed' for w in r['workloads']]
        timings[label]={'1500_crates':{k:summary([r['timings'][k] for r in scale]) for k in keys+['whole_fixture_ms']}}
        for name in sorted({r['fixture'] for r in mixed}):
            rows=[r for r in mixed if r['fixture']==name]
            timings[label][name]={k:summary([r[k] for r in rows]) for k in keys+['external_frame_ms','setup_ms']}
            stable[label+'/'+name]=len({r['state_checksum'] for r in rows})==1
    differences={}
    slow={(r['fixture'],r['step']):r for r in traces['slow']}
    for row in traces['optimized']:
        name=row['fixture']; previous=slow[(name,row['step'])]
        result=differences.setdefault(name,{'sampled_steps':0,'first_changed_sample':None,'max_position':0.,'max_quaternion':0.,'max_velocity':0.,'max_angular_velocity':0.,'changed_scalar_count':0})
        result['sampled_steps']+=1
        if len(previous['bodies'])!=len(row['bodies']): raise RuntimeError('Body count changed')
        for a,b in zip(previous['bodies'],row['bodies']):
            if a[0]!=b[0]: raise RuntimeError('Body identity changed')
            for i,(x,y) in enumerate(zip(a[1:],b[1:])):
                delta=abs(x-y)
                key='max_position' if i<3 else 'max_quaternion' if i<7 else 'max_velocity' if i<10 else 'max_angular_velocity'
                result[key]=max(result[key],delta)
                if delta:
                    result['changed_scalar_count']+=1
                    if result['first_changed_sample'] is None:result['first_changed_sample']=row['step']
    return {'versions':versions,'scale_wrappers':wrappers,'compiler_version':subprocess.check_output([COMPILER,'--version'],text=True),
            'flags':FLAGS,'trials_per_variant':repetitions,'interleaving':'cyclic permutation OLD/SLOW/OPT; one child at a time; no benchmark workers',
            'timer_scope':'unchanged TestScaleStatistics final-step timer; mixed includes all 120 steps and external ResetBody/forces; setup/cache creation reported separately',
            'records':records,'timings':timings,'deterministic_full_trajectory_checksums':stable,
            'slow_optimized_trajectory_differences':differences,
            'trajectory_disposition':'report any differences; no automatic acceptance/rebaseline based on timing',
            'overall_pass':all(stable.values())}


def regressions(directory,jobs):
    # Configure the same arithmetic contract before the unchanged runner.
    configure,_=required('configure-contract',['cmake','-S','.', '-B','build','-DCMAKE_CXX_FLAGS=-ffp-contract=off'],directory)
    before=BASE.fingerprints()
    reuse,production=BASE.reuse_ftft2(directory,jobs)
    checks={'full_production_and_async':reuse['overall_pass']}
    if not checks['full_production_and_async']:
        return {'configure':configure,'ftft2_reuse':reuse,'checks':checks,'overall_pass':False}
    ftft1=BASE.reuse_ftft1(directory);checks['ftft1_smoke']=ftft1['overall_pass']
    gravity,_=required('gravity-explicit',['./build/judas_uniform_gravity_scene_tests','--output',str(directory/'gravity-explicit')],directory)
    counts=load(directory/'gravity-explicit/results.json')
    checks['ftft3_construction']=counts['failures']==0 and counts['scenes']==136 and counts['runtime_constructions']==272 and counts['fixed_steps']==32640
    player,text=required('independent-player',['./build/judas_contact_geometry_tests','--player'],directory)
    player_rows=[json.loads(line) for line in text.splitlines() if line.startswith('{')]
    checks['21_player_sweeps']=len(player_rows)==21 and all(r['pass'] for r in player_rows)
    walks,payloads=BASE.walks(directory/'harness');checks.update(walks['checks'])
    comparison={}
    for name,after in payloads.items():
        before_text=(ROOT/'docs/evidence/ftft4/final/harness'/(name+'.csv')).read_text()
        oldrows,newrows=before_text.splitlines(),after.splitlines()
        differences={};first=None
        if oldrows[0]!=newrows[0] or len(oldrows)!=len(newrows):raise RuntimeError('Harness schema changed')
        for old,new in zip(oldrows[1:],newrows[1:]):
            for column,a,b in zip(oldrows[0].split(','),old.split(','),new.split(',')):
                delta=abs(float(a)-float(b))
                differences[column]=max(differences.get(column,0),delta)
                if delta and first is None:first=int(new.split(',')[0])
        comparison[name]={'byte_identical':before_text==after,'first_numeric_difference_step':first,'max_absolute_difference_by_column':differences}
    checks['source_unchanged_during_regressions']=before==BASE.fingerprints()
    return {'configure':configure,'ftft2_reuse':reuse,'production_suite_count':production.get('production_suite_count'),
            'integration_assertions':production.get('integration_assertions'),'ftft1_smoke':ftft1,'gravity':gravity,'gravity_counts':counts,
            'player_sweeps':player,'player_cases':player_rows,'walks':walks,'slow_optimized_harness_comparison':comparison,
            'source_sha256':before,'checks':checks,'overall_pass':all(checks.values()),
            'human_visual_validation':'NOT RUN; offscreen real-GL automated evidence only'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    modes=parser.add_mutually_exclusive_group(required=True)
    for option in ['build-references','components','build-optimized','build-scale-wrappers','benchmarks','regressions']:modes.add_argument('--'+option,action='store_true')
    parser.add_argument('--jobs',type=int,default=4)
    parser.add_argument('--trials',type=int,default=7)
    args=parser.parse_args()
    name=next(n for n in ['build_references','components','build_optimized','build_scale_wrappers','benchmarks','regressions'] if getattr(args,n))
    output_name={'build_references':'reference-builds','build_optimized':'optimized-build','build_scale_wrappers':'scale-builds'}.get(name,name)
    directory=OUT/output_name
    if (directory/'results.json').exists():raise SystemExit('Refusing to overwrite evidence: '+str(directory))
    directory.mkdir(parents=True,exist_ok=True)
    before=checked_protection();start=time.monotonic()
    if name=='build_references':result=build_references(directory)
    elif name=='build_optimized':result=build_optimized(directory)
    elif name=='build_scale_wrappers':result=build_scale_wrappers(directory)
    elif name=='components':result=components(directory)
    elif name=='benchmarks':
        if args.trials!=7:raise SystemExit('This review requires exactly seven interleaved trials')
        result=benchmark(directory,args.trials)
    else:result=regressions(directory,args.jobs)
    result.update(mode=name,head=BASE.git('rev-parse','HEAD'),seconds=time.monotonic()-start,
                  protected_before=before,protected_after=checked_protection(),runner_sha256=sha(__file__),git_status=BASE.git('status','--short'))
    result['overall_pass']=result['overall_pass'] and result['protected_before']==result['protected_after']
    save(directory/'results.json',result)
    print(json.dumps({'mode':name,'overall_pass':result['overall_pass'],'seconds':result['seconds']}),flush=True)
    return 0 if result['overall_pass'] else 1


if __name__=='__main__':raise SystemExit(main())
