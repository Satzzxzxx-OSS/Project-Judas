// A generic off-centre impulse; units are metres and N s, not weapon policy.
export default class {
 /** @param {{entity: import('judas').Entity}} context */
 constructor({entity}){this.entity=entity;this.state={angular:false,rejectsInvalid:false,linear:false,unchanged:false};}
 start(){const p=this.entity.transform.position,before=this.entity.velocity;
  this.entity.applyImpulseAtPoint({x:0,y:0,z:-2},{x:p.x,y:p.y+1,z:p.z});
  this.state.angular=this.entity.angularVelocity.x<0;
  this.state.linear=Math.abs(this.entity.velocity.z-before.z+2/this.entity.mass)<1e-5;
  const after=this.entity.velocity.z;
  try{this.entity.applyImpulseAtPoint({x:0,y:0,z:1},{x:NaN,y:0,z:0});}catch{this.state.rejectsInvalid=true;}
  this.state.unchanged=this.entity.velocity.z===after;
 }
}
