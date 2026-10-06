#!/usr/bin/env python3
"""Focused M65 input/presentation and useful-invalid-command proof."""
import csv,hashlib,io,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m65/harness-followup-verified';PROJECT=ROOT/'projects/m65_integration/m65_integration.judasproj'
def run(name,text,expected=0):
 script=OUT/(name+'.txt');script.write_text(text)
 env=dict(os.environ,SDL_VIDEODRIVER='offscreen',SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1',XDG_DATA_HOME=str(ROOT/'.cache/m65-harness-followup'),JUDAS_TEST_SCRIPT=str(script))
 r=subprocess.run([str(ROOT/'build/judas'),str(PROJECT)],cwd=ROOT,env=env,capture_output=True,text=True,timeout=90)
 (OUT/(name+'.log')).write_text(r.stdout+r.stderr)
 return r,r.returncode==expected if expected==0 else r.returncode!=0 and '[TestHarness]' in r.stderr

def main():
 assert not OUT.exists(),'Preserve existing evidence; choose a fresh path.'
 OUT.mkdir(parents=True)
 r,okay=run('modern',(ROOT/'docs/evidence/m65/harness-final/modern.txt').read_text().replace('harness-final','harness-followup-verified'))
 lines=r.stdout.splitlines();header=next((s for s in lines if s.startswith('step,time,')),'')
 # Runtime prefab/ragdoll births append objN values after the stable 19 fields.
 data=[','.join(s.split(',')[:19]) for s in lines if s and s[0].isdigit() and s.count(',')>=18]
 rows=list(csv.DictReader(io.StringIO(','.join(header.split(',')[:19])+'\n'+'\n'.join(data))))
 def sample(n):return next(x for x in rows if int(x['frame'])==n)
 checks={'modern':okay,'two_steps':len(rows)==150 and int(sample(15)['steps'])==2,'moves':len(rows)==150 and abs(float(sample(30)['posZ'])-float(sample(5)['posZ']))>1}
 paused=[x for x in rows if 85<=int(x['frame'])<=110]
 checks['paused_steps_zero']=bool(paused) and all(int(x['steps'])==0 for x in paused)
 checks['paused_no_motion']=bool(paused) and len({(x['posX'],x['posY'],x['posZ']) for x in paused})==1
 checks['camera_changes']=all((OUT/n).exists() for n in ['first.png','turned.png','paused.png']) and (OUT/'first.png').read_bytes()!=(OUT/'turned.png').read_bytes()
 _,checks['transitions']=run('transitions',(ROOT/'docs/evidence/m65/harness-final/transitions.txt').read_text().replace('harness-final','harness-followup-verified'))
 cases={'unknown-directive':'BOGUS 1','unknown-axis':'EXPECT_AXIS nonexistent_axis 0 1','unknown-held':'EXPECT_HELD nonexistent_action 0 1','axis-kind':'EXPECT_AXIS pause 0 1','held-kind':'EXPECT_HELD move_y 0 1','held-boolean':'EXPECT_HELD pause 2 1','paused-boolean':'EXPECT_PAUSED 2 1','unknown-action':'ACTION nonexistent_action 1 0 1','missing-argument':'LOOK 1','extra-argument':'EXPECT_PAUSED 0 1 extra','wrong-clock':'FRAME_SECONDS -1','invalid-frame':'EXPECT_PAUSED 0 4'}
 for name,text in cases.items():_,checks[name]=run(name,'FRAMES 3\n'+text+'\n',1)
 result={'checks':checks,'all_pass':all(checks.values()),'runtime_sha256':hashlib.sha256((ROOT/'build/judas').read_bytes()).hexdigest(),'captures':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in OUT.glob('*.png')}}
 (OUT/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));assert result['all_pass']
if __name__=='__main__':main()
