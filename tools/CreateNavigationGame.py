#!/usr/bin/env python3
"""Author ordinary M53 Spring Range content; no engine demo switches."""
from pathlib import Path
import json,re,math
root=Path(__file__).resolve().parents[1];project=root/'projects/shooter_game';scene=project/'Scenes/range.judas'
text=scene.read_text();text=text.split('\n# M53 navigation arena')[0]
# Enlarge the established range, preserving its existing plate assemblies.
text=text.replace('next-id 500','next-id 2000').replace('12 0.5 20','24 0.5 32')
# Old side walls become open access into the expanded wings; outer walls are authored below.
parts=re.split(r'(?=^object )',text,flags=re.M)
parts=[p for p in parts if not re.match(r'object (40|41|42|43) ',p) and not (re.match(r'object (\d+) ',p) and int(re.match(r'object (\d+) ',p)[1])>=500)]
text=''.join(parts).rstrip()+'\n\n# M53 navigation arena\n'
def v(a):return ' '.join(f'{x:.9g}' for x in a)
def obj(i,name,p,fields,q=(1,0,0,0)):
 return f'object {i} "{name}"\n  position {v(p)}\n  rotation {v(q)}\n  scale 1 1 1\n'+fields+'end\n\n'
def render(h,c):return f'  render box\n  render.half-extents {v(h)}\n  render.radius .5\n  render.color {v(c)}\n  render.alpha 1\n  render.secondary-color .8 .8 .8\n  render.secondary-alpha 1\n  render.mesh-asset ""\n  render.texture-asset ""\n'
def body(h):return f'  body static box\n  body.half-extents {v(h)}\n  body.radius .5\n  body.terrain ""\n  body.mass 4\n  body.friction .6\n  body.restitution 0\n  body.initial-velocity 0 0 0\n  body.pickable false\n  body.managed false\n  body.compound-count 0\n  body.fluid-cavity-count 0\n'
def nav(kind,fields):return ''.join('  nav.'+kind+'.'+k+' '+json.dumps(str(val))+'\n' for k,val in fields.items())
for i,(p,h) in enumerate([((24,2,-20.75),(.3,2,15.25)),((-24,2,-4),(.3,2,32)),((0,2,28),(24,2,.3)),((0,2,-36),(24,2,.3)),((-9,1.5,-3),(.35,1.5,12)),((-18,1.5,3),(5,1.5,.35)),((9,1.5,-15),(.35,1.5,12)),((18,1.5,-5),(5,1.5,.35))]):text+=obj(510+i,'Navigation arena wall',p,render(h,(.25,.33,.4))+body(h))
# Opening in the physical outer wall matches the explicit hover crossing.
text+=obj(523,'Navigation landing access',(24,2,12.75),render((.3,2,15.25),(.25,.33,.4))+body((.3,2,15.25)))
# Ramp and raised walkable platform.
q=(math.cos(.12),0,0,math.sin(.12))
text+=obj(520,'Walkable ramp',(-18,.7,-18),render((4,.2,2),(.4,.5,.6))+body((4,.2,2)),q)
text+=obj(521,'Raised platform',(-13,1.5,-18),render((1.3,.2,2),(.4,.5,.6))+body((1.3,.2,2)))
text+=obj(522,'Independent landing',(34,-.5,-4),render((3,.5,3),(.5,.3,.65))+body((3,.5,3)))
surface=dict(enabled=1,includeDynamic=0,profile=0,sources=18446744073709551615,halfExtents='25 6 33',cellSize=.2,cellHeight=.1,simplification=1.3,tileSize=32,minRegion=2,asset='')
text+=obj(550,'Arena navigation surface',(0,0,-4),nav('surface',surface))
other=surface.copy();other['halfExtents']='4 4 4';text+=obj(551,'Landing navigation surface',(34,0,-4),nav('surface',other))
text+=obj(552,'Explicit traversal link',(0,0,0),nav('link',dict(enabled=1,bidirectional=1,start='22 0 -4',end='32 0 -4',radius=.7,area=2)))
text+=obj(553,'Costly corridor',(-15,1,10),nav('modifier',dict(enabled=1,excludeSource=0,blocked=0,halfExtents='4 2 1',area=1)))
text+=obj(554,'Movable navigation blocker',(-16,1.5,-2),render((2,1.5,.7),(.95,.45,.1))+body((2,1.5,.7))+nav('obstacle',dict(enabled=1,cylinder=0,halfExtents='2 1.5 .7',radius=2,height=3,updateDistance=.1)))
ids={'enemy':'53535353535353535353535353535301','prefab':'53535353535353535353535353535302','arena':'53535353535353535353535353535303'}
for name,kind,path in [('enemy','script','Assets/scripts/enemy.js'),('prefab','prefab','Assets/prefabs/navigator.judasprefab'),('arena','script','Assets/scripts/arena.js')]:
 (project/path).parent.mkdir(parents=True,exist_ok=True);(project/(path+'.judasmeta')).write_text(f'JudasAssetMeta 1\nid "{ids[name]}"\ntype {kind}\nsource ""\n')
motor=''.join(line+'\n' for line in text.split('object 10 ',1)[1].split('end\n',1)[0].splitlines() if line.strip().startswith('motor.'))
agent=nav('agent',dict(enabled=1,avoidance=1,profile=0,areas=18446744073709551615,speed=2.8,arrival=.7,cornerDistance=.4,repathSeconds=.55,costs='1 3'))
script=f'  scripts 1\n  script.0.id 1\n  script.0.asset "{ids["enemy"]}"\n  script.0.enabled true\n  script.0.properties "{{}}"\n'
prefab='JudasScene 3\nsettings\n  name "Navigator"\n  world-origin 0 0 0\n  sun-color 1 1 1\n  sun-direction 0 1 0\n  ambient .3 .3 .3\n  fluid-scale 13\n  fidelity-policy none\n  next-id 3\nend\n\n'+obj(1,'Script navigator',(0,0,0),motor+agent+render((.32,.8,.32),(.85,.25,.3))+'  tags 4\n'+script)
prefab+=obj(2,'Query sensor',(0,0,0),body((.38,.85,.38))+'  parent 1\n  body.sensor true\n  tags 4\n')
(project/'Assets/prefabs/navigator.judasprefab').write_text(prefab)
# Linked prefab instances retain normal stable source mapping.
for index,p in enumerate([(-20,1,18),(-14,1,22),(-21,1,-15),(-15,1,-25),(18,1,18),(34,1,-4)]):
 i=1000+index*3
 text+=obj(i,'Navigator prefab',p,f'  prefab.root {i}\n  prefab.source 1\n  prefab.asset "{ids["prefab"]}"\n  prefab.ids '+json.dumps(f'2 "1" "{i}" "2" "{i+1}" ')+'\n  prefab.overrides "0 "\n')
text+=obj(560,'Arena script',(0,0,0),f'  scripts 1\n  script.0.id 1\n  script.0.asset "{ids["arena"]}"\n  script.0.enabled true\n  script.0.properties "{{}}"\n')
scene.write_text(text)
p=project/'shooter_game.judasproj';rows=p.read_text().splitlines();rows=[r for r in rows if not r.startswith('navigation ')];rows=[r.replace('JudasClassification 1 2 2 0', 'JudasClassification 1 3 3 0').replace('1 \\"physical\\" 1 1','1 \\"physical\\" 2 \\"navigator\\" 1 1') if r.startswith('classification ') else r for r in rows]
# Explicit stable project profiles and areas, distinct from other classifications.
rows.append('navigation '+json.dumps('JudasNavigationProject 1 3 3 0 "Default" 1 "Costly" 2 "Traversal" 1 1 0 "Default" 0.34 1.8 50 0.35'))
for idx,r in enumerate(rows):
 if r.startswith('input-map '):
  value=json.loads(r[len('input-map '):]);value=value.split(' \"blocker_toggle\"',1)[0].replace('1 17 ','1 15 ',1);value=value.replace('1 15 ','1 17 ',1)+' "blocker_toggle" 0 1 "key:B" 1 0 "spawn_navigator" 0 1 "key:N" 1 0';rows[idx]='input-map '+json.dumps(value)
p.write_text('\n'.join(rows)+'\n')
