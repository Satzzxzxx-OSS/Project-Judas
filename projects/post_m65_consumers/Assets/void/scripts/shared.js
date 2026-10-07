// Cross-script registry and helpers shared by every behaviour in the scene VM.
import {time,physics} from 'judas';
import {input} from './collection_input.js';
import {SYSTEM} from './system_data.js';
import {add,sub,mul,length,norm} from './math.js';

export const G={game:null,ship:null,foot:null,bolts:null,enemies:new Map(),pendingDebris:new Map(),mode:'foot',auto:null};
export const V=a=>({x:a[0],y:a[1],z:a[2]});
export const PLANETS=SYSTEM.planets.map(p=>({...p,c:V(p.c)}));

/** Use the authoritative resolver at the requested point, including zones/precedence. */
export function gravityAt(p){return physics.gravity(p);}
export function nearestPlanet(p){
 let best=null;
 for(const pl of PLANETS){const d=sub(p,pl.c),l=length(d),alt=l-pl.r;if(!best||alt<best.alt)best={planet:pl,alt,up:norm(d),dist:l};}
 return best;
}
const NONE={mx:0,my:0,roll:0,ascend:false,descend:false,boost:false,fire:false,interact:false,camera:false,assist:false,pitchRate:0,yawRate:0};
let cache={t:-1,i:NONE};
/** Fixed-step intent (keyboard/pad or test autopilot), evaluated once per fixed step. */
export function intent(){
 if(!time.fixed)return NONE;
 if(cache.t===time.elapsed)return cache.i;
 const i=G.auto?{...NONE,...G.auto.intent()}:{mx:input.axis('move_x'),my:input.axis('move_y'),roll:input.axis('roll'),
  ascend:input.held('ascend'),descend:input.held('descend'),boost:input.held('boost'),fire:input.held('fire'),
  interact:input.pressed('interact'),camera:input.pressed('camera'),assist:input.pressed('assist'),pitchRate:0,yawRate:0};
 cache={t:time.elapsed,i};return i;
}
