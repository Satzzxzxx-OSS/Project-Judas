#!/usr/bin/env python3
"""Reproducible ordinary M62 content; no engine demo switches or node scripts."""
from pathlib import Path
import json, shutil, subprocess, sys, math
root=Path(__file__).resolve().parents[1]; out=root/'projects/deformable_lab';baker=Path(sys.argv[1] if len(sys.argv)>1 else root/'build/judas_deformable_bake')
for d in ('Assets/deformables','Assets/materials','Assets/models','Assets/scripts','Assets/ui','Assets/fonts','Assets/prefabs','Assets/worlds','Scenes'):(out/d).mkdir(parents=True,exist_ok=True)
ids={name:f'6262626262626262626262626262{n:04x}' for n,name in enumerate(('curtain','cape','elastic','yielding','cushion','drape','model','controller','lab','ui','prefab','world'),1)}
def meta(path,key,kind): (out/(path+'.judasmeta')).write_text(f'JudasAssetMeta 1\nid "{ids[key]}"\ntype {kind}\nsource ""\n')
for name,args in [('curtain',['sheet','17','21','3','4','2']),('cape',['sheet','11','15','1.2','2','2']),('elastic',['block','4','2','2','3','1','1']),('yielding',['block','4','3','3','2','1.5','1.5']),('cushion',['block','4','2','4','2.5','1','2.5']),('drape',['sheet','13','13','2','2','2'])]:
 path=f'Assets/deformables/{name}.judasdeform';subprocess.run([str(baker),args[0],str(out/path),*args[1:]],check=True);meta(path,name,'deformable')
for name in ('bend_bar.glb','LICENSE.txt'):shutil.copyfile(root/'projects/save_lab/Assets/models'/name,out/'Assets/models'/name)
meta('Assets/models/bend_bar.glb','model','mesh')
for name in ('DejaVuSans.ttf','DejaVuSans.ttf.judasmeta','LICENSE.txt'):shutil.copyfile(root/'projects/save_lab/Assets/fonts'/name,out/'Assets/fonts'/name)
shutil.copyfile(root/'projects/save_lab/Assets/scripts/math.js',out/'Assets/scripts/math.js');(out/'Assets/scripts/math.js.judasmeta').write_text('JudasAssetMeta 1\nid "62626262626262626262626262629999"\ntype script\nsource ""\n')
shutil.copyfile(root/'docs/judasjs/examples/deformable.js',out/'Assets/scripts/example.js');(out/'Assets/scripts/example.js.judasmeta').write_text('JudasAssetMeta 1\nid "62626262626262626262626262629998"\ntype script\nsource ""\n')
for name in ('controller','lab'):meta(f'Assets/scripts/{name}.js',name,'script')
meta('Assets/ui/lab.judasui','ui','ui');meta('Assets/prefabs/cloth.judasprefab','prefab','prefab');meta('Assets/worlds/lab.judasworld','world','world')
# One normal two-sided material; color remains an ordinary authored override.
mat='JudasMaterial 1\nmodel 1\nalpha 0\nbase 1 1 1 1\nmetallic 0\nroughness 0.8\nemissive 0 0 0\nemission 1\nnormal 1\nocclusion 1\ncutoff .5\ndoubleSided 1\nflipV 0\nuv 1 1 0 0\n'+''.join(f'map {n} "" 10497 10497 9987 9729\n' for n in range(5))
(out/'Assets/materials/fabric.judasmat').write_text(mat)
(out/'Assets/materials/fabric.judasmat.judasmeta').write_text('JudasAssetMeta 1\nid "62626262626262626262626262629997"\ntype material\nsource ""\n')
# Existing format, authored content tailored to the new lab.
ui=(root/'projects/save_lab/Assets/ui/lab.judasui').read_text().replace('JUDAS / DYNAMIC LIQUID SURFACES','JUDAS / DEFORMABLE LAB').replace('SAVE LAB PAUSED','DEFORMABLE LAB PAUSED').replace('Loading basin assets...','CURTAIN | CAPE | ELASTIC | YIELDING | MIXED CONTACT').replace('No bulk particles.','Cloth and tetrahedra: normal fixed-step physics').replace('G: pickup/drop | P: tilt bucket | wheel: carry height','Aim + hold mouse: push fabric | G: pick up / drop a rigid prop')
old='WASD/mouse: move/look | Space/C: swim up/down | V: waves | Z/X: pool drain/refill | Q/E: 800 L | G/T/P: carry/throw/tilt | 1/2: scenes | Esc: pause';ui=ui.replace(old,'WASD + mouse | G/T: carry/throw | V: air | C: cape motion | B: load / H: bend-compress-twist | R: release pin | P: spawn | F3: region | 1/2: flat/radial | F5/F8: save/load | Esc')
(out/'Assets/ui/lab.judasui').write_text(ui)
project=(root/'projects/save_lab/save_lab.judasproj').read_text();project=project[:project.index('input-map ')]+project[project.index('input-map '):].split('\n',1)[0]+'\n';project=project.replace('name "Save / Load Lab"','name "Deformable Lab"').replace('startup-scene "Scenes/lab.judas"','startup-scene "Scenes/flat.judas"')
# Build logical map directly, without game-input polling in the native motor.
actions={'jump':'key:Space','interact':'key:G','throw':'key:T','air':'key:V','cape':'key:C','load':'key:B','elastic_mode':'key:H','release':'key:R','spawn':'key:P','region':'key:F3','flat':'key:1','radial':'key:2','save_game':'key:F5','load_game':'key:F8','pause':'key:Escape','restart':'key:F9','push':'mouse:Left','ui_click':'mouse:Left','ui_activate':'key:Return','ui_up':'key:Up','ui_down':'key:Down'}
axes={'move_x':[('key:A',-1),('key:D',1)],'move_y':[('key:S',-1),('key:W',1)],'look_x':[('mouse:dx',1)],'look_y':[('mouse:dy',1)],'look_stick_x':[('stick:RightX',1)],'look_stick_y':[('stick:RightY',1)],'wheel_y':[('mouse:wheelY',1)]}
raw=f'1 {len(actions)+len(axes)} '+''.join(f'"{n}" 0 1 "{binding}" 1 0 ' for n,binding in actions.items())+''.join(f'"{n}" 1 {len(bindings)} '+''.join(f'"{binding}" {scale} 0 ' for binding,scale in bindings) for n,bindings in axes.items())
project=project.split('input-map ',1)[0].replace('Judas Save / Load State Lab','Deformable Lab')+'legacy-gameplay "false"\nclassification '+json.dumps('JudasClassification 1 1 1 0 "pickup" 1 1 0 "Default" 1 1 0 "Default"')+'\ninput-map '+json.dumps(raw)+'\nworld-manifest "'+ids['world']+'"\n';project=project.replace('saveIdentity "save-lab"','saveIdentity "deformable-lab"');project=project.replace('name "Save Lab"','name "Deformable Lab"');(out/'deformable_lab.judasproj').write_text(project)
def v(a):return ' '.join(f'{x:.9g}' for x in a)
def obj(i,name,p,fields,q=(1,0,0,0)):
 defaults={}
 if '  render ' in fields:defaults.update({'render.half-extents':'0.5 0.5 0.5','render.radius':'0.5','render.color':'0.8 0.8 0.8','render.alpha':'1','render.secondary-color':'0.8 0.8 0.8','render.secondary-alpha':'1','render.mesh-asset':'""','render.texture-asset':'""'})
 if '  deformable.' in fields and '  render ' in fields:
  defaults['render.materials']=json.dumps('1 "62626262626262626262626262629997" 0')
 if '  body ' in fields:defaults.update({'body.half-extents':'0.5 0.5 0.5','body.radius':'0.5','body.terrain':'""','body.mass':'1','body.friction':'0.6','body.restitution':'0','body.initial-velocity':'0 0 0','body.pickable':'false','body.managed':'false','body.compound-count':'0'})
 if '  motor.enabled ' in fields:defaults.update({'motor.offset':'0 0 0','motor.stepHeight':'0.5','motor.supportDistance':'0.15','motor.skin':'0.02','motor.maxSlopeDegrees':'50','motor.gravityScale':'1','motor.reorientationDegreesPerSecond':'120','motor.interactionMass':'80','motor.maxPushImpulse':'20','motor.collisionLayer':'0','motor.collisionMask':str(2**64-1),'motor.requiredTags':'0','motor.excludedTags':'0'})
 if '  animation.enabled ' in fields:defaults.update({'animation.play-on-start':'true','animation.speed':'1','animation.time':'0'})
 if '  player-start ' in fields:defaults['player-start.yaw']='0'
 for key,value in defaults.items():
  if ('  '+key+' ') not in fields:fields+='  '+key+' '+value+'\n'
 return f'object {i} "{name}"\n  position {v(p)}\n  rotation {v(q)}\n  scale 1 1 1\n'+fields+'end\n'
def render(h,c):return f'  render box\n  render.half-extents {v(h)}\n  render.color {v(c)}\n'
def body(h,dyn=False,mass=2):return f'  body {"dynamic" if dyn else "static"} box\n  body.half-extents {v(h)}\n  body.mass {mass}\n  body.friction 0.6\n  body.restitution 0\n'
def script(name,properties={}):return f'  scripts 1\n  script.0.id 1\n  script.0.asset "{ids[name]}"\n  script.0.enabled true\n  script.0.properties '+json.dumps(json.dumps(properties))+'\n'
def deform(name,density=.5,shear=1e-5,volume=1e-7,attachments=[],plastic=False):
 fields={'asset':ids[name],'enabled':1,'selfContact':1,'substeps':4,'iterations':4,'collisionLayer':0,'collisionMask':2**64-1,'material.density':density,'material.stretchCompliance':1e-6,'material.shearCompliance':shear,'material.bendCompliance':.0004,'material.volumeCompliance':volume,'material.damping':.7,'material.thickness':.035,'material.friction':.5,'material.airDrag':0,'material.yieldStrain':.06 if plastic else 0,'material.plasticRate':1 if plastic else 0,'material.maximumPlasticStrain':.4,'airVelocity':'0 0 0'}
 text=''.join('  deformable.'+k+' '+json.dumps(str(value))+'\n' for k,value in fields.items());parts=[str(len(attachments))]
 for group,kind,target,joint,offset in attachments:parts += [json.dumps(group),str(kind),str(target),json.dumps(joint),v(offset),'1']
 # Property parser uses all normal generic property values as quoted strings.
 return text+'  deformable.attachments '+json.dumps(' '.join(parts))+'\n'
def settings(name,nextid=1000):return f'JudasScene 3\nsettings\n  name "{name}"\n  world-origin 0 0 0\n  sun-color 1 0.98 0.92\n  sun-direction -0.3 0.8 0.5\n  ambient 0.3 0.32 0.35\n  fluid-scale 1\n  fidelity-policy none\n  next-id {nextid}\nend\n'
for radial in (False,True):
 rows=[]
 if radial:rows.append(obj(1,'Actual radial field',(0,-20,0),'  gravity radial 9.81\n  gravity.region sphere 150\n'));rows.append(obj(2,'Planet',(0,-20,0),'  render sphere\n  render.radius 20\n  render.color 0.22 0.32 0.3\n  body static sphere\n  body.radius 20\n'))
 else:rows.append(obj(1,'Selected uniform field',(0,0,0),'  gravity uniform 9.81\n  gravity.region sphere 200\n'))
 rows.append(obj(3,'Laboratory deck',(10,-.3,0),render((28,.3,12),(.22,.28,.34))+body((28,.3,12))))
 rows.append(obj(10,'Script controlled visitor',(7,1.1,9),'  motor.enabled true\n  motor.radius 0.3\n  motor.halfHeight 0.6\n'+script('controller')+f'  ui.asset "{ids["ui"]}"\n  ui.name "lab"\n  ui.enabled true\n'))
 rows.append(obj(11,'Lab controls',(0,0,0),script('lab',{'radial':radial})))
 rows.append(obj(12,'Compatibility view seed',(7,1.1,9),'  player-start first-person\n'))
 rows.append(obj(100,'A - pinned curtain',(0,.4,0),render((1,2,.05),(.15,.55,.95))+deform('curtain',attachments=[('top',0,0,'',(0,0,0))])+ '  scripts 1\n  script.0.id 1\n  script.0.asset "62626262626262626262626262629998"\n  script.0.enabled true\n  script.0.properties "{}"\n'))
 rows.append(obj(101,'Curtain overhead rail',(0,4.5,0),render((1.8,.06,.06),(.85,.8,.5))+body((1.8,.06,.06))))
 rows.append(obj(110,'Brush / finite mass prop',(0,2,2),render((.35,.35,.35),(1,.6,.2))+body((.35,.35,.35),True,2)+'  tags 1\n'))
 rows.append(obj(200,'B - actual animated skeleton',(6,2,0),f'  render mesh\n  render.mesh-asset "{ids["model"]}"\n  render.color 0.85 0.72 0.3\n  animation.enabled true\n  animation.clip "Wave"\n  animation.loop true\n  animation.play-on-start true\n'))
 rows.append(obj(201,'B - shoulder cape',(6,1,.4),render((1,1,.05),(.8,.15,.22))+deform('cape',attachments=[('top',2,200,'Root/Elbow',(0,-2,.4))])))
 rows.append(obj(202,'B - authored moving body proxy',(6,2,0),body((.45,.8,.25))))
 rows.append(obj(300,'C - elastic beam',(12,2,0),render((1.5,.5,.5),(.1,.8,.6))+deform('elastic',density=10,shear=.0005,volume=1e-6,attachments=[('left',0,0,'',(0,0,0))])))
 rows.append(obj(301,'C - yielding specimen',(18,2,0),render((1,.75,.75),(.8,.45,.1))+deform('yielding',density=1,shear=.002,volume=.00002,attachments=[('left',0,0,'',(0,0,0))],plastic=True)))
 rows.append(obj(310,'Physical loading press',(18,3.3,0),render((.5,.2,.6),(.4,.42,.45))+body((.5,.2,.6),True,12)+'  tags 1\n'))
 rows.append(obj(400,'D - deformable cushion',(24,.6,0),render((1.25,.5,1.25),(.6,.25,.8))+deform('cushion',density=80,shear=.0005,volume=.000005)))
 rows.append(obj(401,'D - draping cloth',(24,2,0),render((1,.05,1),(.85,.75,.2))+deform('drape'),(math.sqrt(.5),-math.sqrt(.5),0,0)))
 rows.append(obj(402,'D - dynamic support',(24,.15,0),render((1.7,.15,1.7),(.35,.4,.45))+body((1.7,.15,1.7),True,10)))
 rows.append(obj(500,'Oblique field',(31,3,0),'  gravity uniform 4\n  gravity.region box 3 5 5\n',(math.sqrt(.5),0,0,math.sqrt(.5))))
 # Prioritize the small oblique region over the encompassing gravity field.
 oblique=rows.pop();rows.insert(0,oblique)
 rows.append(obj(501,'E - oblique curtain',(31,1,0),render((1,1,.05),(.8,.3,.9))+deform('cape',attachments=[('top',0,0,'',(0,0,0))])))
 rows.append(obj(700,'Overview secondary camera',(9,10,13),'  render-camera \n  camera.enabled true\n  camera.width 256\n  camera.height 144\n  camera.cadence 2\n  camera.fov 65\n  camera.near .1\n  camera.far 100\n',(math.cos(-.2),math.sin(-.2),0,0)))
 rows.append(obj(701,'Live secondary view',(7,2,5),render((1.8,1,.05),(1,1,1))+'  render.texture-camera 700\n'))
 for i,x in enumerate((0,6,12,18,24,31)):rows.append(obj(600+i,'Station marker',(x,.15,-2),render((1.5,.15,.2),[(.15,.55,.95),(.8,.15,.22),(.1,.8,.6),(.8,.45,.1),(.6,.25,.8),(.8,.3,.9)][i])))
 (out/'Scenes'/('radial.judas' if radial else 'flat.judas')).write_text(settings('Deformable Lab / '+('radial' if radial else 'uniform'))+'\n'.join(rows))
(out/'Scenes/annex.judas').write_text(settings('Suspended deformable region',3)+obj(1,'Annex specimen',(40,3,0),render((1,1,.1),(.3,.8,.9))+deform('cape',attachments=[('top',0,0,'',(0,0,0))]))+obj(2,'Annex floor',(40,-.3,0),render((4,.3,4),(.3,.3,.4))+body((4,.3,4))))
(out/'Assets/worlds/lab.judasworld').write_text('JudasWorld 1\nbudget 2 8 2 8388608 8388608 8388608 2097152\nregion "annex" "Scenes/annex.judas" 0 0 0 0 0 0 1 44 6 6 1 "snapshot" 262144 ""\n')
# Normal prefab serialization (internally one reusable entity).
prefab=settings('Independent cloth prefab',2)+obj(1,'Spawned patch',(0,0,0),render((1,1,.05),(.5,.8,.95))+deform('cape'))
(out/'Assets/prefabs/cloth.judasprefab').write_text(prefab)
print(out)
