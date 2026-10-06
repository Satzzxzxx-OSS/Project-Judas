#!/usr/bin/env python3
"""One frozen M64 clean Release / production / async gate. Historical evidence is untouched."""
import importlib.util,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m64/final'
def main():
 global OUT
 resume=len(sys.argv)>1 and sys.argv[1]=='--resume-build'
 if resume:
  previous=json.loads((OUT/'production/results.json').read_text())
  assert previous['runs'][-1]['name']=='build' and previous['runs'][-1]['exit_code']=='TIMEOUT'
  assert not previous['checks'],'Only a timed-out build, before tests, may continue.'
  OUT=ROOT/'docs/evidence/m64/final-resumed'
 assert not OUT.exists(),'Preserve broad evidence; do not repeat the gate.'
 if not resume:
  backup=ROOT/'.cache/m64-before-clean-release';assert not backup.exists();backup.parent.mkdir(exist_ok=True)
  if (ROOT/'build').exists():(ROOT/'build').rename(backup)
 OUT.mkdir(parents=True)
 subprocess.run([sys.executable,str(ROOT/'tools/CreateAudioFixtures.py'),str(ROOT/'.cache/m64-audio')],cwd=ROOT,check=True)
 spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
 gate.REVIEWED_SHARED.add('src/WorldState.cpp') # Existing M59 composed-save rejection; no M64 change.
 source=(ROOT/'scripts/m56_validation.py').read_text();gate.OUTPUT_TESTS.update(re.findall(r"'(judas_\w+_tests)'",source.split('original=subprocess.run')[0]))
 original=subprocess.run
 def run(command,*args,**kwargs):
  if command==['cmake','-S','.','-B','build']:command=command+['-DCMAKE_BUILD_TYPE=Release']
  if isinstance(command,list) and command[:3]==['cmake','--build','build']:kwargs['timeout']=3600
  if isinstance(command,list) and command:
   name=Path(command[0]).name
   application={'judas_collision_application_tests':['projects/collision_lab/collision_lab.judasproj','probe','collision'],'judas_fracture_application_tests':['projects/fracture_lab/fracture_lab.judasproj','probe','fracture'],'judas_deformable_application_tests':['projects/deformable_lab/deformable_lab.judasproj','probe','deformable'],'judas_shooter_application_tests':['projects/shooter_game/shooter_game.judasproj',None,'shooter'],'judas_streaming_application_tests':['projects/streamed_range/streamed_range.judasproj',None,'streaming'],'judas_audio_application_tests':['projects/audio_lab/audio_lab.judasproj',None,'audio'],'judas_save_application_tests':['projects/save_lab/save_lab.judasproj','write','save'],'judas_save_streaming_tests':['projects/streamed_range/streamed_range.judasproj','write','save-streaming'],'judas_save_performance_tests':['projects/save_lab/save_lab.judasproj',None,'save-performance']}
   if name in application and len(command)==1:
    project,mode,label=application[name];command=command+[str(ROOT/project)]+([mode] if mode else [])+[str(OUT/label)]
   if name=='judas_material_application_tests' and len(command)==1:command=command+[str(OUT/'material-application'),'quick']
   if name=='judas_material_tests' and len(command)==1:command=command+[str(OUT/'material-gl'),str(ROOT/'projects/material_lab/Assets/environment/studio.judasenv')]
   if name in ('judas_text_localization_tests','judas_text_render_tests','judas_localization_application_tests') and len(command)==1:command=command+[str(OUT/name)]
   if name=='judas_streaming_performance_tests' and len(command)==1:command=command+[str(OUT/'streaming-performance')]
   if name=='judas_audio_acoustics_tests' and len(command)==1:command=command+[str(ROOT/'.cache/m64-audio')]
   if name=='judas_save_storage_tests' and len(command)==1:command=command+[str(ROOT/'.cache/m61-storage-m64')]
   if name.startswith('judas_save_'):kwargs['env']=dict(kwargs.get('env') or {},XDG_DATA_HOME=str(ROOT/'.cache/m64-gate-data'),SDL_AUDIODRIVER='dummy')
  return original(command,*args,**kwargs)
 subprocess.run=run
 try:result=gate.production(OUT,2)
 finally:subprocess.run=original
 (OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));assert result['overall_pass']
if __name__=='__main__':main()
