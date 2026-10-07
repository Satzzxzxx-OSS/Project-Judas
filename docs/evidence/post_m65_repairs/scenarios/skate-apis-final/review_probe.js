import {world,physics,console,time} from 'judas';
import {game} from '../skate/scripts/game.js';
const dist=(a,b)=>Math.hypot(a.x-b.x,a.y-b.y,a.z-b.z);
export default class {
 constructor(){this.n=0;this.state={};}
 uiUpdate(){++this.n;const s=game.skater,r=world.entity('12'),a=r?.animation;
  if(!s||!a?.info.ready)return;
  const left=a.jointTransform('mixamorig:LeftToeBase','model'),right=a.jointTransform('mixamorig:RightToeBase','model');
  if(left&&right){this.state.poseLift=s.view.lift;this.state.measuredLift=Math.max(0,Math.min(left.position.y,right.position.y)-.096);this.state.clip=a.info.clip;}
  if(this.n===180){const e=world.entity('1000001');e.setSocket(r,'mixamorig:LeftHand',{position:{x:.03,y:0,z:0}});this.socket=e;
   const ankle=a.jointTransform('mixamorig:LeftFoot','world'),hip=a.jointTransform('mixamorig:LeftUpLeg','world');
   this.target={x:ankle.position.x,y:ankle.position.y+.1,z:ankle.position.z};
   this.state.ikConfigured=a.limb('review-foot',{root:'mixamorig:LeftUpLeg',middle:'mixamorig:LeftLeg',end:'mixamorig:LeftFoot',target:this.target,pole:{x:hip.position.x,y:hip.position.y-.3,z:hip.position.z-.7},weight:1});
  }
  if(this.n===240){const ankle=a.jointTransform('mixamorig:LeftFoot','world');this.state.ikError=dist(ankle.position,this.target);
   const h=a.jointTransform('mixamorig:LeftHand','world',true);this.state.socketError=Math.abs(dist(this.socket.presentedTransform.position,h.position)-.03);
   a.removeLimb('review-foot');this.socket.clearSocket();
   const body=s.e,t=body.transform;this.joint=physics.createJoint(body,{type:'slider',bodyA:body,anchorB:{x:0,y:0,z:0},lower:-5,upper:5,limits:true});this.state.jointCreated=this.joint.valid;
  }
  if(this.n===260){this.joint.configure({anchorB:{x:0,y:0,z:0}});this.state.jointReanchored=this.joint.valid;this.joint.destroy();this.state.jointRetired=!this.joint.valid;
   this.material=s.e.physicalMaterial;s.e.setPhysicalMaterial('dad488c962a44aaba2adfba152ac50c2',{friction:.7,restitution:.05});this.state.materialRead=s.e.physicalMaterial;
   const root=s.e.transform.position;const hit=physics.raycast({x:root.x,y:root.y+1,z:root.z},{x:0,y:-1,z:0},2,{excludeLayers:['Ragdoll','Pickup']});this.state.hitMaterial=hit?.physicalMaterial??null;this.state.hitBody=hit?.entity?.id??null;
  }
  if(this.n===290)s.e.setPhysicalMaterial(null,{friction:this.material.friction,restitution:this.material.restitution});
  if(this.n%60===0)console.log('REVIEW '+JSON.stringify(this.state));
 }
}
