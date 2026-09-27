#!/usr/bin/env python3
"""Compile FTFT1's pre-fix witness from the exact committed baseline.

Verification only: git archive reads the commit into an isolated ignored build
source tree; it does not reset/stash/checkout the user's current working tree.
Usage: python3 docs/evidence/ftft1/reproduce_baseline.py [--jobs 4]
Dependencies: Python 3, git, CMake, C++17 compiler, pkg-config, SDL2 and GLM dev packages.
The witness is intentionally expected to exit 1 with the two reproduced defects.
"""
import argparse
import hashlib
import io
import json
import pathlib
import shlex
import subprocess
import tarfile
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--jobs', type=int, default=4)
args = parser.parse_args()
if args.jobs < 1:
    parser.error('--jobs must be positive')
root = pathlib.Path(__file__).resolve().parents[3]
evidence = root / 'docs/evidence/ftft1'
commit = '587759e93add89f32774cbc51fe47878fab127ec'
source = root / 'build/ftft1-baseline-src'
build = root / 'build/ftft1-baseline-build'
log_path = evidence / 'pre_fix_verified.log'
metadata_path = evidence / 'pre_fix_verified_metadata.json'
archive_paths = ['CMakeLists.txt', 'src', 'tests', 'third_party', 'tools']

def output(argv):
    return subprocess.check_output(argv, cwd=root, text=True).strip()

def digest(data):
    return hashlib.sha256(data).hexdigest()

started = time.monotonic()
archive = subprocess.check_output(['git', 'archive', '--format=tar', commit, *archive_paths], cwd=root)
source.mkdir(parents=True, exist_ok=True)
# Repeated runs reuse only files proven identical to this exact git archive.
# Never overwrite an existing source file if its bytes differ.
source_hashes = {}
with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
    for member in tar.getmembers():
        target = source / member.name
        if member.isdir():
            target.mkdir(parents=True, exist_ok=True)
            continue
        if not member.isfile() or pathlib.PurePosixPath(member.name).is_absolute() or '..' in pathlib.PurePosixPath(member.name).parts:
            raise SystemExit('Unexpected non-regular archive member: ' + member.name)
        data = tar.extractfile(member).read()
        if target.exists() and target.read_bytes() != data:
            raise SystemExit('Refusing to overwrite changed verification source: ' + str(target))
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists():
            target.write_bytes(data)
        source_hashes[member.name] = digest(data)
actual_source_files = {str(p.relative_to(source)) for p in source.rglob('*') if p.is_file()}
if actual_source_files != set(source_hashes):
    raise SystemExit('Untracked files exist in verification source tree; refusing potentially contaminated build')

metadata = {
    'baseline_commit': commit,
    'baseline_tree': output(['git', 'rev-parse', commit + '^{tree}']),
    'working_head_at_run': output(['git', 'rev-parse', 'HEAD']),
    'archive_paths': archive_paths,
    'archive_sha256': digest(archive),
    'source_file_count': len(source_hashes),
    'source_sha256': source_hashes,
    'witness_source': 'docs/evidence/ftft1/pre_fix_reproduction.cpp',
    'witness_sha256': digest((evidence / 'pre_fix_reproduction.cpp').read_bytes()),
    'commands': [],
    'expected_witness_exit_status': 1,
    'expected_failures': 2,
    'note': 'Fresh engine compiled exclusively from archived committed sources; no current production source was used.'
}
with log_path.open('w') as log:
    log.write('FTFT1 pre-fix verification from exact committed source\n')
    log.write('commit=' + commit + '\ntree=' + metadata['baseline_tree'] + '\n')
    log.write('archive_sha256=' + metadata['archive_sha256'] + '\n')
    log.write('source_files=' + str(len(source_hashes)) + '\n')
    def run(argv):
        metadata['commands'].append(argv)
        log.write('\n$ ' + shlex.join(argv) + '\n')
        log.flush()
        completed = subprocess.run(argv, cwd=root, stdout=log, stderr=subprocess.STDOUT)
        if completed.returncode:
            metadata['failed_command'] = argv
            metadata['failed_exit_status'] = completed.returncode
            metadata_path.write_text(json.dumps(metadata, indent=2) + '\n')
            raise SystemExit('Command failed; see ' + str(log_path))
    run(['cmake', '-S', str(source), '-B', str(build), '-DCMAKE_BUILD_TYPE=Release', '-DJUDAS_GLM=glm::glm'])
    run(['cmake', '--build', str(build), '--target', 'judas_engine', '-j', str(args.jobs)])
    cflags = shlex.split(output(['pkg-config', '--cflags', 'sdl2']))
    libs = shlex.split(output(['pkg-config', '--libs', 'sdl2']))
    binary = build / 'ftft1_pre_fix_tests'
    run(['c++', '-O3', '-DNDEBUG', '-std=c++17', '-I', str(source / 'src'), '-I', str(source / 'third_party/glad/include'),
         *cflags, str(evidence / 'pre_fix_reproduction.cpp'), str(build / 'libjudas_engine.a'), str(build / 'libjudas_glad.a'),
         *libs, '-pthread', '-o', str(binary)])
    metadata['commands'].append([str(binary)])
    log.write('\n$ ' + str(binary) + '\n')
    completed = subprocess.run([str(binary)], cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    log.write(completed.stdout)
    log.write('witness_exit_status=' + str(completed.returncode) + '\n')
    required = ['OBSERVED baseline_mismatch applied=1 position_x=44',
                'OBSERVED partial_apply result=0 first_entity_lifecycle=2',
                'FTFT1 checks: 2 failures']
    passed = completed.returncode == 1 and all(line in completed.stdout for line in required)
    metadata['observed_witness_exit_status'] = completed.returncode
    metadata['both_defects_reproduced'] = passed
    metadata['elapsed_seconds'] = time.monotonic() - started
    metadata['compiled_sha256'] = {str(p.relative_to(root)): digest(p.read_bytes()) for p in [build / 'libjudas_engine.a', build / 'libjudas_glad.a', binary]}
    log.write('verification=' + ('PASS (both original defects reproduced)' if passed else 'FAIL') + '\n')
    metadata_path.write_text(json.dumps(metadata, indent=2) + '\n')
    print(completed.stdout, end='')
    print('Fresh-source pre-fix verification:', 'PASS' if passed else 'FAIL')
    print('Elapsed seconds:', metadata['elapsed_seconds'])
    if not passed:
        raise SystemExit(1)
