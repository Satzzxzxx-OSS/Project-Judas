#!/usr/bin/env python3
"""Final ordinary-loop, editor and moved-package checks. Original games untouched."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]; P=ROOT/'projects/post_m65_consumers'; E=ROOT/'docs/evidence/post_m65_consumers'
def launch(args,out,env):
    out.mkdir(parents=True,exist_ok=False)
    r=subprocess.run(args,cwd='/tmp',env=env,capture_output=True,text=True,timeout=180)
    (out/'run.log').write_text(r.stdout+r.stderr);(out/'exit.json').write_text(json.dumps({'exit_code':r.returncode,'command':[str(a) for a in args]},indent=2)+'\n')
    print(out.name,r.returncode,flush=True);assert r.returncode==0

def main():
    env={k:v for k,v in os.environ.items() if not k.startswith(('JUDAS_','FTFT_'))};env.update(SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy')
    with tempfile.TemporaryDirectory(prefix='judas-consumers-final-') as tmp:
        p=Path(tmp)/'project';shutil.copytree(P,p)
        for name,scene,events in [('skate','skate/park','1 key:W 1\n220 key:W 0\n'),('rooftop','rooftop/rooftops','1 key:W 1\n220 key:W 0\n'),('void','void/system','1 key:W 1\n40 key:W 0\n50 key:F 1\n51 key:F 0\n80 key:Space 1\n130 key:Space 0\n150 mouse:Left 1\n200 mouse:Left 0\n')]:
            out=E/('realtime-'+name);out.mkdir(exist_ok=False);f=out/'events.txt';f.write_text(events)
            r=subprocess.run([str(ROOT/'build/judas_consumer_review'),str(p/'Scenes'/f'{scene}.judas'),str(f),str(out),'600'],cwd='/tmp',env=dict(env,JUDAS_REVIEW_REAL_TIME='1',JUDAS_REVIEW_CAPTURE_EVERY='600'),capture_output=True,text=True,timeout=120)
            (out/'run.log').write_text(r.stdout+r.stderr);(out/'exit.json').write_text(json.dumps({'exit_code':r.returncode})+'\n');assert r.returncode==0;print(out.name,r.returncode,flush=True)
            out=E/('editor-'+name)
            launch([str(ROOT/'build/judas_editor'),str(p/'Scenes'/f'{scene}.judas')],out,dict(env,JUDAS_EDITOR_AUTOTEST=str(out/'editor')))
        out=E/'harness-scene-boundary';out.mkdir(exist_ok=False);f=out/'events.txt';f.write_text('FRAMES 180\nLOG_EVERY 0\nACTION collection_rooftop 1 5 6\nSCREENSHOT 150 '+str(out/'queued.png')+'\n')
        r=subprocess.run([str(ROOT/'build/judas'),str(p/'post_m65_consumers.judasproj')],cwd='/tmp',env=dict(env,JUDAS_TEST_SCRIPT=str(f)),capture_output=True,text=True,timeout=60);(out/'run.log').write_text(r.stdout+r.stderr);assert r.returncode==0
        # A syntax error in an imported ordinary game module, preserving the failure.
        ap=p/'Assets/void/scripts/autopilot.js';ap.write_text(ap.read_text()+'\nconst duplicate=1; const duplicate=2;\n')
        out=E/'import-diagnostic';out.mkdir(exist_ok=False);f=out/'events.txt';f.write_text('FRAMES 20\nLOG_EVERY 0\n')
        r=subprocess.run([str(ROOT/'build/judas'),str(p/'Scenes/void/system.judas')],cwd='/tmp',env=dict(env,JUDAS_TEST_SCRIPT=str(f)),capture_output=True,text=True,timeout=60);(out/'run.log').write_text(r.stdout+r.stderr);(out/'repro.js').write_text('const duplicate=1; const duplicate=2;\n');(out/'exit.json').write_text(json.dumps({'exit_code':r.returncode})+'\n')
    # Export as an ordinary Release package, then physically copy outside the repo.
    out=E/'package-final';out.mkdir(exist_ok=False);package=ROOT/'.cache/post-m65-package/JudasThreeGames'
    r=subprocess.run([str(ROOT/'build/judas_export'),str(P/'post_m65_consumers.judasproj'),str(package),str(ROOT/'build/judas')],capture_output=True,text=True);(out/'export.log').write_text(r.stdout+r.stderr);assert r.returncode==0
    moved=Path('/tmp/JudasThreeGames-post-M65');assert not moved.exists();shutil.copytree(package,moved)
    records=[]
    # Actual shipped executable, each scene and modal pause, from unrelated cwd.
    for name,scene,action in [('launcher',None,None),('skate','skate/park','skate_pause'),('rooftop','rooftop/rooftops','rooftop_pause'),('void','void/system','void_pause')]:
        f=out/(name+'-steps.txt');f.write_text('FRAMES 180\nLOG_EVERY 0\n'+(f'ACTION {action} 1 90 91\nEXPECT_PAUSED 1 100\n' if action else '')+'SCREENSHOT 150 '+str(out/(name+'.png'))+'\n')
        args=[str(moved/'judas')]+([str(moved/'Scenes'/f'{scene}.judas')] if scene else [])
        r=subprocess.run(args,cwd='/tmp',env=dict(env,JUDAS_TEST_SCRIPT=str(f)),capture_output=True,text=True,timeout=60);(out/(name+'.log')).write_text(r.stdout+r.stderr);records.append({'game':name,'exit_code':r.returncode});assert r.returncode==0,(name,r.stderr)
    # Same Application/engine observer placed beside the shipped runtime and marker
    # so EngineExecutableDir and packaged-save/resource resolution really use it.
    observer=moved/'review-observer';shutil.copyfile(ROOT/'build/judas_consumer_review',observer);observer.chmod(0o755)
    obs=out/'switch';obs.mkdir();f=obs/'events.txt';f.write_text('10 key:F1 1\n11 key:F1 0\n260 key:Escape 1\n261 key:Escape 0\n360 key:F2 1\n361 key:F2 0\n540 key:Escape 1\n541 key:Escape 0\n620 key:F3 1\n621 key:F3 0\n780 key:Escape 1\n781 key:Escape 0\n900 key:F4 1\n901 key:F4 0\n1020 key:F1 1\n1021 key:F1 0\n1180 key:R 1\n1181 key:R 0\n1350 key:F4 1\n1351 key:F4 0\n')
    r=subprocess.run([str(observer),str(moved/'game.judasproj'),str(f),str(obs),'1440'],cwd='/tmp',env=dict(env,JUDAS_REVIEW_CAPTURE_EVERY='240'),capture_output=True,text=True,timeout=180);(obs/'run.log').write_text(r.stdout+r.stderr);assert r.returncode==0;observer.unlink()
    manifest={'moved_package':str(moved),'actual_executable_checks':records,'ordinary_boundary_switch_exit':r.returncode,'package_bytes':sum(x.stat().st_size for x in package.rglob('*') if x.is_file()),'assets':len(list((package/'Assets').rglob('*.judasmeta'))),'scenes':sorted(str(x.relative_to(package)) for x in (package/'Scenes').rglob('*.judas')),'runtime_sha256':hashlib.sha256((package/'judas').read_bytes()).hexdigest()}
    (out/'results.json').write_text(json.dumps(manifest,indent=2)+'\n');print('package-final passed',flush=True)
if __name__=='__main__':main()
