// Physical damping/control building block, not a teleport or engine pickup API.
import {input} from 'judas';
export default class {
 /** @param {{entity: import("judas").Entity}} context */
 constructor({entity}) {this.entity=entity;this.state={mass:0,capture:false};}
 start(){input.pointerCapture=true;this.state.capture=input.pointerCapture;this.state.mass=this.entity.mass;}
 fixedUpdate(){const v=this.entity.velocity;
  // Project chooses damping; engine supplies real mass and world inertia.
  this.entity.applyForce({x:-v.x*this.entity.mass,y:-v.y*this.entity.mass,z:-v.z*this.entity.mass});
  const omega=this.entity.angularVelocity,m=this.entity.inertiaWorld;
  this.entity.applyTorque({x:-(m.x.x*omega.x+m.y.x*omega.y+m.z.x*omega.z),y:-(m.x.y*omega.x+m.y.y*omega.y+m.z.y*omega.z),z:-(m.x.z*omega.x+m.y.z*omega.y+m.z.z*omega.z)});
 }
 destroy(){input.pointerCapture=false;}
}
