// Physical wreckage from a destroyed raider; removes itself after a while.
import {G} from './shared.js';
export default class Debris {
 constructor({entity}){this.entity=entity;this.age=0;}
 fixedUpdate(dt){
  const init=G.pendingDebris.get(this.entity.id);
  if(init&&!init.applied){init.applied=true;this.entity.velocity=init.v;this.entity.angularVelocity=init.w;}
  this.age+=dt;if(this.age>9){G.pendingDebris.delete(this.entity.id);this.entity.destroy();}
 }
 destroy(){G.pendingDebris.delete(this.entity.id);}
}
