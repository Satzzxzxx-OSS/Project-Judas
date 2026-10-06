#!/usr/bin/env python3
"""Current editor Play/Stop, normal export and moved standalone smoke."""
import hashlib,json,os,shutil,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m65/handoff-smoke';PROJECT=ROOT/'projects/m65_integration/m65_integration.judasproj'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 assert not OUT.exists(),'Preserve evidence; select fresh paths before rerunning.'
 OUT.mkdir(parents=True);scratch=Path('/tmp/judas-m65-handoff-smoke');assert not scratch.exists();scratch.mkdir()
 env=dict(os.environ,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_DATA_HOME=str(scratch/'data'));env.pop('LIBGL_ALWAYS_SOFTWARE',None)
 records=[]
 def run(name,command,cwd=ROOT,environment=env):
  start=time.perf_counter()
  with (OUT/(name+'.log')).open('w') as f:r=subprocess.run(command,cwd=cwd,env=environment,stdout=f,stderr=subprocess.STDOUT,timeout=120)
  records.append({'name':name,'command':list(map(str,command)),'cwd':str(cwd),'exit_code':r.returncode,'seconds':time.perf_counter()-start});assert r.returncode==0
 editorProject=scratch/'editor-project';shutil.copytree(PROJECT.parent,editorProject)
 before=digest(editorProject/'Scenes/integration.judas')
 run('editor',[str(ROOT/'build/judas_editor'),str(editorProject/PROJECT.name)],environment=dict(env,JUDAS_EDITOR_AUTOTEST=str(OUT/'editor')))
 text=(OUT/'editor.log').read_text();assert 'undo restores: IDENTICAL' in text and 'authored scene after play/stop is IDENTICAL' in text and before==digest(editorProject/'Scenes/integration.judas')
 run('export',[str(ROOT/'build/judas_export'),str(PROJECT),str(scratch/'export'),str(ROOT/'build/judas')])
 packages=[p.parent for p in (scratch/'export').rglob('judas-package.txt')];assert len(packages)==1
 moved=scratch/'Moved Game';shutil.move(str(packages[0]),str(moved));unrelated=scratch/'unrelated-cwd';unrelated.mkdir()
 harness=OUT/'moved.txt';harness.write_text((ROOT/'docs/evidence/m65/harness-final/transitions.txt').read_text().replace('harness-final','handoff-smoke'))
 with harness.open('a') as f:f.write(f'ACTION pause 1 165 166\nEXPECT_PAUSED 1 170\nSCREENSHOT 175 {OUT}/moved-paused.png\n')
 run('moved',[str(moved/'judas')],cwd=unrelated,environment=dict(env,JUDAS_TEST_SCRIPT=str(harness)))
 assert not any(s in (OUT/'moved.log').read_text() for s in ['[Script]','script callback failed','[TestHarness] pause expectation failed'])
 files={str(p.relative_to(moved)):digest(p) for p in moved.rglob('*') if p.is_file()}
 result={'runs':records,'all_pass':True,'editor_authored_unchanged':True,'source_runtime_sha256':digest(ROOT/'build/judas'),'moved_package':str(moved),'package_bytes':sum(p.stat().st_size for p in moved.rglob('*') if p.is_file()),'package_files':files,'license_materials_present':all((moved/p).exists() for p in ['Assets/models/LICENSE.txt','Assets/fonts/LICENSE.txt']),'human_visual_acceptance':'pending operator'}
 (OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print('Editor, normal export, moved unrelated-cwd launch: PASS')
if __name__=='__main__':main()
