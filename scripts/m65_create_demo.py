#!/usr/bin/env python3
"""Original integration fixtures. Never writes in consumer or historical projects."""
from pathlib import Path
import json,struct,math,hashlib,shutil
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'projects/m65_integration';A=P/'Assets'
def asset(name,data=None,kind='script'):
 p=A/name;p.parent.mkdir(parents=True,exist_ok=True)
 if data is not None:p.write_text(data)
 key=hashlib.md5(('M65 original:'+name).encode()).hexdigest();Path(str(p)+'.judasmeta').write_text(f'JudasAssetMeta 1\nid "{key}"\ntype {kind}\nsource ""\n');return key
# Simple original two-leg, one-arm articulated rig: no third-party rider licensing.
names=['Root','LeftHip','LeftKnee','LeftFoot','RightHip','RightKnee','RightFoot','Shoulder','Elbow','Hand']
parents=[-1,0,1,2,0,4,5,0,7,8];local=[(0,2,0),(-.38,0,0),(0,-.9,0),(0,-.9,0),(.38,0,0),(0,-.9,0),(0,-.9,0),(.5,.55,0),(.55,0,0),(.5,0,0)]
world=[]
for i,p in enumerate(parents):world.append(tuple(local[i][k]+(world[p][k] if p>=0 else 0) for k in range(3)))
binary=bytearray();views=[];acc=[]
def accessor(rows,kind,component=5126):
 binary.extend(b'\0'*((-len(binary))%4));start=len(binary);n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[kind]
 for row in rows:binary.extend(struct.pack('<'+('f' if component==5126 else 'H')*n,*row))
 views.append({'buffer':0,'byteOffset':start,'byteLength':len(binary)-start});a={'bufferView':len(views)-1,'componentType':component,'count':len(rows),'type':kind}
 if kind=='VEC3':a.update(min=[min(r[k] for r in rows) for k in range(3)],max=[max(r[k] for r in rows) for k in range(3)])
 acc.append(a);return len(acc)-1
pos=[];normal=[];uv=[];joint=[];weight=[];ix=[]
for i,center in enumerate(world):
 half=(.42,.28,.18) if i==0 else (.1,.41,.1) if i in [1,2,4,5] else (.16,.07,.22) if i in [3,6] else (.24,.08,.09)
 shift=(0,-.41,0) if i in [1,2,4,5] else (.23,0,0) if i in [7,8] else (0,0,0)
 c=tuple(center[k]+shift[k] for k in range(3));corners=[tuple(c[k]+sign[k]*half[k] for k in range(3)) for sign in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
 for face,n in [((0,3,2,1),(0,0,-1)),((4,5,6,7),(0,0,1)),((0,4,7,3),(-1,0,0)),((1,2,6,5),(1,0,0)),((3,7,6,2),(0,1,0)),((0,1,5,4),(0,-1,0))]:
  base=len(pos)
  for k,j in enumerate(face):pos.append(corners[j]);normal.append(n);uv.append([(0,0),(1,0),(1,1),(0,1)][k]);joint.append([i,0,0,0]);weight.append([1,0,0,0])
  ix.extend([[base+j] for j in [0,1,2,0,2,3]])
attrs={'POSITION':accessor(pos,'VEC3'),'NORMAL':accessor(normal,'VEC3'),'TEXCOORD_0':accessor(uv,'VEC2'),'JOINTS_0':accessor(joint,'VEC4',5123),'WEIGHTS_0':accessor(weight,'VEC4')};indices=accessor(ix,'SCALAR',5123)
binds=accessor([[1,0,0,0,0,1,0,0,0,0,1,0,-w[0],-w[1],-w[2],1] for w in world],'MAT4');times=accessor([[v] for v in [0,.5,1,1.5,2]],'SCALAR')
anims=[]
for name,node,axis,angles in [('Lean',0,(0,0,1),[0,.12,0,-.12,0]),('Wave',7,(0,1,0),[0,.6,0,-.6,0])]:
 rotations=accessor([[axis[0]*math.sin(a/2),axis[1]*math.sin(a/2),axis[2]*math.sin(a/2),math.cos(a/2)] for a in angles],'VEC4')
 anims.append({'name':name,'samplers':[{'input':times,'output':rotations,'interpolation':'LINEAR'}],'channels':[{'sampler':0,'target':{'node':node,'path':'rotation'}}]})
nodes=[{'name':n,'translation':list(local[i]),'children':[j for j,p in enumerate(parents) if p==i]} for i,n in enumerate(names)]+[{'name':'Mesh','mesh':0,'skin':0}]
data={'asset':{'version':'2.0','generator':'Judas M65 original CC0 fixture'},'scene':0,'scenes':[{'nodes':[0,10]}],'nodes':nodes,'skins':[{'joints':list(range(10)),'inverseBindMatrices':binds,'skeleton':0}],'meshes':[{'primitives':[{'attributes':attrs,'indices':indices,'mode':4}]}],'animations':anims,'bufferViews':views,'accessors':acc,'buffers':[{'byteLength':len(binary)}]}
encoded=json.dumps(data,separators=(',',':')).encode();encoded+=b' '*((-len(encoded))%4);binary+=b'\0'*((-len(binary))%4);glb=struct.pack('<III',0x46546c67,2,28+len(encoded)+len(binary))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+struct.pack('<II',len(binary),0x004e4942)+binary
asset('models/rider.glb',kind='mesh');(A/'models/rider.glb').write_bytes(glb);(A/'models/LICENSE.txt').write_text('M65 original jointed rider geometry/rig/clips: Project Judas contributors. CC0 1.0. No third-party character content.\n')
asset('physical/slippery.judasphysmat','JudasPhysicalMaterial 1\nfriction 0.05\nrestitution 0.05\n','physicalMaterial');asset('physical/grippy.judasphysmat','JudasPhysicalMaterial 1\nfriction 0.9\nrestitution 0.05\n','physicalMaterial')
# Original Rooftop course is preserved apart from superseded checkpoint box workaround.
s=(ROOT/'docs/evidence/m65/consumer-course-before.js').read_text();s=s.replace('// Checkpoints are game-side volume tests: CharacterMotor capsules do not\n// generate trigger events, so sensor bodies cannot be used for this.','// M65 integration copy: ordinary sensors report overlap; this JS retains checkpoint rules.')
s=s.replace('uiUpdate(dt){',"uiUpdate(dt){if(input.pressed('lab')){scenes.load('Scenes/integration.judas');return;}");s=s.replace('this.doc=ui.get(\'hud\');','link.course=this;this.doc=ui.get(\'hud\');')
a=s.index('  // Checkpoints (forward only).');b=s.index('\n }\n\n openMenu',a)
s=s[:a]+''' }

 checkpoint(index){if(index<=this.state.checkpoint||index>=course.checkpoints.length)return;
  this.state.checkpoint=index;this.splits.push(this.state.elapsed);
  this.show(`CHECKPOINT - ${course.checkpoints[index].name}  ${clock(this.state.elapsed)}`,1.8);
  console.log(`checkpoint ${index} ${this.state.elapsed.toFixed(2)}`);
 }
 finish(){if(this.state.finished)return;this.state.finished=true;this.state.running=false;
  const best=session.get('rooftopBest'),t=this.state.elapsed,record=typeof best!=='number'||t<best;
  if(record)session.set('rooftopBest',t);this.openMenu('FINISHED',`Time ${clock(t)}${record?'  NEW BEST':''}\\nBest ${clock(record?t:best)}   Falls ${this.state.falls}`);
''' +s[b:]
# Idempotent regenerate from frozen copy stored by source hash stage? this script is generation once,
# a missing original manual section means already migrated; avoid rerunning without source restoration.
(A/'scripts/course.js').write_text(s)
sensor=asset('scripts/checkpoint.js',"""import {link} from './runner.js';
export const properties={index:{type:'number',default:0},finish:{type:'boolean',default:false}};
export default class {constructor({properties}){this.props=properties;}onTriggerEnter(e){if(e.other?.id!==link.runner?.entity.id)return;if(this.props.finish)link.course?.finish();else link.course?.checkpoint(this.props.index);}}
""")
course=json.loads((A/'scripts/course_data.js').read_text().split('export const course = ')[1].rstrip().rstrip(';'))
scene=(ROOT/'docs/evidence/m65/consumer-rooftops-before.judas').read_text()
for i,box in enumerate(course['checkpoints'][1:]+[course['finish']],1):
 lo,hi=box['min'],box['max'];center=[(a+b)/2 for a,b in zip(lo,hi)];half=[(b-a)/2 for a,b in zip(lo,hi)]
 scene+=f'\nobject {5000+i} "Sensor checkpoint {i}"\n  position '+ ' '.join(map(str,center))+'\n  rotation 1 0 0 0\n  scale 1 1 1\n  body static box\n  body.half-extents '+ ' '.join(map(str,half))+f'\n  body.radius .5\n  body.mass 1\n  body.terrain ""\n  body.friction .6\n  body.restitution .1\n  body.initial-velocity 0 0 0\n  body.pickable false\n  body.managed false\n  body.compound-count 0\n  body.sensor true\n  scripts 1\n  script.0.id 1\n  script.0.asset "{sensor}"\n  script.0.enabled true\n  script.0.properties '+json.dumps(json.dumps({'index':i,'finish':i==len(course['checkpoints'])}))+ '\nend\n'
scene=scene.replace('next-id 5000','next-id 6000');(P/'Scenes/rooftops.judas').write_text(scene)
print('M65 original rider and current parkour sensor integration generated')

asset("prefabs/posed.judasprefab",kind="prefab")

# Original checker image for ordinary primitive UV-instance authoring.
import zlib,binascii
def chunk(k,v):return struct.pack('>I',len(v))+k+v+struct.pack('>I',binascii.crc32(k+v)&0xffffffff)
raw=b''.join(b'\x00'+b''.join(bytes((210,210,210,255) if (x//8+y//8)%2 else (70,70,70,255)) for x in range(32)) for y in range(32))
(A/'textures').mkdir(exist_ok=True);(A/'textures/checker.png').write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',32,32,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))
asset('textures/checker.png',kind='texture')
