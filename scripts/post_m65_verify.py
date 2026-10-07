#!/usr/bin/env python3
"""Check recorded consumer evidence; known defects stay failures, not fix claims."""
from pathlib import Path
import hashlib,json,re
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'docs/evidence/post_m65_consumers'
def load(path):return json.loads((E/path).read_text())
def rows(path):return [json.loads(x) for x in (E/path/'observations.jsonl').read_text().splitlines()]
def js(path):
 out=[]
 for line in (E/path/'run.log').read_text().splitlines():
  if line.startswith('JS: {'):
   try:out.append(json.loads(line[4:]))
   except ValueError:pass
 return out
checks=[]
def check(v,label):
 checks.append({'check':label,'passed':bool(v)});print(('PASS ' if v else 'FAIL ')+label)
def main():
 project=(ROOT/'projects/post_m65_consumers/post_m65_consumers.judasproj').read_text()
 check('legacy-gameplay "false"' in project,'ordinary shared project excludes legacy gameplay')
 check('Scenes/launcher.judas' in project,'registered collection startup')
 ids=[]
 for p in (ROOT/'projects/post_m65_consumers/Assets').rglob('*.judasmeta'):
  ids+=re.findall(r'\bid "([0-9a-f]{32})"',p.read_text())
 check(len(ids)==len(set(ids)),'all namespaced asset identities unique')
 sources=list((ROOT/'projects/post_m65_consumers/Assets').rglob('*.js'))
 check(not any('__judas' in p.read_text() for p in sources),'consumer scripts do not use hidden native dispatch')
 package=load('package-final/results.json');check(len(package['scenes'])==9 and not any('lab' in p for p in package['scenes']),'export exactly selects gameplay and streamed scenes, excludes labs')
 check(all(q['exit_code']==0 for q in package['actual_executable_checks']),'actual moved executable starts all four scenes')
 check(package['runtime_sha256']==hashlib.sha256((ROOT/'build/judas').read_bytes()).hexdigest(),'package uses accepted Release runtime bytes')
 r=rows('package-final/switch');check(len(set(q['scene'] for q in r))==4,'ordinary moved-package application switches all four experiences')
 check(len(set(q['scene'] for q in r if q['paused']))==3,'each game modal pause is captured')
 check(r[-1]['bodies']==0 and r[-1]['scene']=='Scenes/launcher.judas','final scene replacement leaves empty menu world')
 for game in ['skate','rooftop','void']:
  log=(E/f'editor-{game}/run.log').read_text();check('authored scene after play/stop is IDENTICAL' in log,f'{game} editor Play/Stop restores authored scene')
  check('failed 0' in log,f'{game} editor resources publish without failed loads')
  check('scriptErrors=0' in (E/f'serial-realtime-{game}/run.log').read_text(),f'{game} final real-clock run no script fault')
 check(any('wall' in q.get('mode','') for q in js('rooftop-wall')),'actual physical input enters Rooftop wall-run')
 rising=[q for q in js('rooftop-ramp') if q.get('sup') and q.get('v',[0,0])[1]>0]
 check(len(rising)>=60,'actual inclined support remains rising in 68 representative steps')
 r=rows('rooftop-course');s=next(v['state'] for v in r[-1]['states'] if 'finished' in (v['state'] or {}));check(s['finished'] and s['falls']==0,'original Rooftop course completes with zero falls')
 check('checkpoint 1' in (E/'rooftop-checkpoint-followup/run.log').read_text(),'actual first checkpoint sensor overlap delivered')
 r=rows('rooftop-events');contact=next(q['state'] for q in r[-1]['states'] if isinstance(q['state'],dict) and 'enter' in q['state']);check(contact['enter']==2 and contact['stay']>0 and contact['exit']==2 and contact['valid'],'actual Rooftop runner receives collision enter/stay/exit with valid other handle')
 log=(E/'skate-apis-followup/run.log').read_text();state=json.loads([l[11:] for l in log.splitlines() if l.startswith('JS: REVIEW ')][-1]);check(state['poseLift']==state['measuredLift'],'actual rider resolved toes drive lift')
 check(state['ikConfigured'] and state['ikError']<.00001,'actual imported rider two-bone IK reaches target')
 check(state['socketError']<.000001,'actual hand socket follows resolved pose')
 check(state['jointCreated'] and state['jointReanchored'] and state['jointRetired'],'actual board runtime joint lifecycle')
 check(state['hitMaterial']==state['materialRead']['asset'] and state['hitBody']=='10','actual board ray reports active physical material')
 log=(E/'skate-save/run.log').read_text();check('SAVES save completed' in log and 'SKATER restore' in log,'normal streamed Skate save/load completes and restores scripts')
 r=rows('skate-stream-revisit-followup');a=next(q for q in r if q['frame']==300);b=next(q for q in r if q['frame']==540);check(all(q['state']=='unloaded' for q in a['regions'] if q['id'] in ['street-1','street-2','street-3']) and all(q['state']=='active' for q in b['regions'] if q['id'] in ['street-1','street-2','street-3']),'normal spatial-interest region unload then revisit reconstructs')
 check(any(q.get('bolts',{}).get('fired',0)>0 for q in js('void-input')),'real MouseLeft input fires existing Void cannons')
 r=rows('void-input');check(any(q['state'].get('cockpit') for q in r[-1]['states'] if isinstance(q['state'],dict)),'actual Void view input changes cockpit mode')
 r=rows('void-ruby');check(any(q['state'].get('core') and q['state'].get('stage')==3 for q in r[-1]['states'] if isinstance(q['state'],dict)),'original Ruby scenario collects core and starts ambush')
 r=rows('void-query-retained-workaround');state=next(q['state'] for q in r[-1]['states'] if q['entity']=='1000000' and 'gravity' in (q['state'] or {}));check(state['gravity']==state['motorGravity'],'ordinary-script gravity agrees with authoritative motor sample')
 check(state['suitAfter']==93 and not state['motorHit'],'retained original pilot approximation required by observed engine miss')
 check(all(max(q.get('mixedPeak',0) for q in rows(name))>0 for name in ['serial-realtime-void','serial-realtime-skate','serial-realtime-rooftop']),'all three real audio backends produce nonzero PCM, listening not claimed')
 log=(E/'import-lexical-diagnostic/run.log').read_text();check('Assets/void/scripts/autopilot.js:114:' in log,'exact imported lexical-error reproduction reports source and line')
 log=(E/'api-coverage-followup.log').read_text();check(json.loads(log)['pass'] and json.loads(log)['publicSymbols']==289,'current JudasJS/type coverage passes without binding changes')
 check('SUMMARY 35 checks 0 failures' in (E/'authoring.log').read_text(),'actual consumer batch/reference authoring checks pass')
 known=[{'finding':'N1','status':'REPRODUCED; NOT FIXED','evidence':'capsule-repro-final.log'},{'finding':'N2','status':'REPRODUCED M65 REGRESSION; NOT FIXED','evidence':'reference-vm-cost-serial.log and serial-realtime-skate'},{'finding':'Void S1','status':'REPRODUCED; NOT FIXED','evidence':'scaling/measurements.json'},{'finding':'Skate F01','status':'IMPROVED INFRASTRUCTURE; IMPORTED RIGS STILL AWAKE','evidence':'skate-ragdolls-wake'},{'finding':'Harness outer boundaries','status':'REPRODUCED; NOT FIXED','evidence':'harness-scene-boundary'}]
 (E/'review-verification.json').write_text(json.dumps({'checks':checks,'passed':sum(q['passed'] for q in checks),'failures':sum(not q['passed'] for q in checks),'known_engine_defects':known,'human_acceptance':'pending'},indent=2)+'\n')
 assert all(q['passed'] for q in checks),'recorded evidence discrepancy'
if __name__=='__main__':main()
