#!/usr/bin/env python3
"""Original, redistributable M64 lab content; uses the shared normal collider cooker."""
from pathlib import Path
import math, hashlib, json, shutil, subprocess
root=Path(__file__).resolve().parents[1]
project=root/'projects/collision_lab'
assets=project/'Assets'
def asset(path, data=None, kind=None):
 p=assets/path;p.parent.mkdir(parents=True,exist_ok=True)
 if data is not None:p.write_text(data)
 identity=hashlib.md5(('Judas M64 original:'+path).encode()).hexdigest()
 if kind:(Path(str(p)+'.judasmeta')).write_text(f'JudasAssetMeta 1\nid "{identity}"\ntype {kind}\nsource ""\n')
 return identity

def obj(path,vertices,triangles):
 text='o PhysicalSource\n'+''.join('v %.9g %.9g %.9g\n'%tuple(v) for v in vertices)
 # Explicit per-face visual normals; collision welding uses source position IDs.
 for face in triangles:
  a,b,c=[vertices[i] for i in face];u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)];n=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]);length=math.sqrt(sum(x*x for x in n));text+='vn %.9g %.9g %.9g\n'%tuple(x/length for x in n)
 for normal,face in enumerate(triangles,1):text+='f '+' '.join(f'{i+1}//{normal}' for i in face)+'\n'
 source=asset('models/'+path+'.obj',text,'mesh');output='collision/'+path+'.judascollision';identity=asset(output,kind='collision')
 subprocess.run([root/'build/judas_collision_cook','hull' if path=='asymmetric' else 'mesh',assets/('models/'+path+'.obj'),assets/output,source,'0']+(['--two-sided'] if path=='doorway' else []),check=True)
 return source,identity

# Quarter pipe includes a connected flat approach, no hidden box fallback.
xs=[(-12,0),(0,0)]+[(6*math.sin(i*math.pi/64),6*(1-math.cos(i*math.pi/64))) for i in range(1,33)]
vertices=[(x,y,z) for x,y in xs for z in (-4,4)]
triangles=[]
for i in range(len(xs)-1):a=2*i;b=a+2;triangles.extend([(a,a+1,b+1),(a,b+1,b)])
ramp,rampphys=obj('quarterpipe',vertices,triangles)
# An open wall with a real doorway: grid connectivity preserves the opening.
vertices=[(x,y,0) for y in (0,4,6) for x in (-4,-2,2,4)];triangles=[]
for row in range(2):
 for col in range(3):
  if row==0 and col==1:continue
  a=row*4+col;triangles.extend([(a,a+1,a+5),(a,a+5,a+4)])
arch,archphys=obj('doorway',vertices,triangles)
# Shifted, tapered hull gives a visible pivot/COM and true non-box inertia.
vertices=[(x+1.2,y,z) for y,half in [(-.5,.7),(.7,.35)] for x,z in [(-half,-.4),(half,-.4),(half,.4),(-half,.4)]]
triangles=[(0,2,1),(0,3,2),(4,5,6),(4,6,7),(0,1,5),(0,5,4),(1,2,6),(1,6,5),(2,3,7),(2,7,6),(3,0,4),(3,4,7)]
hull,hullphys=obj('asymmetric',vertices,triangles)
# Original reconstruction baseline: 16 independent unrotated horizontal slabs.

mathid=asset('scripts/math.js',(root/'projects/fracture_lab/Assets/scripts/math.js').read_text(),'script')
controller=asset('scripts/controller.js',(root/'projects/fracture_lab/Assets/scripts/controller.js').read_text(),'script')
lab=asset('scripts/lab.js',kind='script');prefab=asset('prefabs/prop.judasprefab',kind='prefab')
world=asset('worlds/lab.judasworld','JudasWorld 1\nbudget 2 8 2 8388608 8388608 8388608 2097152\nregion "annex" "Scenes/annex.judas" 34 0 0 0 0 0.258819045 0.965925826 18 10 10 1 "snapshot" 262144 ""\n','world')
font=asset('fonts/DejaVuSans.ttf',kind='font');shutil.copyfile(root/'projects/fracture_lab/Assets/fonts/DejaVuSans.ttf',assets/'fonts/DejaVuSans.ttf');shutil.copyfile(root/'projects/fracture_lab/Assets/fonts/LICENSE.txt',assets/'fonts/LICENSE.txt')
ui=asset('ui/lab.judasui',(root/'projects/fracture_lab/Assets/ui/lab.judasui').read_text().replace('be36a149b9918581ce406f3632b79cab',font).replace('FRACTURE','COLLISION').replace('Slot: fracture-current','Slot: collision-current'),'ui')
project.mkdir(parents=True,exist_ok=True);(project/'Scenes').mkdir(exist_ok=True)
proj=(root/'projects/fracture_lab/fracture_lab.judasproj').read_text().replace('Fracture Lab','Collision Geometry Lab').replace('63636363636363636363636363630008',world)
(project/'collision_lab.judasproj').write_text(proj)
(project/'collision_lab_local.judasproj').write_text(proj.split('world-manifest')[0].replace('Collision Geometry Lab','Collision Geometry Lab (root saves)'))

def object_(id,name,pos=(0,0,0),fields='',rotation='1 0 0 0'):
 return f'object {id} {json.dumps(name)}\n  position '+ ' '.join(map(str,pos))+f'\n  rotation {rotation}\n  scale 1 1 1\n'+fields+'end\n\n'
def render(shape='box',half='1 1 1',mesh='',color='.4 .65 .8'):
 return f'  render {shape}\n  render.half-extents {half}\n  render.radius .5\n  render.color {color}\n  render.alpha 1\n  render.secondary-color .6 .75 .8\n  render.secondary-alpha 1\n  render.mesh-asset "{mesh}"\n  render.texture-asset ""\n'
def body(shape='box',dynamic=False,half='1 1 1',cook='',children=[]):
 return f'  body {"dynamic" if dynamic else "static"} {shape}\n  body.half-extents {half}\n  body.radius .5\n  body.terrain ""\n  body.collision-asset "{cook}"\n  body.mass 4\n  body.friction .3\n  body.restitution .05\n  body.initial-velocity 0 0 0\n  body.pickable true\n  body.managed false\n  body.compound-count {len(children)}\n'+''.join('  body.compound-box '+c+'\n' for c in children)
def script(id):return f'  scripts 1\n  script.0.id 1\n  script.0.asset "{id}"\n  script.0.enabled true\n  script.0.properties "{{}}"\n'
header='JudasScene 3\nsettings\n  name "Collision geometry lab"\n  world-origin 0 0 0\n  sun-color 1 .98 .92\n  sun-direction -.3 .8 .5\n  ambient .3 .32 .35\n  fluid-scale 1\n  fidelity-policy none\n  next-id 5000\nend\n'
player=(root/'projects/fracture_lab/Scenes/flat.judas').read_text().split('object 10 "Visitor"')[1].split('\nend')[0]
player='object 10 "Visitor"'+player+'\nend\n';player=player.replace('63636363636363636363636363630004',controller).replace('63636363636363636363636363630006',ui)
scene=header+object_(1,'Uniform field',fields='  gravity uniform 9.81\n  gravity.region sphere 200\n')+object_(3,'Deck',(5,-.35,0),render(half='29 .35 18',color='.14 .19 .25')+body(half='29 .35 18'))+player+'\n'
scene+=object_(11,'Lab script',fields=script(lab))+object_(12,'View seed',(0,1.1,8),'  player-start first-person\n  player-start.yaw 0\n')
scene+=object_(100,'Actual quarterpipe',(-3,0,-6),render('mesh',mesh=ramp,color='.15 .65 .55')+body('triangle-mesh',cook=rampphys))
scene+=object_(101,'Open doorway',(9,0,-3),render('mesh',mesh=arch,color='.8 .6 .2')+body('triangle-mesh',cook=archphys))
scene+=object_(200,'Asymmetric hull',(0,2,0),'  tags 1\n'+render('mesh',mesh=hull,color='.85 .3 .2')+body('hull',True,cook=hullphys))
children=['0 0 0 1 .15 .25 .923879533 0 0 .382683432 1 0 "" 17','-1 -.4 0 .1 .1 .1 1 0 0 0 0 .4 "" 22',f'1 .7 0 .1 .1 .1 .965925826 0 .258819045 0 5 0 "{hullphys}" 31']
scene+=object_(201,'Oriented compound',(4,2,2),'  tags 1\n'+render('compound')+body('compound',True,children=children))
scene+=object_(220,'Closest point marker',(0,2,0),render('sphere',color='1 .1 .8'))
# Box-chain visible comparison remains separate from the mesh.
for i in range(16):
 theta=(i+.5)*math.pi/32;x=6*math.sin(theta);y=6*(1-math.cos(theta));width=6*(math.sin((i+1)*math.pi/32)-math.sin(i*math.pi/32))
 scene+=object_(300+i,'Legacy facet slab',(15+x,y-.15,-6),render(half=f'{width/2} .15 4',color='.55 .25 .2')+body(half=f'{width/2} .15 4'))
# Small original M62/M63 assets, baked through existing generic tools.
import re
base=(root/'projects/deformable_lab/Scenes/flat.judas').read_text()
for source,sourceid,newid,pos in [('drape',401,400,(-.1,3,-6)),('cushion',400,401,(-2,4,-8))]:
 path='deformables/'+source+'.judasdeform';identity=asset(path,kind='deformable');subprocess.run([root/'build/judas_deformable_bake', 'sheet' if source=='drape' else 'block', assets/path]+(['5','5','3','3','1'] if source=='drape' else ['2','2','2','1.6','1.6','1.6']),check=True)
 section=re.search(r'object '+str(sourceid)+r' .*?\nend',base,re.S).group()
 fields='\n'.join(line for line in section.splitlines() if line.strip().startswith('deformable.'))+'\n'
 fields=re.sub(r'deformable.asset "[^"]+"',f'deformable.asset "{identity}"',fields)
 scene+=object_(newid,'Mesh cloth drape' if source=='drape' else 'Mesh soft contact',pos,render(color='.8 .6 .15')+fields,rotation='.707106781 -.707106781 0 0' if source=='drape' else '1 0 0 0')
path='deformables/fragment.judasdeform';identity=asset(path,kind='deformable');subprocess.run([root/'build/judas_deformable_bake','fracture-rigid',assets/path,'2','2','1','1.5','1.5','.3'],check=True)
section=re.search(r'object 100 .*?\nend',(root/'projects/fracture_lab/Scenes/flat.judas').read_text(),re.S).group()
fields='\n'.join(line for line in section.splitlines() if line.strip().startswith('deformable.'))+'\n'
fields=re.sub(r'deformable.asset "[^"]+"',f'deformable.asset "{identity}"',fields)
fields=re.sub(r'deformable.attachments .*', 'deformable.attachments "0"',fields)
scene+=object_(402,'Ordinary fracture fragment contact',(-1,5,-4),render(color='.5 .4 .8')+fields)
# Two normal spatial voices expose opening versus material obstruction.
import wave,struct
sound=asset('audio/tone.wav',kind='audio')
with wave.open(str(assets/'audio/tone.wav'),'wb') as f:
 f.setnchannels(1);f.setsampwidth(2);f.setframerate(48000);f.writeframes(b''.join(struct.pack('<h',int(4000*math.sin(2*math.pi*800*i/48000))) for i in range(48000)))
scene+=object_(460,'Fixed acoustic observer',(9,2,1),'  audio-listener\n  listener.enabled true\n  listener.follow-view false\n')
for id,x in [(461,9),(462,5)]:
 fields=f'  audio-emitter\n  audio.asset "{sound}"\n  audio.enabled true\n  audio.play-on-start true\n  audio.loop true\n  audio.spatial true\n  audio.volume .1\n  audio.pitch 1\n  audio.reference-distance 1\n  audio.maximum-distance 30\n  audio.rolloff 1\n  audio.attenuation none\n  audio.loading buffered\n  audio.group "master"\n  audio.occlusion true\n'
 scene+=object_(id,'Opening tone' if id==461 else 'Obstructed tone',(x,2,-6),render('sphere',color='.2 .8 .8')+fields)
# Bake nav from physical geometry, including the actual doorway opening.
scene+=object_(450,'Physical navigation surface',(5,0,0),'  nav.surface.asset ""\n  nav.surface.enabled "1"\n  nav.surface.halfExtents "30 10 18"\n  nav.surface.profile "0"\n  nav.surface.sources "18446744073709551615"\n  nav.surface.includeDynamic "0"\n  nav.surface.cellSize "0.2"\n  nav.surface.cellHeight "0.1"\n  nav.surface.tileSize "32"\n  nav.surface.minRegion "2"\n  nav.surface.simplification "1.3"\n')
(project/'Scenes/flat.judas').write_text(scene)
(assets/'prefabs/prop.judasprefab').write_text(header+object_(1,'Spawned hull',(0,0,0),'  tags 1\n'+render('mesh',mesh=hull,color='.6 .3 .9')+body('hull',True,cook=hullphys)))
(project/'Scenes/annex.judas').write_text(header+object_(1,'Region mesh',fields=render('mesh',mesh=ramp)+body('triangle-mesh',cook=rampphys))+object_(2,'Region compound',(0,3,0),'  tags 1\n'+render('compound')+body('compound',True,children=children)))
labjs='''import {world,input,ui,scenes,saves,physics} from 'judas';
import {mul,add} from './math.js';
export default class {
 constructor(){this.state={spawned:[],region:false,slot:'collision-current',shots:0};this.pending=[];this.request=0;this.message='Green: true mesh | red: separate legacy facets | doorway is physically open.';this.region=0;}
 start(){this.hud=ui.get('lab');if(this.hud){this.hud.get('pause').visible=false;this.hud.modal=false;this.hud.get('controls').text='WASD/mouse | Space | click | G/T | P | 1 lab / 2 mesh / 3 boxes | V assist / C break | Esc';}}
 restore(){this.start();if(this.state.region)this.region=scenes.requestRegion('annex');}
 update(){if(this.hud?.modal)return;for(const action of ['spawn','region','push','adopt','cape'])if(input.pressed(action))this.pending.push(action);if(input.pressed('restart'))scenes.reload();for(const [a,s] of [['flat','flat'],['radial','skate-mesh'],['game','skate-boxes']])if(input.pressed(a))scenes.load('Scenes/'+s+'.judas');}
 fixedUpdate(){const c=world.entity('200')?.collider;this.state.inspectedHull=c?.type==='hull'&&!!c.asset&&c.vertexCount===8;const q=physics.closestPoint({x:0,y:3,z:0},10,{ignored:[world.entity('10')]});this.state.closest=!!q&&q.distance>=0&&q.entity?.valid;for(const action of this.pending){
  if(action==='spawn'&&this.state.spawned.length<12){const r=world.viewRay;if(r){const e=world.spawnPrefab(PREFAB,{position:add(r.origin,mul(r.direction,3))});this.state.spawned.push(e.id);}}
  if(action==='region'){if(this.region){scenes.releaseRegion(this.region);this.region=0;this.state.region=false;}else{this.region=scenes.requestRegion('annex');this.state.region=true;}}
  if(action==='adopt'){const e=scenes.resolveRegionEntity('annex',2);if(e)scenes.adopt(e,'root');}
  if(action==='cape'){const f=world.entity('402')?.fracture;if(f){const s=f.state;const i=s.interfaces.find(i=>!i.broken);if(i)this.state.fragmentReleased=f.release(i.key,s.revision);}}
  if(action==='push'){const r=world.viewRay;if(r){const h=physics.raycast(r.origin,r.direction,60,{ignored:[world.entity('10')]});if(h?.entity?.hasTag('pickup')){h.entity.applyImpulseAtPoint(mul(r.direction,18),h.point);this.state.shots++;}}}
 }this.pending=[];}
 uiUpdate(){if(!this.hud)return;if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;input.pointerCapture=!this.hud.modal;}if(input.pressed('save_game'))this.command('save');if(input.pressed('load_game'))this.command('load');if(this.request){const s=saves.status(this.request);if(s){this.message=s.state+(s.error?' — '+s.error:'');if(['completed','failed','cancelled'].includes(s.state))this.request=0;}}
  const r=world.viewRay;let text='No collider hit';if(r){const h=physics.raycast(r.origin,r.direction,60,{ignored:[world.entity('10')]});if(h?.entity){const c=h.entity.collider;const p=physics.closestPoint(r.origin,60,{ignored:[world.entity('10')]});if(p)world.entity('220').transform={position:p.point};text=`${c.type} | ${c.triangleCount} triangles | child ${h.childKey} / feature ${h.feature} | ${c.children.map(c=>c.key+':'+c.type).join(', ')}`;}}
  this.hud.get('title').text='M64 / COLLISION GEOMETRY LAB';this.hud.get('score').text=text;this.hud.get('progress').text=this.regions()+' | F3 annex / H adopt | F5/F8 saves (root) | F9 reload';this.hud.get('message').text=this.message;
 }
 regions(){try{return scenes.regions.map(r=>r.id+': '+r.state).join(' / ');}catch{return 'Root world — saves enabled (no additive manifest)';}}
 command(op){if(!this.request)try{this.request=op==='save'?saves.save(this.state.slot):op==='load'?saves.load(this.state.slot):saves.delete(this.state.slot);}catch(e){this.message=String(e);}}
 onUI(e){if(e.type==='back'||e.type==='click'&&e.element==='resume'){this.hud.modal=false;this.hud.get('pause').visible=false;input.pointerCapture=true;}if(e.type==='click'&&e.element==='restart')scenes.reload();if(e.type==='click'&&e.element==='quit')ui.quit();if(e.type==='click'&&['save','load','delete'].includes(e.element))this.command(e.element);}
 destroy(){if(this.region)scenes.releaseRegion(this.region);}
}
'''.replace('PREFAB',json.dumps(prefab))
(assets/'scripts/lab.js').write_text(labjs)
skate=asset('scripts/skate.js',kind='script')
(assetspath:=assets/'scripts/skate.js').write_text("""import {input,physics,world} from 'judas';
import {add,sub,mul,dot,length,norm,rotate} from './math.js';
// Original reconstruction of the audit's four-ray chassis + seam transport.
// This is not Claude's unavailable skate script or a native engine mode.
export default class {
 constructor({entity}){this.entity=entity;this.state={assist:true,seconds:0,min:8,maxX:-5};}
 update(){if(input.pressed('air'))this.state.assist=!this.state.assist;}
 fixedUpdate(dt){const t=this.entity.transform;const up={x:0,y:1,z:0}; // Authored uniform field of THIS project fixture, not an engine gravity rule.
 let n={x:0,y:0,z:0},hits=0;
  for(const x of [-.5,.5])for(const z of [-.22,.22]){const origin=add(t.position,rotate(t.rotation,{x,y:0,z}));const hit=physics.raycast(origin,mul(up,-1),1,{ignored:[this.entity]});if(hit){hits++;n=add(n,hit.normal);const pointVelocity=add(this.entity.velocity,{x:0,y:0,z:0});const spring=Math.max(0,(.4-hit.distance)*140-dot(pointVelocity,up)*10);this.entity.applyForce(mul(up,spring));this.entity.applyTorque({x:(origin.y-t.position.y)*up.z*spring-(origin.z-t.position.z)*up.y*spring,y:(origin.z-t.position.z)*up.x*spring-(origin.x-t.position.x)*up.z*spring,z:(origin.x-t.position.x)*up.y*spring-(origin.y-t.position.y)*up.x*spring});}}
  if(hits&&this.state.assist){n=norm(n);const v=this.entity.velocity,projected=sub(v,mul(n,dot(v,n)));if(length(projected)>.1)this.entity.velocity=mul(norm(projected),length(v));}
  this.state.seconds+=dt;this.state.min=Math.min(this.state.min,length(this.entity.velocity));this.state.maxX=Math.max(this.state.maxX,t.position.x);
 }
}
""")
for baseline in [False,True]:
 skateboard=header+object_(1,'Gravity',fields='  gravity uniform 9.81\n  gravity.region sphere 200\n')+object_(3,'Observation deck',(5,-.6,0),render(half='29 .1 18')+body(half='29 .1 18'))+player+object_(11,'Lab script',fields=script(lab))+object_(12,'View seed',(-6,1.1,3),'  player-start first-person\n  player-start.yaw 0\n')
 if not baseline:skateboard+=object_(100,'Actual quarterpipe',(0,0,-6),render('mesh',mesh=ramp,color='.15 .65 .55')+body('triangle-mesh',cook=rampphys))
 else:
  skateboard+=object_(100,'Flat approach',(-6,-.15,-6),render(half='6 .15 4')+body(half='6 .15 4'))
  for i in range(16):
   theta=(i+.5)*math.pi/32;x=6*math.sin(theta);y=6*(1-math.cos(theta));width=6*(math.sin((i+1)*math.pi/32)-math.sin(i*math.pi/32))
   skateboard+=object_(300+i,'Separate baseline slab',(x,y-.15,-6),render(half=f'{width/2} .15 4',color='.55 .25 .2')+body(half=f'{width/2} .15 4'))
 skateboard+=object_(200,'Four ray chassis',(-5,.4,-6),render(half='.65 .08 .3',color='.8 .3 .7')+body(half='.65 .08 .3',dynamic=True).replace('body.initial-velocity 0 0 0','body.initial-velocity 8 0 0')+script(skate))
 skateboard+=object_(201,'Inspection compound',(4,2,2),render('compound')+body('compound',True,children=children))+object_(220,'Marker',fields=render('sphere'))
 (project/'Scenes'/('skate-boxes.judas' if baseline else 'skate-mesh.judas')).write_text(skateboard)
print('Original collision lab:',project)
