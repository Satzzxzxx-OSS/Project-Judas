import {input,world,console} from 'judas';
import {vec} from './helpers.js';
export const properties={keyTag:{type:'string',default:'key'},action:{type:'string',default:'interact'},travel:{type:'number',default:3}};
export default class LockedDoor {
 constructor({entity,properties}){this.entity=entity;this.properties=properties;this.state={open:false,height:0};}
 start(){this.entity.transform={position:vec(0,1.5+this.state.height,-3)};}
 update(){if(input.pressed(this.properties.action)){
   const keys=world.queryTags([this.properties.keyTag]);
   const unlocked=keys.some(key=>key.scriptState(1)?.collected);
   if(!unlocked){console.log('Door locked. Press K to collect the yellow key, then G.');return;}
   this.state.open=!this.state.open;this.entity.playAudio();this.entity.burst(24);
   console.log('Door',this.state.open?'opened':'closed');
 }}
 fixedUpdate(dt){const target=this.state.open?this.properties.travel:0;
   this.state.height+=Math.max(-2*dt,Math.min(2*dt,target-this.state.height));
   this.entity.transform={position:vec(0,1.5+this.state.height,-3)};
 }
}
