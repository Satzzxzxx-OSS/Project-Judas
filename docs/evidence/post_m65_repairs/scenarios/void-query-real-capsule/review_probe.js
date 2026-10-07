import {physics,world,console} from 'judas';
import {G,gravityAt} from '../void/scripts/shared.js';
import {SYSTEM} from '../void/scripts/system_data.js';
export default class {constructor(){this.n=0;this.state={};}uiUpdate(){++this.n;if(this.n===180){
 const e=G.foot.entity,p=e.transform.position,up=e.character.state.up,origin={x:p.x+up.x*1.2,y:p.y+up.y*1.2,z:p.z+up.z*1.2};
 const hit=physics.raycast(origin,{x:-up.x,y:-up.y,z:-up.z},6,{ignored:[world.entity(SYSTEM.ids.ship)]});
 this.state.motorHit=hit?.entity?.id===e.id;this.state.shape=hit?.shape;this.state.hit=hit?.entity?.id??null;this.state.distance=hit?.distance??null;this.state.gravity=gravityAt(p);this.state.motorGravity=e.character.state.gravity;
 const suit=G.foot.state.suit;G.bolts.fire('enemy',origin,{x:-up.x*80,y:-up.y*80,z:-up.z*80},world.entity(SYSTEM.ids.ship),7);this.state.suitBefore=suit;
 }if(this.n===190)this.state.suitAfter=G.foot.state.suit;if(this.n%60===0)console.log('REVIEW '+JSON.stringify(this.state));}}
