#!/usr/bin/env python3
"""One clean Release / production / async gate for M63 structural fracture integration.
Historical runners are reused with new output paths. No historical assertions
or evidence are rewritten. Run only after the focused candidate is complete.
"""
import importlib.util,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m63/final'
def main():
 assert not OUT.exists(),'Preserve gate evidence; do not repeat the broad run.'
 backup=ROOT/'.cache/m63-before-clean-release'
 if '--resume-preflight' in sys.argv:assert backup.exists() and not (ROOT/'build').exists()
 else:
  assert not backup.exists();backup.parent.mkdir(exist_ok=True)
  if (ROOT/'build').exists():(ROOT/'build').rename(backup)
 OUT.mkdir(parents=True)
 subprocess.run([sys.executable,str(ROOT/"tools/CreateAudioFixtures.py"),str(ROOT/".cache/m63-audio")],cwd=ROOT,check=True)
 spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
 gate.REVIEWED_SHARED.add("src/WorldState.cpp") # M59 explicitly rejects composed disk saves; historical evidence stays protected.
 source=(ROOT/'scripts/m56_validation.py').read_text()
 gate.OUTPUT_TESTS.update(re.findall(r"'(judas_\w+_tests)'",source.split('original=subprocess.run')[0]))
 original=subprocess.run
 def run(command,*args,**kwargs):
  if command==['cmake','-S','.','-B','build']:command=command+['-DCMAKE_BUILD_TYPE=Release']
  if isinstance(command,list) and command:
   name=Path(command[0]).name
   if name=='judas_fracture_application_tests' and len(command)==1:command=command+[str(ROOT/'projects/fracture_lab/fracture_lab.judasproj'),'probe',str(ROOT/'.cache/m63-production-fracture')]
   if name=='judas_deformable_application_tests' and len(command)==1:command=command+[str(ROOT/'projects/deformable_lab/deformable_lab.judasproj'),'probe',str(ROOT/'.cache/m63-production-probe')]
   if name=='judas_shooter_application_tests' and len(command)==1:command=command+[str(ROOT/'projects/shooter_game/shooter_game.judasproj'),str(OUT/'shooter-application')]
   if name=='judas_material_application_tests' and len(command)==1:command=command+[str(OUT/'material-application'),'quick']
   if name=='judas_material_tests' and len(command)==1:command=command+[str(OUT/'material-gl'),str(ROOT/'projects/material_lab/Assets/environment/studio.judasenv')]
   if name in ('judas_text_localization_tests','judas_text_render_tests','judas_localization_application_tests') and len(command)==1:command=command+[str(OUT/name)]
   if name=='judas_streaming_application_tests' and len(command)==1:command=command+[str(ROOT/'projects/streamed_range/streamed_range.judasproj'),str(OUT/'streaming-application')]
   if name=='judas_streaming_performance_tests' and len(command)==1:command=command+[str(OUT/'streaming-performance')]
   if name=='judas_audio_acoustics_tests' and len(command)==1:command=command+[str(ROOT/'.cache/m63-audio')]
   if name=='judas_audio_application_tests' and len(command)==1:command=command+[str(ROOT/'projects/audio_lab/audio_lab.judasproj'),str(OUT/'audio-application')]
   if name=='judas_save_storage_tests' and len(command)==1:command=command+[str(ROOT/'.cache/m61-storage-m63')]
   if name=='judas_save_application_tests' and len(command)==1:command=command+[str(ROOT/'projects/save_lab/save_lab.judasproj'),'write',str(OUT/'save-application')]
   if name=='judas_save_streaming_tests' and len(command)==1:command=command+[str(ROOT/'projects/streamed_range/streamed_range.judasproj'),'write',str(OUT/'save-streaming')]
   if name=='judas_save_performance_tests' and len(command)==1:command=command+[str(ROOT/'projects/save_lab/save_lab.judasproj'),str(OUT/'save-performance')]
   if name.startswith('judas_save_'):
    kwargs['env']=dict(kwargs.get('env') or {},XDG_DATA_HOME=str(ROOT/'.cache/m63-gate-data'),SDL_AUDIODRIVER='dummy')
  return original(command,*args,**kwargs)
 subprocess.run=run
 try:result=gate.production(OUT,2)
 finally:subprocess.run=original
 (OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));assert result['overall_pass']
if __name__=='__main__':main()
