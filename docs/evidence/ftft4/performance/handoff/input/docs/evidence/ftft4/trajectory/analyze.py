#!/usr/bin/env python3
"""Read committed-scene/application traces and isolated actual-engine diagnostics.
No production changes, expected-value substitution, or acceptance rebaselining.
"""
import csv
import hashlib
import json
import math
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[4]
OUT=Path(__file__).resolve().parent

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def readjsonl(p): return [json.loads(line) for line in p.read_text().splitlines() if line.startswith('{')]
def norm(a,b): return math.sqrt(sum((float(x)-float(y))**2 for x,y in zip(a,b)))
result={'scope':'diagnosis of observed trajectory changes; not acceptance rebaselining',
        'csv_precision':'positions/velocities rounded to 4 decimals; every 10th post-step state only',
        'mapping_basis':'TestHarness iterates RuntimeWorld::DynamicBodies in Scene object order; RuntimeWorld::Build appends each dynamic body in that order',
        'scenes':{},'pair_diagnostics':{},'source_sha256':{}}
for scene in ['classic','terrain']:
    scene_path=ROOT/'assets/scenes'/f'{scene}.judas'
    blocks=re.findall(r'^object (\d+) "([^"]+)"\n(.*?)^end$',scene_path.read_text(),re.M|re.S)
    objects=[{'scene_id':int(i),'name':n,'body':re.search(r'^  body dynamic (\w+)',b,re.M)[1]}
             for i,n,b in blocks if re.search(r'^  body dynamic ',b,re.M)]
    paths=[ROOT/'docs/evidence/ftft4'/phase/'harness'/f'{scene}_near.csv' for phase in ['baseline','final']]
    a,b=[list(csv.DictReader(p.open())) for p in paths]
    data={'scene_sha256':sha(scene_path),'baseline_trace_sha256':sha(paths[0]),'final_trace_sha256':sha(paths[1]),'objects':[]}
    for index in [-1]+list(range(len(objects))):
        prefix='pos' if index<0 else f'obj{index}Pos'
        vp='vel' if index<0 else f'obj{index}Vel'
        columns=[prefix+d for d in 'XYZ']+[vp+d for d in 'XYZ']
        differences=[(x,y) for x,y in zip(a,b) if any(float(x[k])!=float(y[k]) for k in columns)]
        r={'kind':'player' if index<0 else 'dynamic_body','csv_index':index,
           'identity':None if index<0 else objects[index],
           'first_recorded_difference':None,'changed_sample_rows':len(differences),
           'max_position_vector_difference_m':max(norm([x[prefix+d] for d in 'XYZ'],[y[prefix+d] for d in 'XYZ']) for x,y in zip(a,b)),
           'max_velocity_vector_difference_mps':max(norm([x[vp+d] for d in 'XYZ'],[y[vp+d] for d in 'XYZ']) for x,y in zip(a,b))}
        if differences:
            x,y=differences[0]
            r['first_recorded_difference']={'step':int(x['step']),'baseline':{k:x[k] for k in columns},'final':{k:y[k] for k in columns}}
        data['objects'].append(r)
    result['scenes'][scene]=data

for case,index in [('planet_cube',0),('plank_cube',2)]:
    rows={phase:[r for r in readjsonl(OUT/f'{phase}.jsonl') if r['case']==case] for phase in ['baseline','final']}
    first=next(i for i,(a,b) in enumerate(zip(rows['baseline'],rows['final'])) if a['after']!=b['after'] or a['velocity']!=b['velocity'])
    diag={'first_difference_step':first,'baseline':rows['baseline'][first],'final':rows['final'][first],
          'pre_contact_pose_identical':all(rows['baseline'][first][k]==rows['final'][first][k] for k in ['before','q']),
          'first_actual_contact_step':{p:next(r['step'] for r in rr if r['solver_gaps']) for p,rr in rows.items()},
          'application_matches':{}}
    for phase in ['baseline','final']:
        app=list(csv.DictReader((ROOT/'docs/evidence/ftft4'/phase/'harness/classic_near.csv').open()))
        lookup={r['step']:r for r in rows[phase]};count=0;errors=[]
        for r in app:
            step=int(r['step'])
            if step not in lookup:continue
            for field,key in [('after','Pos'),('velocity','Vel')]:
                for axis,x in zip('XYZ',lookup[step][field]):
                    count+=1
                    if float(f'{x:.4f}')!=float(r[f'obj{index}{key}{axis}']):errors.append([step,field,axis])
        diag['application_matches'][phase]={'scalar_comparisons':count,'mismatches':errors}
    result['pair_diagnostics'][case]=diag

terrain=readjsonl(OUT/'terrain-runtime-run.log')
app={int(r['step']):r for r in csv.DictReader((ROOT/'docs/evidence/ftft4/final/harness/terrain_near.csv').open())}
count=0;errors=[]
for r in terrain:
    if r['step'] not in app:continue
    for axis,x in zip('XYZ',r['player']):
        count+=1
        if float(f'{x:.4f}')!=float(app[r['step']]['pos'+axis]):errors.append([r['step'],axis])
result['terrain_runtime_diagnostic']={'steps':len(terrain),'application_player_scalar_comparisons':count,'mismatches':errors,
    'first_observed_post_step_forward_ship_hit':next(r['step'] for r in terrain if r['post_step_forward_hit'] and r['post_step_forward_scene_id']==7),
    'support_ids_steps_40_80':sorted(set(r['support_scene_id'] for r in terrain if 40<=r['step']<=80)),
    'limitation':'Forward sweep is an additional post-step observation, not a captured internal move-and-slide query. No before/after controlled ablation isolates all terrain-path changes.'}
result['old_plank_axis_preference']={'best_face':-0.039325714111328125,'best_edge':-0.039325714111328125,
    'old_edge_selection_rhs':.95*(-.039325714111328125)-.001,
    'old_prefers_duplicate_edge_axis':-.039325714111328125 < .95*(-.039325714111328125)-.001,
    'mechanism':'The former preference compares negative overlaps; duplicate edge axis wins above this gap, selecting the one-point edge manifold despite aligned face geometry.'}
result['limits']=[
    'Pair diagnostic legacy raw contacts arrays can contain hit=false placeholder entries; actual generated physics contact is measured by solver_gaps. No contact is claimed from a placeholder.',
    'Planet common midpoint shift is approximately along normal. It does not change normal torque in exact arithmetic; friction lever arms change, and the finite iterative solve couples friction and normal responses.',
    'Premature bounce and nonphysical separated-impact time remain FTFT4B OPEN. Four-point geometry does not solve this.',
    'Terrain/player propagation route is observed, but this diagnostic does not prove all later terrain, fluid, ship or player trajectories are physically more accurate.'
]
paths=[ROOT/'src'/n for n in ['PhysicsWorld.cpp','PhysicsWorld.h','Contacts.cpp','Contacts.h','ContactGeometryInternal.h','ContactSolver.cpp','ContactSolver.h','Narrowphase.cpp','Narrowphase.h','RigidBody.cpp','RigidBody.h','RigidBodyGravity.cpp','RadialTerrain.cpp','RadicalGravity.cpp','Simulation.cpp','TestHarness.cpp','RuntimeWorld.cpp','PlayerController.cpp']]
paths += list((ROOT/'docs/evidence/ftft4/pre-fix/source/src').glob('*'))
paths += [OUT/'contact_probe.cpp',OUT/'runtime_probe.cpp',Path(__file__).resolve()]
result['source_sha256']={str(p.relative_to(ROOT)):sha(p) for p in paths if p.is_file()}
result['executables_sha256']={str(p.relative_to(ROOT)):sha(p) for p in [ROOT/'build/ftft4_trajectory_baseline',ROOT/'build/ftft4_trajectory_final',ROOT/'build/ftft4_runtime_trajectory',ROOT/'build/libjudas_engine.a']}
(OUT/'analysis.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'trace_scenes':list(result['scenes']),'pair_first_differences':{k:v['first_difference_step'] for k,v in result['pair_diagnostics'].items()},'terrain_runtime':result['terrain_runtime_diagnostic']},indent=2))
