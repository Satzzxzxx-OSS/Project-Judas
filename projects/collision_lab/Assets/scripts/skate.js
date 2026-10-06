import {input,physics,world} from 'judas';
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
