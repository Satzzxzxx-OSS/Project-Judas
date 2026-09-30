#!/usr/bin/env python3
"""Time the final compiled shipped fixtures once, without concurrent tests."""
import hashlib,json,os,pathlib,subprocess,time
ROOT=pathlib.Path(__file__).resolve().parents[5];OUT=pathlib.Path(__file__).resolve().parent/'final-performance';OUT.mkdir(exist_ok=False)
def hashes():return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for d in ('src','tests') for p in sorted((ROOT/d).rglob('*')) if p.suffix in ('.cpp','.h')}
r={'source_before':hashes(),'environment':{'SDL_VIDEODRIVER':'offscreen','LIBGL_ALWAYS_SOFTWARE':'1'}};cmd=['./build/judas_production_fluid_performance','--output',str(OUT/'fixtures')];start=time.monotonic();p=subprocess.run(cmd,cwd=ROOT,env=dict(os.environ,**r['environment']),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(OUT/'run.log').write_text(p.stdout);r.update(command=cmd,exit_code=p.returncode,seconds=time.monotonic()-start,source_after=hashes());r['source_unchanged']=r['source_before']==r['source_after'];r['pass']=p.returncode==0 and r['source_unchanged'];(OUT/'results.json').write_text(json.dumps(r,indent=2)+'\n');print(p.stdout);raise SystemExit(0 if r['pass'] else 1)
