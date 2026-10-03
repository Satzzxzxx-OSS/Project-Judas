// Visual follow only. Movement/forces still belong in fixedUpdate.
import {world,time} from 'judas';
export default class {
 /** @param {{entity: import('judas').Entity}} context */
 constructor({entity}){this.entity=entity;this.state={calls:0,synchronized:false,frameMode:false};}
 /** @param {number} dt @param {number} alpha */
 presentationUpdate(dt,alpha){
  const pose=this.entity.presentedTransform;
  // The offset rotates with the entity; it does not assume universal world-up.
  // A real project can compose a local camera offset/look quaternion here.
  world.setView(pose,70);
  this.state.calls++;
  this.state.synchronized=alpha>=0&&alpha<=1&&Number.isFinite(pose.position.x);
  this.state.frameMode=!time.fixed;
 }
 destroy(){world.clearView();}
}
