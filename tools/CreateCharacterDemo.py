#!/usr/bin/env python3
"""Ordinary M49 content: no engine fixture switches or alternate motion path."""
from pathlib import Path
import shutil,json,math,re
ROOT=Path(__file__).resolve().parents[1];out=ROOT/'projects/character_demo'
for folder in ('Scenes','Assets/scripts','Assets/ui','Assets/fonts','Assets/prefabs'):(out/folder).mkdir(parents=True,exist_ok=True)
for name in ('DejaVuSans.ttf','DejaVuSans.ttf.judasmeta','LICENSE.txt'):shutil.copyfile(ROOT/'projects/fluid_demo/Assets/fonts'/name,out/'Assets/fonts'/name)
ids={'menu':'49494949494949494949494949494901','controller':'49494949494949494949494949494902','prefab':'49494949494949494949494949494903','platform':'49494949494949494949494949494904'}
for name,kind,path in [('menu','ui','Assets/ui/menu.judasui'),('controller','script','Assets/scripts/controller.js'),('platform','script','Assets/scripts/platform.js'),('prefab','prefab','Assets/prefabs/motor.judasprefab')]:
 (out/(path+'.judasmeta')).write_text(f'JudasAssetMeta 1\nid "{ids[name]}"\ntype {kind}\nsource ""\n')
(out/'Assets/ui/menu.judasui').write_text((ROOT/'projects/fluid_demo/Assets/ui/menu.judasui').read_text())
project=(ROOT/'projects/fluid_demo/fluid_demo.judasproj').read_text().replace('Production Fluid / Pool and Planet','Scriptable Character Motor').replace('Scenes/pool.judas','Scenes/flat.judas')
raw=json.loads(project.split('input-map ',1)[1]);raw=raw.replace('1 49 ','1 50 ',1).replace('fluid_pool','motor_flat').replace('fluid_planet','motor_planet')+' "motor_pool" 0 1 "key:F3" 1 0 '
(out/'character_demo.judasproj').write_text(project.split('input-map ',1)[0]+'input-map '+json.dumps(raw)+'\n')
def v(a):return ' '.join(f'{x:.9g}' for x in a)
motor='''  motor.enabled true
  motor.radius 0.3
  motor.halfHeight 0.6
  motor.offset 0 0 0
  motor.stepHeight 0.55
  motor.supportDistance 0.15
  motor.skin 0.02
  motor.maxSlopeDegrees 50
  motor.gravityScale 1
  motor.reorientationDegreesPerSecond 120
  motor.interactionMass 80
  motor.maxPushImpulse 20
  motor.collisionLayer 0
  motor.collisionMask 18446744073709551615
  motor.requiredTags 0
  motor.excludedTags 0
'''
def render(h,c):return f'  render box\n  render.half-extents {v(h)}\n  render.radius 0.5\n  render.color {v(c)}\n  render.alpha 1\n  render.secondary-color 0.8 0.8 0.8\n  render.secondary-alpha 1\n  render.mesh-asset ""\n  render.texture-asset ""\n'
def body(h,dynamic=False,mass=80):return f'''  body {'dynamic' if dynamic else 'static'} box
  body.half-extents {v(h)}
  body.radius 0.5
  body.terrain ""
  body.mass {mass}
  body.friction 0.6
  body.restitution 0
  body.initial-velocity 0 0 0
  body.pickable true
  body.managed false
  body.compound-count 0
  body.fluid-cavity-count 0
'''
def script(controlled=True):return f'  scripts 1\n  script.0.id 1\n  script.0.asset "{ids["controller"]}"\n  script.0.enabled true\n  script.0.properties '+json.dumps(json.dumps({'controlled':controlled}))+'\n'
def object(i,name,pos,fields,q=(1,0,0,0)):return f'object {i} "{name}"\n  position {v(pos)}\n  rotation {v(q)}\n  scale 1 1 1\n'+fields+'end\n'
def settings(name,nextid):return f'JudasScene 3\nsettings\n  name "{name}"\n  world-origin 0 0 0\n  sun-color 1 0.98 0.92\n  sun-direction -0.3 0.8 0.5\n  ambient 0.3 0.32 0.35\n  fluid-scale 13\n  fluid-update-rate-hz 20\n  fluid-hydrostatic-drag-rate 2\n  fidelity-policy none\n  next-id {nextid}\nend\n\n'
ui=f'  ui.asset "{ids["menu"]}"\n  ui.name "game_ui"\n  ui.enabled true\n'
for planet in (False,True):
 rows=[]
 if planet:
  # Real radial world. Main spawn and tangent course sit near its north pole.
  fields='  render sphere\n  render.radius 30\n  render.color 0.25 0.42 0.27\n  body static sphere\n  body.radius 30\n  body.friction 0.8\n  gravity radial 9.81\n  gravity.region sphere 100\n'
  # Borrow ordinary complete sphere component fields from the accepted content.
  sphere=(ROOT/'projects/fluid_demo/Scenes/planet.judas').read_text().split('object 1 ',1)[1].split('end\n',1)[0]
  rows.append('object 1 '+sphere+'end\n');height=30
 else:
  rows.append(object(1,'Floor',(0,-.5,0),render((20,.5,20),(.3,.36,.4))+body((20,.5,20))))
  rows.append(object(2,'Gravity',(0,0,0),'  gravity uniform 9.81\n  gravity.region sphere 100\n'));height=0
 rows.append(object(10,'Script controlled capsule',(0,height+1.1,6),motor+script()+ui))
 rows.append(object(11,'Independent scripted capsule',(3,height+1.1,4),motor+script(False)+render((.3,.9,.3),(.1,.7,1))))
 # Player-start is solely the old compatibility session's view seed. No legacy
 # locomotion runs once the project script supplies its independent main view.
 rows.append(object(12,'Compatibility view seed',(0,height+1.1,6),'  player-start first-person\n  player-start.yaw 0\n'))
 for i in range(4):rows.append(object(20+i,'Step',(0,height+.15*(i+1),1-i*.8),render((1,.15*(i+1),.45),(.8,.7,.35))+body((1,.15*(i+1),.45))))
 rows.append(object(25,'Wall',(4,height+2,-3),render((.2,2,4),(.7,.35,.2))+body((.2,2,4))))
 rows.append(object(26,'Corner',(1,height+2,-6),render((3,2,.2),(.7,.35,.2))+body((3,2,.2))))
 for i,angle in enumerate((25,65)):
  q=(math.cos(math.radians(angle)/2),0,0,math.sin(math.radians(angle)/2));rows.append(object(30+i,f'Slope {angle}',(-4-i*3,height+1,-2),render((2,.2,2),(.4,.7,.3))+body((2,.2,2)),q))
 rows.append(object(40,'Finite mass pushable',(2,height+.55,1),render((.5,.5,.5),(1,.3,.6))+body((.5,.5,.5),True,40)))
 rows.append(object(41,'Translating support',(-5,height+.45,5),render((2,.25,2),(.2,.6,.8))+body((2,.25,2),True,500)))
 rows.append(object(42,'Rotating support',(6,height+.45,5),render((2,.25,2),(.5,.4,.9))+body((2,.25,2),True,500)))
 for i,bid,typ,q in [(43,41,3,(1,0,0,0)),(44,42,1,(.707106781,0,0,.707106781))]:
  pos=(-5,height+.45,5) if bid==41 else (6,height+.45,5)
  fields=f'''  joint {typ}
  joint.body-a {bid}
  joint.body-b 0
  joint.anchor-a 0 0 0
  joint.anchor-b 0 0 0
  joint.frame-a {v(q)}
  joint.frame-b {v(q)}
  joint.enabled true
  joint.limits {'true' if typ==3 else 'false'}
  joint.lower -2
  joint.upper 2
  joint.motor true
  joint.speed {1 if typ==3 else .4}
  joint.max-force 10000
  joint.spring false
  joint.rest 0
  joint.stiffness 10
  joint.damping 1
'''
  if typ==3:fields+=f'  scripts 1\n  script.0.id 1\n  script.0.asset "{ids["platform"]}"\n  script.0.enabled true\n  script.0.properties "{{}}"\n'
  rows.append(object(i,'Physical support constraint',pos,fields))
 (out/'Scenes'/('planet.judas' if planet else 'flat.judas')).write_text(settings('M49 / '+('radial' if planet else 'flat'),50)+'\n'.join(rows))
# Same accepted water content in a NEW current project; historical and current
# approved fluid-demo files remain untouched. Only its controller/content changes.
pool=(ROOT/'projects/fluid_demo/Scenes/pool.judas').read_text();pool=re.sub(r'object (\d+) "Player"\n(.*?)end\n',lambda m:object(int(m[1]),'Script controlled capsule',(0,3.8,5.5),motor+script()),pool,flags=re.S)
pool=re.sub(r'object (\d+) "Project HUD"\n(.*?)end\n',lambda m:object(int(m[1]),'Project HUD',(0,0,0),ui),pool,flags=re.S)
(out/'Scenes/pool.judas').write_text(pool)
(out/'Assets/prefabs/motor.judasprefab').write_text(settings('Motor prefab',2)+object(1,'Independent motor',(0,0,0),motor+script(False)+render((.3,.9,.3),(.1,.7,1))))
(out/'Assets/scripts/platform.js').write_text("import {physics} from 'judas';export default class {constructor({entity}){this.entity=entity;this.sign=1;}fixedUpdate(){const j=physics.joint(this.entity);if(!j)return;const x=j.state.coordinate;if(x>1.8)this.sign=-1;if(x< -1.8)this.sign=1;j.setMotor(this.sign,10000);}}\n")
print(out)
