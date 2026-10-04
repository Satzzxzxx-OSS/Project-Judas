#!/usr/bin/env python3
"""M57 single clean Release/production/async gate. Historical gate files stay unchanged."""
import importlib.util,json,os,shutil,subprocess,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m57/final'
def main():
 resume='--resume-build' in sys.argv
 if not resume:
  assert not OUT.exists(),'Preserve previous gate evidence.'
  backup=ROOT/'.cache/m57-before-clean-release';assert not backup.exists();backup.parent.mkdir(exist_ok=True)
  if (ROOT/'build').exists():(ROOT/'build').rename(backup)
 OUT.mkdir(parents=True,exist_ok=resume)
 spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
 spec56=importlib.util.spec_from_file_location('m56',ROOT/'scripts/m56_validation.py');m56=importlib.util.module_from_spec(spec56);spec56.loader.exec_module(m56)
 # Same argument relocation as M56, plus bounded M57 fixtures (profiles run separately).
 source=(ROOT/'scripts/m56_validation.py').read_text();import re
 gate.OUTPUT_TESTS.update(re.findall(r"'(judas_\w+_tests)'",source.split('original=subprocess.run')[0]))
 original=subprocess.run
 def run(command,*a,**kw):
  if command==['cmake','-S','.','-B','build']:command=command+['-DCMAKE_BUILD_TYPE=Release']
  if isinstance(command,list) and command:
   name=Path(command[0]).name
   if name=='judas_shooter_application_tests' and len(command)==1:command=command+[str(ROOT/'projects/shooter_game/shooter_game.judasproj'),str(OUT/'shooter-application')]
   if name=='judas_material_application_tests' and len(command)==1:command=command+[str(OUT/'material-application'),'quick']
   if name=='judas_material_tests' and len(command)==1:command=command+[str(OUT/'material-gl'),str(ROOT/'projects/material_lab/Assets/environment/studio.judasenv')]
  return original(command,*a,**kw)
 subprocess.run=run
 try:result=gate.production(OUT,6)
 finally:subprocess.run=original
 (OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));assert result['overall_pass']
if __name__=='__main__':main()
