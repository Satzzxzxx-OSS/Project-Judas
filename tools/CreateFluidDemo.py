#!/usr/bin/env python3
"""Author ordinary fluid scenes; no runtime fluid manipulation or special solver."""
import json,math,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'projects/fluid_demo'
for p in ('Scenes','Assets/scripts','Assets/ui','Assets/fonts'):(DEST/p).mkdir(parents=True,exist_ok=True)
FONT='41414141414141414141414141414103';UI='fdfdfdfdfdfdfdfdfdfdfdfdfdfdfd01';SCRIPT='fdfdfdfdfdfdfdfdfdfdfdfdfdfdfd02'
for name in ('DejaVuSans.ttf','DejaVuSans.ttf.judasmeta','LICENSE.txt'):
 shutil.copyfile(ROOT/'projects/scene_demo/Assets/fonts'/name,DEST/'Assets/fonts'/name)
(DEST/'Assets/ui/menu.judasui').write_text((ROOT/'projects/scene_demo/Assets/ui/menu.judasui').read_text().replace('37373737373737373737373737373737',''))
def meta(path,identity,kind):(DEST/(path+'.judasmeta')).write_text(f'JudasAssetMeta 1\nid "{identity}"\ntype {kind}\nsource ""\n')
meta('Assets/ui/menu.judasui',UI,'ui');meta('Assets/scripts/fluid.js',SCRIPT,'script')
project=(ROOT/'judas_tech_demo.judasproj').read_text()
project=project.replace('name "Judas Technology Demonstration"','name "Production Fluid / Pool and Planet"').replace('startup-scene "assets/scenes/terrain.judas"','startup-scene "Scenes/pool.judas"').replace('assets-dir "assets"','assets-dir "Assets"').replace('scenes-dir "assets/scenes"','scenes-dir "Scenes"').replace('saves-dir "saves"','saves-dir "Saves"')
# Logical scene selection is content; leave physical/gameplay input in M35.
line=project.splitlines()[-1];encoded=line[len('input-map '):];raw=json.loads(encoded)
raw=raw.replace('1 47 ','1 49 ',1)+' "fluid_pool" 0 1 "key:F1" 1 0 "fluid_planet" 0 1 "key:F2" 1 0 '
project=project[:project.rfind('input-map ')]+f'input-map {json.dumps(raw)}\n'
(DEST/'fluid_demo.judasproj').write_text(project)
(DEST/'Assets/scripts/fluid.js').write_text('''import {ui,input,scenes} from 'judas';
export default class {
 start(){this.hud=ui.get('game_ui');this.hud.modal=false;ui.debugOverlayVisible=false;
  for(const id of ['main','options','pause','image'])this.hud.get(id).visible=false;
  this.hud.get('hud').visible=true;
  this.hud.get('hud_title').text=scenes.current==='Scenes/pool.judas'?'PRODUCTION FLUID / FLAT POOL':'PRODUCTION FLUID / RADIAL PLANET';
  this.hud.get('hud_help').text='WASD + mouse: move/look | Space: swim up / jump\\nG: pick up/drop | H: throw | Aim down to dip the open bucket\\nF1: flat pool | F2: planet | R: reset | Escape: pause';
  this.hud.get('counter').text='Orange block floats; purple block sinks.\\nClear-sided bucket carries actual particles: dip, lift, tilt to pour.\\nUse the steps at the near side to leave the basin.';
  this.hud.get('pause_options').text='Switch pool / planet';
 }
 uiUpdate(){if(input.pressed('fluid_pool'))scenes.load('Scenes/pool.judas');if(input.pressed('fluid_planet'))scenes.load('Scenes/planet.judas');
  if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;}}
 onUI(e){if(e.type==='back'||(e.type==='click'&&e.element==='resume')){this.hud.modal=false;this.hud.get('pause').visible=false;}
  if(e.type==='click'&&e.element==='pause_options')scenes.load(scenes.current==='Scenes/pool.judas'?'Scenes/planet.judas':'Scenes/pool.judas');
  if(e.type==='click'&&e.element==='pause_quit')ui.quit();}
}
''')
def vec(v):return ' '.join(f'{x:.9g}' for x in v)
def create(planet):
 angle=-math.pi/4 if planet else 0;c=math.cos(angle);s=math.sin(angle)
 q=(math.cos(angle/2),0,0,math.sin(angle/2));origin=(30/math.sqrt(2),30/math.sqrt(2),0) if planet else (0,0,0)
 def point(p):return (origin[0]+c*p[0]-s*p[1],origin[1]+s*p[0]+c*p[1],p[2])
 rows=[];idx=0;solids=[]
 def obj(name,p=(0,0,0),fields='',world=False):
  nonlocal idx
  idx+=1;pos=p if world else point(p);rot=(1,0,0,0) if world else q
  defaults={}
  if '  render ' in fields:defaults.update({'render.half-extents':'0.5 0.5 0.5','render.radius':'0.5','render.color':'0.8 0.8 0.8','render.alpha':'1','render.secondary-color':'0.8 0.8 0.8','render.secondary-alpha':'1','render.mesh-asset':'""','render.texture-asset':'""'})
  if '  body ' in fields:defaults.update({'body.half-extents':'0.5 0.5 0.5','body.radius':'0.5','body.terrain':'""','body.mass':'1','body.friction':'0.6','body.restitution':'0.1','body.initial-velocity':'0 0 0','body.pickable':'false','body.managed':'false','body.compound-count':'0','body.fluid-cavity-count':'0'})
  if '  fluid-volume\n' in fields:defaults.update({'fluid-volume.emitter-offset':'0 0 0','fluid-volume.max-particles':'0'})
  seen={line.strip().split()[0] for line in fields.splitlines() if line.strip()}
  for key,val in defaults.items():
   if key not in seen:fields+=f'  {key} {val}\n'
  rows.append(f'object {idx} "{name}"\n  position {vec(pos)}\n  rotation {vec(rot)}\n  scale 1 1 1\n'+fields+'end\n');return idx
 def box(name,p,h,color=(.5,.55,.6),dynamic=False,mass=1):
  fields=f'  render box\n  render.half-extents {vec(h)}\n  render.color {vec(color)}\n  body {"dynamic" if dynamic else "static"} box\n  body.half-extents {vec(h)}\n  body.mass {mass}\n  body.friction 0.8\n  body.restitution 0\n  body.pickable {str(dynamic).lower()}\n'
  if not dynamic:solids.append((p,h))
  return obj(name,p,fields)
 if planet:
  obj('Spherical world',(0,0,0),'  render sphere\n  render.radius 30\n  render.color 0.25 0.42 0.27\n  body static sphere\n  body.radius 30\n  body.friction 0.8\n  gravity radial 9.81\n  gravity.region sphere 100\n',True)
 else:
  box('Flat ground',(0,-.25,0),(14,.25,14),(.26,.32,.29))
  obj('Uniform gravity',(0,0,0),'  gravity uniform 9.81\n  gravity.region sphere 100\n')
 for p,h in [((-3.5,1.2,0),(.15,1.6,3.65)),((3.5,1.2,0),(.15,1.6,3.65)),((0,1.2,-3.5),(3.35,1.6,.15)),((0,1.2,3.5),(3.35,1.6,.15))]:box('Basin wall',p,h,(.36,.44,.5))
 box('Entry deck',(0,1.25,5.4),(4.5,1.55,1.75),(.65,.62,.48))
 # Six solid stair treads lead from the floor through water to the deck.
 for k in range(7):
  height=.4*(k+1);z=.9+.4*k
  box('Exit step',(0,height/2,z),(.7,height/2,.3),(.7,.7,.65))
 # Empty resolved open container: five collision boxes and one cavity.
 # Resolved walls meet at faces instead of overlapping into thin union
 # partitions. Same fixed quadrature per solid; no sampler quality change.
 pieces=[((0,-.86,0),(.97,.12,.97)),((-.85,.06,0),(.12,.8,.97)),((.85,.06,0),(.12,.8,.97)),((0,.06,-.85),(.73,.8,.12)),((0,.06,.85),(.73,.8,.12))]
 fields='  render compound\n  render.color 0.7 0.75 0.8\n  render.secondary-color 0.6 0.85 0.9\n  render.secondary-alpha 0.25\n  body dynamic compound\n  body.mass 1500\n  body.friction 0.8\n  body.restitution 0\n  body.pickable true\n  body.compound-count 5\n'
 for p,h in pieces:fields+=f'  body.compound-box {vec(p)} {vec(h)}\n'
 fields+='  body.fluid-cavity-count 1\n  body.fluid-cavity 0 0.06 0 0.73 0.8 0.73\n'
 cup=obj('Open carrying bucket',(2,3.9,4.7),fields)
 floating=box('Low density block',(-2.3,3.4,4.5),(.4,.4,.4),(1,.45,.1),True,256)
 sinking=box('High density block',(-1.2,3.4,4.5),(.4,.4,.4),(.65,.3,.8),True,1024)
 # Build real regular lattice rows with only authored solid space excluded.
 spacing=.65;side=10;layers=4;width=spacing*side;count=0
 for y in range(layers):
  for z in range(side):
   start=None
   for x in range(side+1):
    p=((x+.5)*spacing-width/2,(y+.5)*spacing,(z+.5)*spacing-width/2)
    admitted=x<side and not any(all(abs(p[i]-b[i])<h[i] for i in range(3)) for b,h in solids)
    if admitted and start is None:start=x
    if not admitted and start is not None:
     n=x-start;obj('Initial water row',((start+n/2)*spacing-width/2,p[1],p[2]),f'  fluid-volume\n  fluid-volume.spacing {spacing}\n  fluid-volume.count {n} 1 1\n  fluid-volume.emitter false\n');count+=n;start=None
 player=obj('Player',(0,3.8,5.5),'  player-start first-person\n  player-start.yaw 0\n  player-start.density 950\n  player-start.fluid-drag 2\n  player-start.swim-acceleration 4\n')
 obj('Project HUD',(0,0,0),f'  ui.asset "{UI}"\n  ui.name "game_ui"\n  ui.enabled true\n  scripts 1\n  script.0.id 1\n  script.0.asset "{SCRIPT}"\n  script.0.enabled true\n  script.0.properties "{{}}"\n')
 settings=f'JudasScene 3\nsettings\n  name "Production fluid / {"radial planet" if planet else "flat pool"}"\n  world-origin 0 0 0\n  sun-color 1 0.98 0.92\n  sun-direction -0.3 0.8 0.5\n  ambient 0.3 0.32 0.35\n  fluid-scale 13\n  fluid-update-rate-hz 20\n  fluid-hydrostatic-drag-rate 2\n  fidelity-policy none\n  next-id {idx+1}\nend\n\n'
 path=DEST/'Scenes'/('planet.judas' if planet else 'pool.judas');path.write_text(settings+'\n'.join(rows))
 return dict(scene=str(path.relative_to(ROOT)),particles=count,spacing=spacing,cup=cup,floating=floating,sinking=sinking,player=player,origin=origin,rotation=q,bodies=len(solids)+3+(1 if planet else 0))
(DEST/'demo-content.json').write_text(json.dumps({'pool':create(False),'planet':create(True)},indent=2)+'\n')
print((DEST/'demo-content.json').read_text())
