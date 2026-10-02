#!/usr/bin/env python3
"""Original M46 three-bone bendable bar; offline asset authoring, not a parser."""
import base64,json,math,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def create():
 binary=bytearray();views=[];accessors=[]
 def accessor(rows,kind,component=5126):
  while len(binary)%4:binary.append(0)
  offset=len(binary);n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[kind];fmt='f' if component==5126 else 'H'
  for row in rows:binary.extend(struct.pack('<'+fmt*n,*row))
  views.append({'buffer':0,'byteOffset':offset,'byteLength':len(binary)-offset});a={'bufferView':len(views)-1,'componentType':component,'count':len(rows),'type':kind}
  if kind=='VEC3':a.update(min=[min(r[k] for r in rows) for k in range(3)],max=[max(r[k] for r in rows) for k in range(3)])
  accessors.append(a);return len(accessors)-1
 positions=[];normals=[];uv=[];joints=[];weights=[];indices=[]
 for slab in range(6):
  y0=slab*.5;y1=y0+.5
  corners=[(-.35,y0,-.25),(.35,y0,-.25),(.35,y1,-.25),(-.35,y1,-.25),(-.35,y0,.25),(.35,y0,.25),(.35,y1,.25),(-.35,y1,.25)]
  for face,normal in [((0,3,2,1),(0,0,-1)),((4,5,6,7),(0,0,1)),((0,4,7,3),(-1,0,0)),((1,2,6,5),(1,0,0)),((3,7,6,2),(0,1,0)),((0,1,5,4),(0,-1,0))]:
   base=len(positions)
   for k,index in enumerate(face):
    point=corners[index];positions.append(point);normals.append(normal);uv.append([(0,0),(1,0),(1,1),(0,1)][k]);y=point[1];joints.append([0,1,2,0]);weights.append([max(0,1-y),max(0,1-abs(y-1)),min(1,max(0,y-1)),0])
   indices.extend([[base+i] for i in [0,1,2,0,2,3]])
 attributes={'POSITION':accessor(positions,'VEC3'),'NORMAL':accessor(normals,'VEC3'),'TEXCOORD_0':accessor(uv,'VEC2'),'JOINTS_0':accessor(joints,'VEC4',5123),'WEIGHTS_0':accessor(weights,'VEC4')}
 ix=accessor(indices,'SCALAR',5123);matrices=[]
 for y in [0,-1,-2]:matrices.append([1,0,0,0,0,1,0,0,0,0,1,0,0,y,0,1])
 bind=accessor(matrices,'MAT4');times=accessor([[t] for t in [0,.5,1,1.5,2]],'SCALAR')
 def rotations(angles):return [[0,0,math.sin(a/2),math.cos(a/2)] for a in angles]
 animations=[{'name':'Wave','samplers':[{'input':times,'output':accessor(rotations([0,.7,0,-.7,0]),'VEC4'),'interpolation':'LINEAR'},{'input':times,'output':accessor(rotations([0,-.5,0,.5,0]),'VEC4'),'interpolation':'LINEAR'}],'channels':[{'sampler':0,'target':{'node':1,'path':'rotation'}},{'sampler':1,'target':{'node':2,'path':'rotation'}}]},
 {'name':'Stretch','samplers':[{'input':times,'output':accessor([[0,1+v,0] for v in [0,.25,.5,.25,0]],'VEC3'),'interpolation':'LINEAR'},{'input':times,'output':accessor([[1+v,1-v,1] for v in [0,.1,.2,.1,0]],'VEC3'),'interpolation':'LINEAR'}],'channels':[{'sampler':0,'target':{'node':2,'path':'translation'}},{'sampler':1,'target':{'node':0,'path':'scale'}}]}]
 data={'asset':{'version':'2.0','generator':'Judas M46 original asset authoring'},'scene':0,'scenes':[{'nodes':[0,3]}],'nodes':[{'name':'Root','children':[1]},{'name':'Elbow','translation':[0,1,0],'children':[2]},{'name':'Tip','translation':[0,1,0]},{'name':'Mesh','mesh':0,'skin':0}], 'skins':[{'joints':[0,1,2],'inverseBindMatrices':bind,'skeleton':0}],'meshes':[{'primitives':[{'attributes':attributes,'indices':ix,'mode':4}]}],'animations':animations,'bufferViews':views,'accessors':accessors,'buffers':[{'byteLength':len(binary)}]}
 encoded=json.dumps(data,separators=(',',':')).encode();encoded+=b' '*((-len(encoded))%4);binary+=b'\0'*((-len(binary))%4)
 glb=struct.pack('<III',0x46546c67,2,12+8+len(encoded)+8+len(binary))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+struct.pack('<II',len(binary),0x004e4942)+binary
 target=ROOT/'projects/animation_demo/Assets/models';target.mkdir(parents=True,exist_ok=True);(target/'bend_bar.glb').write_bytes(glb)
 data['buffers'][0]['uri']='data:application/octet-stream;base64,'+base64.b64encode(binary).decode();fixture=ROOT/'tests/fixtures/m46';fixture.mkdir(parents=True,exist_ok=True);(fixture/'bend_bar.gltf').write_text(json.dumps(data,indent=2)+'\n')
 (target/'LICENSE.txt').write_text('Original Project Judas M46 bend-bar geometry, rig and clips. CC0 1.0 public-domain dedication. Author: Project Judas contributors. No third-party artwork.\n')
if __name__=='__main__':create()
