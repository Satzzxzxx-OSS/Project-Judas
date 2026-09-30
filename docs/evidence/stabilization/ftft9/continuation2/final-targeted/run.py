from pathlib import Path
import os, subprocess, time, json, hashlib
root=Path(__file__).resolve().parents[6]
out=Path(__file__).resolve().parent
os.chdir(root)
def hashes():
    paths=[Path("CMakeLists.txt")]+[p for d in ("src","tests","scripts") for p in Path(d).rglob("*") if p.is_file() and p.suffix in (".cpp",".h",".py",".sh")]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(paths)}
def execute(name,cmd):
    start=time.monotonic()
    with (out/(name+".log")).open("w") as log:
        run=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
    entry={"name":name,"command":cmd,"exit_code":run.returncode,"seconds":time.monotonic()-start,"log":str((out/(name+".log")).relative_to(root))}
    result["runs"].append(entry);save();print(json.dumps(entry),flush=True)
    return run.returncode==0
def save():
    (out/"results.json").write_text(json.dumps(result,indent=2)+"\n")
result={"scope":"Post-rejection precise rollback, disk-preservation oracle repair, single shipped-workload timing. Not FTFT9 acceptance.","source_sha256_before":hashes(),"runs":[]}
save()
if execute("build",["cmake","--build","build","--target","judas_project_tests","judas_fluid_boundary_contact_tests","judas_production_fluid_performance","-j4"]):
    execute("project",["./build/judas_project_tests"])
    execute("boundary",["./build/judas_fluid_boundary_contact_tests"])
    execute("performance",["./build/judas_production_fluid_performance","--output",str(out/"performance")])
result["source_sha256_after"]=hashes()
result["source_unchanged_during_run"]=result["source_sha256_before"]==result["source_sha256_after"]
result["targeted_pass"]=all(r["exit_code"]==0 for r in result["runs"]) and len(result["runs"])==4 and result["source_unchanged_during_run"]
save()
raise SystemExit(0 if result["targeted_pass"] else 1)
