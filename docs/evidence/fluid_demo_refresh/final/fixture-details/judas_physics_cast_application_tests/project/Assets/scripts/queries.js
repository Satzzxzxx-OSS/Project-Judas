
import Demo from './demo.js';import {world,physics} from 'judas';
export default class extends Demo {
 start(){super.start();const assert=(x,s)=>{if(!x)throw Error(s);};
 const v=(x,y,z)=>({x,y,z}),filter={excludeLayers:['Excluded'],excludedTags:['IgnoreQuery'],ignored:[world.entity('5')]};
 const before=JSON.stringify(world.entity('4').transform);
 let h=physics.raycast(v(-2,1.5,8),v(0,0,-2),100,filter);
 assert(h&&h.entity.id==='4'&&h.entity.valid&&Math.abs(h.distance-8.2)<.0001&&h.normal.z===1&&Math.abs(h.point.z+.2)<.0001,'ray data');
 assert(!physics.raycast(v(20,1.5,8),v(0,0,-1),100,filter),'miss');
 assert(!physics.raycast(v(2,1.5,8),v(0,0,-1),10,filter),'classification');
 assert(!physics.raycast(v(0,1.5,8),v(0,0,-1),8,filter),'ignored entity');
 assert(Math.abs(physics.sphereCast(v(-2,1.5,8),.4,v(0,0,-1),100,filter).distance-7.8)<.0001,'sphere');
 const pose={position:v(-2,1.5,8),rotation:{w:1,x:0,y:0,z:0}};
 assert(Math.abs(physics.capsuleCast(pose,.2,.4,v(0,0,-1),100,filter).distance-8)<.0001,'capsule');
 assert(Math.abs(physics.boxCast(pose,v(.3,.3,.3),v(0,0,-1),100,filter).distance-7.9)<.0001,'box');
 h=physics.raycast(v(-1,1.5,8),v(0,0,-1),100,filter);
 assert(h&&h.entity.id===this.spawned[0].id,'runtime prefab hit');
 const retained=h.entity;retained.destroy();assert(!retained.valid,'safe destroyed hit wrapper');
 assert(JSON.stringify(world.entity('4').transform)===before,'read only');
 this.spawn();this.state.checked=true;
 }
}