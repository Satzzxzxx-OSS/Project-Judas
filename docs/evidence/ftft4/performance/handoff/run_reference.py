#!/usr/bin/env python3
"""Compile/run the isolated component references; NOT a Judas engine runner."""
import hashlib,json,os,pathlib,platform,subprocess,sys,time
ROOT=pathlib.Path(__file__).resolve().parent
BUILD=ROOT/'.build-reference';OUT=ROOT/'verification-run';BUILD.mkdir(exist_ok=True);OUT.mkdir(exist_ok=True)
manifest=json.loads((ROOT/'input/MANIFEST.json').read_text());mismatches=[]
for name,v in manifest['files'].items():
 p=ROOT/'input'/name
 if not p.exists() or len(p.read_bytes())!=v['bytes'] or hashlib.sha256(p.read_bytes()).hexdigest()!=v['sha256']:mismatches.append(name)
if mismatches:raise SystemExit('Input export mismatch: '+repr(mismatches))
subprocess.run([sys.executable,str(ROOT/'work/code/make_variants.py')],check=True,timeout=30)
cc=os.environ.get('CXX','g++')
meta={'scope':'scalar-template, algebraic SAT, prepared-bound tests; no GLM or engine build', 'export_members_verified':len(manifest['files']),'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'platform':platform.platform(),'commands':[]}
tasks=[('arithmetic_review',['../input/docs/evidence/ftft4/geometry-final/input_fixtures.txt']),('sat_review',[]),('factored_fixture_check',['../input/docs/evidence/ftft4/geometry-final/input_fixtures.txt']),('bounds_cache_review',[])]
for name,args in tasks:
 compile=[cc,'-O3','-std=c++17','-ffp-contract=off','-Wall','-Wextra',str(ROOT/'work/code'/f'{name}.cpp'),'-o',str(BUILD/name)]
 start=time.monotonic()
 print('Compiling',name,flush=True)
 with (OUT/f'{name}.build.log').open('w') as f: p=subprocess.run(compile,stdout=f,stderr=subprocess.STDOUT,timeout=120)
 meta['commands'].append({'argv':compile,'exit':p.returncode,'seconds':time.monotonic()-start})
 if p.returncode:raise SystemExit(f'compile failed: {name}')
 run=[str(BUILD/name),*args];start=time.monotonic()
 with (OUT/f'{name}.jsonl').open('w') as out,(OUT/f'{name}.stderr').open('w') as err:p=subprocess.run(run,cwd=ROOT/'work',stdout=out,stderr=err,timeout=120)
 meta['commands'].append({'argv':run,'cwd':'work','exit':p.returncode,'seconds':time.monotonic()-start})
 print(name, 'PASS' if p.returncode==0 else 'FAIL',flush=True)
 if p.returncode:raise SystemExit(f'run failed: {name}')
meta['source_sha256']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((ROOT/'work/code').glob('*'))if p.is_file()}
(OUT/'RUN_METADATA.json').write_text(json.dumps(meta,indent=2)+'\n')
print('All component runners completed. Full engine/performance acceptance NOT run.')
