#!/usr/bin/env python3
"""Original current fracture content. Geometry is baked; gameplay stays project JS.
The M62 format helpers are loaded as definitions only (never run its generator).
"""
from pathlib import Path
import ast,json,shutil,subprocess,sys,math
root=Path(__file__).resolve().parents[1];out=root/'projects/fracture_lab';baker=Path(sys.argv[1] if len(sys.argv)>1 else root/'build/judas_deformable_bake')
for d in ('Assets/deformables','Assets/materials','Assets/scripts','Assets/ui','Assets/fonts','Assets/prefabs','Assets/worlds','Assets/audio','Scenes'):(out/d).mkdir(parents=True,exist_ok=True)
ns=globals();tree=ast.parse((root/'tools/CreateDeformableLab.py').read_text());exec(compile(ast.Module([n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name in ('v','obj','render','body','script','deform','settings')],type_ignores=[]),'<format helpers>','exec'),ns)
ids={name:f'6363636363636363636363636363{n:04x}' for n,name in enumerate(('panel','beam','frame','controller','lab','ui','prefab','world','outer','inner','beep','barrier','agent','nav','events'),1)}
def meta(path,key,kind):(out/(path+'.judasmeta')).write_text(f'JudasAssetMeta 1\nid "{ids[key]}"\ntype {kind}\nsource ""\n')
for name,mode,cells,size,material in [('panel','fracture-rigid',[3,3,1],[3,3,.3],[12000,9000,1e-8]),('beam','fracture',[3,1,1],[3,.6,.6],[1500,1500,1e-6]),('frame','fracture-rigid',[3,2,1],[6,2,.4],[4500,4500,1e-8]),('barrier','fracture-rigid',[3,2,1],[3,3,.4],[2500,2500,1e-8])]:
 p=f'Assets/deformables/{name}.judasdeform';subprocess.run([str(baker),mode,str(out/p),*map(str,cells+size+material)],check=True);meta(p,name,'deformable')
# Two normal M57 material slots: visibly distinct rough interior.
for name,col in [('outer',(1,1,1)),('inner',(.65,.3,.12))]:
 mat=(root/'projects/deformable_lab/Assets/materials/fabric.judasmat').read_text().replace('base 1 1 1 1','base '+v((*col,1))).replace('doubleSided 1','doubleSided 0')
 (out/f'Assets/materials/{name}.judasmat').write_text(mat);meta(f'Assets/materials/{name}.judasmat',name,'material')
for f in ('DejaVuSans.ttf','DejaVuSans.ttf.judasmeta','LICENSE.txt'):shutil.copyfile(root/'projects/deformable_lab/Assets/fonts'/f,out/'Assets/fonts'/f)
for f in ('controller.js','math.js'):shutil.copyfile(root/'projects/deformable_lab/Assets/scripts'/f,out/'Assets/scripts'/f)
meta('Assets/scripts/controller.js','controller','script');(out/'Assets/scripts/math.js.judasmeta').write_text('JudasAssetMeta 1\nid "63636363636363636363636363639999"\ntype script\nsource ""\n')
for f in ('lab','agent','events'):meta(f'Assets/scripts/{f if f!='events' else 'fracture-events'}.js',f,'script')
meta('Assets/ui/lab.judasui','ui','ui');meta('Assets/prefabs/panel.judasprefab','prefab','prefab');meta('Assets/worlds/lab.judasworld','world','world')
ui=(root/'projects/deformable_lab/Assets/ui/lab.judasui').read_text().replace('DEFORMABLE LAB','FRACTURE LAB').replace('Cloth and tetrahedra: normal fixed-step physics','Solved traction | irreversible topology | real physical pieces')
old='WASD + mouse | G/T: carry/throw | V: air | C: cape motion | B: load / H: bend-compress-twist | R: release pin | P: spawn | F3: region | 1/2: flat/radial | F5/F8: save/load | Esc'
ui=ui.replace('CURTAIN | CAPE | ELASTIC | YIELDING | MIXED CONTACT','PHYSICAL CELLS | PLASTIC BEAM | SUPPORTED FRAME').replace('Aim + hold mouse: push fabric | G: pick up / drop a rigid prop','Aim and click once: physical impulse | G: carry/drop rubble').replace('Slot A / B','Slot: fracture-current')
ui=ui.replace(old,'WASD/mouse | click: impulse | G/T: carry/throw | B: low/high/off beam load | R: release right support | P: prefab | C: remove aimed fragment | F3: annex / H: adopt | 1/2/3: flat/radial/game | F5/F8: save/load | F9: reload | Esc')
(out/'Assets/ui/lab.judasui').write_text(ui)
proj=(root/'projects/deformable_lab/deformable_lab.judasproj').read_text().replace('Deformable Lab','Fracture Lab').replace('deformable-lab','fracture-lab').replace('"'+ '6262626262626262626262626262'+'000c"','"'+ids['world']+'"')
# Retain logical input conventions; add a third registered scene action.
raw=json.loads(proj.split('input-map ',1)[1].split('\n')[0]);head,entries=raw.split(' ',2)[0:2],None
raw=raw.replace('1 29 ','1 30 ',1) if raw.startswith('1 29 ') else '1 '+str(int(raw.split()[1])+1)+' '+raw.split(' ',2)[2]
raw=raw.replace('"elastic_mode"','"adopt"')
raw+=' "game" 0 1 "key:3" 1 0 '
proj=proj.split('input-map ',1)[0]+'input-map '+json.dumps(raw)+'\nworld-manifest "'+ids['world']+'"\n'
(out/'fracture_lab.judasproj').write_text(proj)
# helper puts two explicit material bindings on fractured owners, not copies.
def specimen(i,name,p,asset,density,attach=[],plastic=False,q=(1,0,0,0)):
 s=obj(i,name,p,'  tags 1\n'+script('events',{'obstacle':'550' if asset=='barrier' else ''})+render((1,1,.2),(.4,.7,.85))+deform(asset,density=density,shear=.0008 if plastic else 1e-6,volume=1e-6,attachments=attach,plastic=plastic),q)
 s=s.replace(json.dumps('1 "62626262626262626262626262629997" 0'),json.dumps('2 "'+ids['outer']+'" 0 "'+ids['inner']+'" 0'))
 return s
for radial in (False,True):
 rows=[]
 rows.append(obj(1,'Gravity',(0,-30,0) if radial else (0,0,0),'  gravity radial 9.81\n  gravity.region sphere 150\n' if radial else '  gravity uniform 9.81\n  gravity.region sphere 200\n'))
 if radial:rows.append(obj(2,'Planet',(0,-30,0),'  render sphere\n  render.radius 30\n  render.color .18 .28 .23\n  body static sphere\n  body.radius 30\n'))
 rows.append(obj(3,'Deck',(8,-.3,0),render((23,.3,12),(.18,.23,.29))+body((23,.3,12))))
 rows.append(obj(10,'Visitor',(0,1.1,8),'  motor.enabled true\n  motor.radius .3\n  motor.halfHeight .6\n'+script('controller')+f'  ui.asset "{ids["ui"]}"\n  ui.name "lab"\n  ui.enabled true\n'))
 rows.append(obj(4,'Annex walkway',(34,-.3,0),render((3.5,.3,2),(.18,.23,.29))+body((3.5,.3,2))))
 rows.append(obj(11,'Controls',(0,0,0),script('lab',{'radial':radial,'game':False})))
 rows.append(obj(12,'View seed',(0,1.1,8),'  player-start first-person\n'))
 rows.append(specimen(100,'A - impact panel',(0,2,0),'panel',40,[('bottom',0,0,'',(0,0,0))]))
 rows.append(specimen(200,'B - yielding beam',(6,2,0),'beam',5,[('left',0,0,'',(0,0,0))],True))
 rows.append(specimen(300,'C - alternate load paths',(14,3,0),'frame',30,[('left',0,0,'',(0,0,0)),('right',0,0,'',(0,0,0))]))
 for i,x in enumerate((0,6,14,23)):
  rows.append(obj(600+i,'Exhibit plinth',(x,.2,-2),render((1.5,.2,.3),(.65,.42,.2))+body((1.5,.2,.3))))
 rows.append(obj(400,'Pickup projectile',(2,.5,3),render((.25,.25,.25),(.9,.45,.15))+body((.25,.25,.25),True,5)+'  tags 1\n'))
 oblique=obj(500,'Oblique field',(23,2,0),'  gravity uniform 5\n  gravity.region box 4 6 4\n',(math.sqrt(.5),0,0,math.sqrt(.5)));rows.insert(0,oblique)
 rows.append(specimen(501,'D - oblique assembly',(23,2,0),'panel',20,[('left',0,0,'',(0,0,0))],q=(math.cos(.3),0,0,math.sin(.3))))
 rows.append(obj(700,'Secondary camera',(7,9,12),'  render-camera \n  camera.enabled true\n  camera.width 256\n  camera.height 144\n  camera.cadence 2\n  camera.fov 65\n  camera.near .1\n  camera.far 100\n',(math.cos(-.23),math.sin(-.23),0,0)))
 rows.append(obj(701,'Secondary view',(5,2,5),render((1.5,.9,.05),(1,1,1))+'  render.texture-camera 700\n'))
 (out/'Scenes'/('radial.judas' if radial else 'flat.judas')).write_text(settings('Fracture Lab / '+('radial' if radial else 'uniform'))+'\n'.join(rows))
(out/'Scenes/annex.judas').write_text(settings('Fracture retention annex',4)+specimen(1,'Retained fracture family',(42,3,0),'panel',10)+obj(2,'Annex floor',(42,-.3,0),render((5,.3,5),(.3,.3,.4))+body((5,.3,5))))
(out/'Assets/worlds/lab.judasworld').write_text('JudasWorld 1\nbudget 2 8 2 8388608 8388608 8388608 2097152\nregion "annex" "Scenes/annex.judas" 0 0 0 0 0 0 1 47 6 6 1 "snapshot" 262144 ""\n')
(out/'Assets/prefabs/panel.judasprefab').write_text(settings('Fracture prefab',2)+specimen(1,'Independent physical family',(0,0,0),'panel',10))
print(out)
# Bounded ordinary game exhibit: an independently baked corridor floor and one
# obstacle. The obstacle reflects the barrier; fractured floor rebaking is absent.
rows=[obj(1,'Uniform field',(0,0,0),'  gravity uniform 9.81\n  gravity.region sphere 100\n'),obj(3,'Corridor floor',(0,-.3,0),render((3,.3,12),(.2,.25,.3))+body((3,.3,12))),obj(10,'Visitor',(0,1.1,7),'  motor.enabled true\n  motor.radius .3\n  motor.halfHeight .6\n'+script('controller')+f'  ui.asset "{ids["ui"]}"\n  ui.name "lab"\n  ui.enabled true\n'),obj(11,'Range controls',(0,0,0),script('lab',{'game':True,'shotImpulse':120})),obj(12,'View seed',(0,1.1,7),'  player-start first-person\n'),specimen(100,'Physical breakable barrier',(0,1.5,0),'barrier',15,[('left',0,0,'',(0,0,0)),('right',0,0,'',(0,0,0))])]
for i,x in enumerate((-2,2)):rows.append(obj(20+i,'Corridor wall',(x,1.5,0),render((.5,1.5,12),(.25,.3,.35))+body((.5,1.5,12))))
rows.append(obj(550,'Barrier navigation obstacle',(0,1.5,0),'  nav.obstacle.enabled "1"\n  nav.obstacle.cylinder "0"\n  nav.obstacle.halfExtents "1.5 1.5 .25"\n  nav.obstacle.height "3"\n  nav.obstacle.radius "1.5"\n  nav.obstacle.updateDistance ".1"\n'))
rows.append(obj(551,'Corridor navigation',(0,0,0),'  nav.surface.enabled "1"\n  nav.surface.asset ""\n  nav.surface.cellHeight ".1"\n  nav.surface.cellSize ".15"\n  nav.surface.halfExtents "3 4 12"\n  nav.surface.includeDynamic "0"\n  nav.surface.minRegion "2"\n  nav.surface.profile "0"\n  nav.surface.simplification "1.3"\n  nav.surface.sources "18446744073709551615"\n  nav.surface.tileSize "32"\n'))
rows.append(obj(600,'JS navigating courier',(0,1.1,-6),render((.3,.8,.3),(.8,.3,.15))+'  motor.enabled true\n  motor.radius .3\n  motor.halfHeight .6\n  nav.agent.enabled "1"\n  nav.agent.avoidance "1"\n  nav.agent.profile "0"\n  nav.agent.areas "18446744073709551615"\n  nav.agent.speed "2"\n  nav.agent.arrival ".4"\n  nav.agent.cornerDistance ".3"\n  nav.agent.repathSeconds ".5"\n  nav.agent.costs ""\n'+script('agent')))
# Original short tone: non-game-specific audio obstruction uses normal rays.
import wave,struct
with wave.open(str(out/'Assets/audio/beacon.wav'),'wb') as w:
 w.setnchannels(1);w.setsampwidth(2);w.setframerate(22050);w.writeframes(b''.join(struct.pack('<h',int(6000*math.sin(2*math.pi*440*i/22050))) for i in range(22050)))
meta('Assets/audio/beacon.wav','beep','audio')
rows.append(obj(610,'Occluded beacon',(0,1,-4),'  audio-emitter\n  audio.asset "'+ids['beep']+'"\n  audio.enabled true\n  audio.play-on-start true\n  audio.loop true\n  audio.spatial true\n  audio.volume .2\n  audio.pitch 1\n  audio.attenuation inverse\n  audio.occlusion true\n  audio.reference-distance 1\n  audio.maximum-distance 30\n  audio.rolloff 1\n'))
rows.append(obj(611,'Active-view listener',(0,1.8,7),'  audio-listener\n  listener.enabled true\n  listener.follow-view true\n'))
(out/'Scenes/game.judas').write_text(settings('Fracture Range')+'\n'.join(rows))
# Reusable imported volumetric partition: an L-shaped three-cell source, not
# a special runtime geometry branch. Explicit pairs keep visual seams distinct.
source=['JudasFractureSource 1','settings 1 5000 5000 0 1e-8 256'];nodes=[];parts=[]
for p,(x,y,z) in enumerate(((0,0,0),(1,0,0),(0,1,0))):
 center=(x,y,z);local=[(x+(.5 if k&1 else -.5),y+(.5 if k&2 else -.5),z+(.5 if k&4 else -.5)) for k in range(8)];nodes.extend(local)
 source.extend('node '+v(n) for n in local)
 pattern=((0,1,3,7),(0,3,2,7),(0,2,6,7),(0,6,4,7),(0,4,5,7),(0,5,1,7))
 source.extend('tet '+v(tuple(p*8+n for n in t)) for t in pattern)
 parts.append('part '+json.dumps('import-'+str(p))+' '+v(center)+' .5 .5 .5 8 '+v(range(p*8,(p+1)*8))+' 6 '+v(range(p*6,(p+1)*6)))
source+=parts
for b,(a,c,axis) in enumerate(((0,1,0),(0,2,1))):
 normal=[0,0,0];normal[axis]=1;centroid=[0,0,0];centroid[axis]=.5;pairs=[(i,j) for i in range(a*8,(a+1)*8) for j in range(c*8,(c+1)*8) if nodes[i]==nodes[j]]
 source.append('bond '+json.dumps('import-bond-'+str(b))+f' {a} {c} '+v(normal)+' '+v(centroid)+' 1 '+str(len(pairs))+' '+v([n for pair in pairs for n in pair]))
source+=['group "left" 4 0 2 4 6','end'];(out/'Assets/deformables/L-partition.source').write_text('\n'.join(source)+'\n')
subprocess.run([str(baker),'partition',str(out/'Assets/deformables/imported-L.judasdeform'),str(out/'Assets/deformables/L-partition.source')],check=True)
(out/'Assets/deformables/imported-L.judasdeform.judasmeta').write_text('JudasAssetMeta 1\nid "63636363636363636363636363639995"\ntype deformable\nsource ""\n')
# Ordinary tracked source is kept in authoring assets; cook is self-contained.
# The explicit source is authoring input, not a registered runtime resource.
(out/'Assets/deformables/L-partition.source.judasmeta').unlink(missing_ok=True)
ids['imported']='63636363636363636363636363639995'
for f in ('flat','radial'):
 p=out/f'Scenes/{f}.judas';p.write_text(p.read_text()+specimen(450,'Imported irregular assembly',(19,2,3),'imported',10,[('left',0,0,'',(0,0,0))]))
