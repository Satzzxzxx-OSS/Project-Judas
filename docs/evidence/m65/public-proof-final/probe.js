import {world,session,console} from 'judas';
import {add,rotate,axis,length,sub,norm} from './math.js';
function check(ok,name){if(!ok)throw new Error(name);console.log('PASS '+name);}
export default class {
 constructor(){this.n=0;this.targets=[];}
 start(){this.motor=world.entity('10');this.motor.transform={position:{x:-4,y:1,z:2}};}
 fixedUpdate(){this.n++;
  if(this.n===3){const a=world.entity('101');const q=axis(norm({x:1,y:2,z:3}),.65);a.transform={rotation:q};this.a=a;
   for(const [side,x] of [['Left',.1],['Right',-.1]]){const hip=a.animation.jointTransform(side+'Hip','world');const target=add(hip.position,rotate(q,{x,y:-1.5,z:.1}));a.animation.limb(side,{target,pole:add(hip.position,rotate(q,{x:0,y:0,z:2}))});this.targets.push([side,target]);}
  }
  if(this.n===4){for(const [side,target] of this.targets)check(length(sub(this.a.animation.jointTransform(side+'Foot','world').position,target))<.0001,'arbitrarily oriented '+side+' limb resolves world target');}
  if(this.n===10){check(session.get('m65.callback')==='destroy','normal motor/sensor callback ran');check(!this.motor.valid,'motor destroyed during callback retires safe handle');check(!world.entity('10').valid,'destroyed motor lookup does not alias another entity');check(session.get('m65.exit')===1,'destroying overlapped motor produces one exit');}
 }
}