
import Demo from './demo.js';import {world,physics} from 'judas';
export default class extends Demo {
 start(){super.start();const assert=(x,s)=>{if(!x)throw Error(s);};
 const joint=physics.joint(world.entity('31'));assert(joint&&joint.valid&&joint.state.active,'live joint');
 joint.setEnabled(false);assert(!joint.state.active,'disabled joint');joint.setEnabled(true);
 joint.setLimits(-.7,.7);joint.setMotor(-.5,10);joint.setSpring(0,2,1);
 let rejected=false;try{joint.setMotor(1,-1);}catch(e){rejected=true;}assert(rejected,'invalid motor rejected');
 const root=world.spawnPrefab('44444444444444444444444444444402',{position:{x:10,y:4,z:0},rotation:{w:1,x:0,y:0,z:0},scale:{x:1,y:1,z:1}});
 let owned=null;for(const child of root.children){const h=physics.joint(child);if(h)owned=h;}
 assert(owned&&owned.valid,'spawned prefab joint');root.destroy();assert(!owned.valid,'stale joint fails safely');
 this.state.checked=true;
 }
}