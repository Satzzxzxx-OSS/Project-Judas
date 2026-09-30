from pathlib import Path
import subprocess,hashlib,json,time,os
root=Path(__file__).resolve().parents[5];os.chdir(root);out=Path(__file__).resolve().parent
sources=[Path("src/FluidWorld.cpp"),Path("src/FluidWorld.h"),Path("src/ProductionFluidCoupling.cpp"),Path("src/FluidHydrostatics.cpp"),Path("tests/ProductionFluidTests.cpp"),out/"coarse_gap_witness.cpp"]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
result={"head":subprocess.check_output(["git","rev-parse","HEAD"],text=True).strip(),"scope":"Current restored-code defect verification only, not acceptance.","source_sha256_before":{str(p):sha(p) for p in sources},"runs":[]}
def save():(out/"results.json").write_text(json.dumps(result,indent=2)+"\n")
def run(name,cmd):
 t=time.monotonic()
 with (out/(name+".log")).open("w") as f:r=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT)
 entry={"name":name,"command":cmd,"exit_code":r.returncode,"seconds":time.monotonic()-t};result["runs"].append(entry);save();print(json.dumps(entry),flush=True);return r.returncode
save()
if run("build",["cmake","--build","build","--target","judas_production_fluid_tests","-j4"])==0:
 run("half_025",["./build/judas_production_fluid_tests","--case","half_025","--output",str(out/"half_025")])
 if run("compile-probe",["/usr/bin/c++","-std=gnu++17","-ffp-contract=off","-O3","-DNDEBUG","-Wall","-Wextra","-Isrc",str(out/"coarse_gap_witness.cpp"),"build/libjudas_engine.a","/usr/lib/x86_64-linux-gnu/libSDL2.so","build/libjudas_glad.a","-o","build/ftft9_coarse_gap_witness"])==0:
  result["probe_binary_sha256"]=sha(Path("build/ftft9_coarse_gap_witness"))
  run("coarse-gap-witness",["./build/ftft9_coarse_gap_witness"])
result["source_sha256_after"]={str(p):sha(p) for p in sources};result["source_unchanged"]=result["source_sha256_before"]==result["source_sha256_after"];save()
