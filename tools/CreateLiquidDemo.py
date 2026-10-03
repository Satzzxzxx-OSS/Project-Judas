"""Original M54 content. Geometry is authored physical tetrahedra, not render meshes."""
from pathlib import Path
import json, math, shutil, re
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'projects/liquid_reservoir_demo'
for folder in ('Scenes','Assets/liquid','Assets/scripts','Assets/ui','Assets/fonts','Assets/prefabs'):(P/folder).mkdir(parents=True,exist_ok=True)
ids={name:f'5454545454545454545454545454{i:04d}' for i,name in enumerate(('main','tank','bucket','planet','controller','lab','ui','prefab','math'),1)}
def meta(path,name,kind):Path(str(path)+'.judasmeta').write_text(f'JudasAssetMeta 1\nid "{ids[name]}"\ntype {kind}\nsource ""\n')
def cavity(name,vertices):
 tets=((0,1,3,7),(0,3,2,7),(0,2,6,7),(0,6,4,7),(0,4,5,7),(0,5,1,7));path=P/f'Assets/liquid/{name}.judascavity';path.write_text('JudasCavity1 6\n'+'\n'.join(' '.join(str(c) for i in t for c in vertices[i]) for t in tets)+'\n');meta(path,name,'liquid')
def boxvertices(a,b):return [(b[0] if i&1 else a[0], b[1] if i&2 else a[1],b[2] if i&4 else a[2]) for i in range(8)]
main=[]
for i in range(8):
 y=1.2 if i&2 else 0;hx=1.2 if i&2 else .8;hz=.8 if i&2 else .6;main.append((hx if i&1 else -hx,y,hz if i&4 else -hz))
cavity('main',main);cavity('tank',boxvertices((-.6,0,-.6),(.6,1.2,.6)));cavity('bucket',boxvertices((-.14,-.16,-.14),(.14,.16,.14)));cavity('planet',boxvertices((-1,0,-1),(1,1,1)))
for f in ('DejaVuSans.ttf','DejaVuSans.ttf.judasmeta','LICENSE.txt'):shutil.copyfile(ROOT/'projects/shooter_game/Assets/fonts'/f,P/'Assets/fonts'/f)
font=json.loads((P/'Assets/fonts/DejaVuSans.ttf.judasmeta').read_text().split('id ',1)[1].splitlines()[0])
ui=(ROOT/'projects/shooter_game/Assets/ui/range.judasui').read_text();ui=ui.replace('JUDAS / SPRING RANGE','JUDAS / CONSERVED LIQUID LAB').replace('RANGE PAUSED','LIQUID LAB PAUSED').replace('RESTART ROUND','RELOAD AUTHORED VOLUMES').replace('Clear every plate once. Hits score again after the hinge settles.','Volume is authoritative. Geometry is downstream.');ui=ui.replace('SCORE 0','Loading basin assets...').replace('12 HINGED PLATES','No bulk particles.').replace('Shoot all 12 plates','G: pickup/drop | P: tilt bucket | wheel: carry height');
ui=ui.replace('WASD / left stick: move   Mouse / right stick: aim   LMB / RT: fire   Space / A: jump   V / Y: view   R: restart   Esc: pause','WASD: move | mouse: look | 1/2: scenes | Q/E: 800 L out/back | F/H: spill fill/drain | Esc: pause');(P/'Assets/ui/lab.judasui').write_text(ui);meta(P/'Assets/ui/lab.judasui','ui','ui')
def vec(v):return ' '.join(f'{x:.9g}' for x in v)
def obj(i,name,pos,fields,q=(1,0,0,0)):return f'object {i} "{name}"\n  position {vec(pos)}\n  rotation {vec(q)}\n  scale 1 1 1\n'+fields+'end\n\n'
def render(h,c,alpha=1):return f'  render box\n  render.half-extents {vec(h)}\n  render.radius .5\n  render.color {vec(c)}\n  render.alpha {alpha}\n  render.secondary-color .3 .55 .65\n  render.secondary-alpha .25\n  render.mesh-asset ""\n  render.texture-asset ""\n'
def body(h,motion='static',mass=1,shape='box',boxes=[]):return f'  body {motion} {shape}\n  body.half-extents {vec(h)}\n  body.radius .5\n  body.terrain ""\n  body.mass {mass}\n  body.friction .6\n  body.restitution .05\n  body.initial-velocity 0 0 0\n  body.pickable false\n  body.managed false\n  body.compound-count {len(boxes)}\n'+''.join(f'  body.compound-box {vec(c)} {vec(h)}\n' for c,h in boxes)+'  body.fluid-cavity-count 0\n'
def liq(kind,d):return ''.join(f'  liquid.{kind}.{k} '+json.dumps(str(v))+'\n' for k,v in d.items())
def basin(i,name,pos,geom,volume,material='water',q=(1,0,0,0),ht=.002):return obj(i,name,pos,liq('basin',dict(enabled=1,geometry=ids[geom],asset='',initialVolume=volume,volumeTolerance=.00001,heightTolerance=ht,**{'material.id':material,'material.density':1000})),q)
def script(asset,props={}):return f'  scripts 1\n  script.0.id 1\n  script.0.asset "{ids[asset]}"\n  script.0.enabled true\n  script.0.properties '+json.dumps(json.dumps(props,separators=(',',':')))+'\n'
motor=''.join(line+'\n' for line in (ROOT/'projects/shooter_game/Scenes/range.judas').read_text().split('object 10 ',1)[1].split('end\n',1)[0].splitlines() if line.strip().startswith('motor.'))
def gravity(radial=False):return '  gravity '+('radial' if radial else 'uniform')+' 9.81\n  gravity.region sphere 1000\n'
def walls(start,center,h,q=(1,0,0,0)):
 # Simple physical outer walls. Main frustum also has an explicit sloped floor envelope;
 # visual cavity rim is distinct from the authored integration tetrahedra.
 pieces=[((0,-.06,0),(h[0],.06,h[2])),((h[0]+.06,h[1]/2,0),(.06,h[1]/2,h[2]+.12)),((-h[0]-.06,h[1]/2,0),(.06,h[1]/2,h[2]+.12)),((0,h[1]/2,h[2]+.06),(h[0],h[1]/2,.06)),((0,h[1]/2,-h[2]-.06),(h[0],h[1]/2,.06))]
 def rotate(v):w,x,y,z=q;return (v[0]*(1-2*y*y-2*z*z)+v[1]*(2*x*y-2*z*w)+v[2]*(2*x*z+2*y*w),v[0]*(2*x*y+2*z*w)+v[1]*(1-2*x*x-2*z*z)+v[2]*(2*y*z-2*x*w),v[0]*(2*x*z-2*y*w)+v[1]*(2*y*z+2*x*w)+v[2]*(1-2*x*x-2*y*y))
 s=''
 if start==40:
  # Inner faces follow x=+/-(.8+y/3), z=+/-(.6+y/6), matching the
  # authored tapered physical cavity rather than an unrelated outer box.
  s+=obj(start,'Tapered basin floor',(center[0],center[1]-.06,center[2]),render((.8,.06,.6),(.33,.4,.46))+body((.8,.06,.6)))
  for j,(offset,size,rotation) in enumerate((( (1.038,.588,0),(.04,.633,.8),(math.cos(math.atan(1/3)/2),0,0,-math.sin(math.atan(1/3)/2))), ((-1.038,.588,0),(.04,.633,.8),(math.cos(math.atan(1/3)/2),0,0,math.sin(math.atan(1/3)/2))), ((0,.593,.739),(.8,.609,.04),(math.cos(math.atan(1/6)/2),math.sin(math.atan(1/6)/2),0,0)), ((0,.593,-.739),(.8,.609,.04),(math.cos(math.atan(1/6)/2),-math.sin(math.atan(1/6)/2),0,0)))):
   s+=obj(start+j+1,'Tapered basin wall',tuple(a+b for a,b in zip(center,offset)),render(size,(.33,.4,.46))+body(size),rotation)
  return s
 for j,(offset,size) in enumerate(pieces):r=rotate(offset);pos=tuple(a+b for a,b in zip(center,r));s+=obj(start+j,'Physical basin wall',pos,render(size,(.33,.4,.46),.4 if j else 1)+body(size),q)
 return s
header=lambda name:f'JudasScene 3\nsettings\n  name "{name}"\n  world-origin 1000000000000 -2000000000000 3000000000000\n  sun-direction -.3 .8 .5\n  sun-color 1 .94 .85\n  ambient .4 .42 .5\n  fluid-scale 13\n  fidelity-policy none\n  next-id 2000\nend\n\n'
bucketboxes=[((0,-.18,0),(.18,.02,.18)),((-.16,0,0),(.02,.16,.18)),((.16,0,0),(.02,.16,.18)),((0,0,-.16),(.14,.16,.02)),((0,0,.16),(.14,.16,.02))]
bucketfields=render((.18,.18,.18),(.8,.6,.2)).replace('render box','render compound')+body((.18,.18,.18),'dynamic',2,'compound',bucketboxes)+'  tags 1\n'+liq('container',dict(enabled=1,geometry=ids['bucket'],initialVolume=0,**{'material.id':'water','material.density':1000},openingArea=.025,discharge=.6,opening='4 -.14 .16 -.14 .14 .16 -.14 .14 .16 .14 -.14 .16 .14'))
prefab=header('Conserved liquid bucket')+obj(1,'Bucket',(0,0,0),bucketfields);(P/'Assets/prefabs/bucket.judasprefab').write_text(prefab);meta(P/'Assets/prefabs/bucket.judasprefab','prefab','prefab')
def saveScene(name,text):
 path=P/f'Scenes/{name}.judas';previous=path.read_text() if path.exists() else '';bakes={}
 for m in re.finditer(r'object (\d+) [\s\S]*?\nend',previous):
  a=re.search(r'liquid.basin.asset "([^"]+)"',m[0])
  if a:bakes[m[1]]=a[1]
 def retain(m):return m[0].replace('liquid.basin.asset ""',f'liquid.basin.asset "{bakes.get(m[1], "")}"')
 path.write_text(re.sub(r'object (\d+) [\s\S]*?\nend',retain,text))
scene=header('Reservoir Lab')+obj(1,'Uniform gravity region',(0,0,0),gravity())+obj(2,'Lab floor',(0,-.2,-4),render((12,.2,10),(.12,.2,.25))+body((12,.2,10)))
scene+=obj(10,'Script character',(-3,1.2,-1.8),motor+script('controller'))
scene+=obj(11,'Liquid lab UI',(0,0,0),f'  ui.enabled true\n  ui.asset "{ids["ui"]}"\n  ui.name "lab"\n'+script('lab'))
scene+=basin(20,'MAIN / initially 1000 L',(-3,0,-3),'main',1)+basin(21,'STORAGE / 800 L transfer',(-.5,0,-5),'tank',0)+basin(22,'RECEIVER / pour here',(1.5,0,-3),'tank',0)
scene+=basin(30,'Spill depression A',(4,0,-6),'tank',.1,'graph-water')+basin(31,'Spill depression B',(5.5,0,-6),'tank',.1,'graph-water')+basin(32,'Spill experiment storage',(8,0,-6),'tank',1.6,'graph-water')
scene+=obj(33,'Saddle connection',(4.75,.4,-6),liq('connection',dict(enabled=1,bidirectional=1,source=30,destination=31,openingArea=.06,discharge=.6)))
for i,pos,h in ((40,(-3,0,-3),(1.2,1.2,.8)),(50,(-.5,0,-5),(.6,1.2,.6)),(60,(1.5,0,-3),(.6,1.2,.6)),(70,(4,0,-6),(.6,1.2,.6)),(80,(5.5,0,-6),(.6,1.2,.6)),(90,(8,0,-6),(.6,1.2,.6))):scene+=walls(i,pos,h)
scene+=obj(200,'Physical conserved bucket',(-1,.6,-1),f'  prefab.root 200\n  prefab.source 1\n  prefab.asset "{ids["prefab"]}"\n  prefab.ids "1 \\"1\\" \\"200\\" "\n  prefab.overrides "0 "\n')
scene+=obj(201,'Floating physical box',(-3,1.5,-3),render((.18,.18,.18),(.9,.25,.15))+body((.18,.18,.18),'dynamic',12)+'  tags 1\n'+liq('interaction',dict(enabled=1,drag=2)))
saveScene('lab',scene)
q=(math.cos(.45),0,0,math.sin(.45));up=(-math.sin(.9),math.cos(.9),0);origin=tuple(x*20 for x in up)
def relative(p):w,x,y,z=q;return (origin[0]+p[0]*math.cos(.9)-p[1]*math.sin(.9),origin[1]+p[0]*math.sin(.9)+p[1]*math.cos(.9),p[2])
radial=header('Radial Reservoir')+obj(1,'Actual Judas radial gravity',(0,0,0),gravity(True))
radial+=obj(2,'Planet',(0,0,0),render((20,20,20),(.22,.32,.25)).replace('render box','render sphere').replace('render.radius .5','render.radius 20')+body((20,20,20)).replace('body static box','body static sphere').replace('body.radius .5','body.radius 20'))
radial+=obj(10,'Script character',relative((0,1,1.8)),motor+script('controller'),q)+obj(11,'Liquid lab UI',(0,0,0),f'  ui.enabled true\n  ui.asset "{ids["ui"]}"\n  ui.name "lab"\n'+script('lab',{'radial':True}))
radial+=basin(120,'Radial lake',origin,'planet',2,'water',q,.025)+basin(121,'Radial storage',relative((4,0,0)),'tank',0,'water',q,.04)
radial+=walls(140,origin,(1,1,1),q)+walls(150,relative((4,0,0)),(.6,1.2,.6),q)
radial+=obj(201,'Radial buoyant box',relative((0,.8,0)),render((.15,.15,.15),(.9,.25,.15))+body((.15,.15,.15),'dynamic',8)+liq('interaction',dict(enabled=1,drag=2)),q)
saveScene('radial',radial)
# Reuse the normal input format but register only this project's actual actions.
actions={'jump':'Space','interact':'G','throw':'T','tilt':'P','drain':'Q','refill':'E','fill_graph':'F','drain_graph':'H','lab_scene':'1','radial_scene':'2','restart':'R','pause':'Escape','ui_up':'Up','ui_down':'Down','ui_activate':'Return','ui_click':'mouse:Left','spawn':'N'}
bindings=[]
for name,key in actions.items():bindings.append(f'"{name}" 0 1 "{key if key.startswith("mouse:") else "key:"+key}" 1 0')
axes={'move_x':[('key:A',-1,0),('key:D',1,0),('stick:LeftX',1,.15)],'move_y':[('key:S',-1,0),('key:W',1,0),('stick:LeftY',-1,.15)],'look_x':[('mouse:dx',1,0)],'look_y':[('mouse:dy',1,0)],'look_stick_x':[('stick:RightX',1,.15)],'look_stick_y':[('stick:RightY',1,.15)],'wheel_y':[('mouse:wheelY',1,0)]}
for name,items in axes.items():bindings.append(f'"{name}" 1 {len(items)} '+ ' '.join(f'"{key}" {scale} {dead}' for key,scale,dead in items))
project='JudasProject 1\nname "Conserved Liquid Lab"\nstartup-scene "Scenes/lab.judas"\nassets-dir "Assets"\nscenes-dir "Scenes"\nsaves-dir "Saves"\ninput-map '+json.dumps('1 '+str(len(bindings))+' '+' '.join(bindings))+'\nclassification '+json.dumps('JudasClassification 1 1 1 0 "pickup" 1 1 0 "Default" 1 1 0 "Default"')+'\n';(P/'liquid_reservoir_demo.judasproj').write_text(project)
for name in ('controller','lab','math'):meta(P/f'Assets/scripts/{name}.js',name,'script')
