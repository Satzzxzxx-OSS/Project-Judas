// Travel gates and loading UI are GAME policy. Judas only reports residency.
import {scenes,input,world,physics,localization,saves} from 'judas';
import {add,mul,sub,length} from './math.js';
export default class Residency {
 constructor(player){this.player=player;this.manual=new Map();this.held=null;this.waiting=false;this.next=1;this.counter=0;}
 start(){this.manual.set('gallery-0',scenes.requestRegion('gallery-0'));}
 snapshot(){return {names:[...this.manual.keys()],held:this.held?.valid?saves.reference(this.held):null,next:this.next};}
 restore(state){for(const token of this.manual.values())scenes.releaseRegion(token);this.manual.clear();for(const name of state?.names||[])this.manual.set(name,scenes.requestRegion(name));this.held=state?.held?saves.resolve(state.held):null;this.next=state?.next||1;}
 update(){const pose=this.player.entity.transform;
  scenes.setInterest('range-travel',pose.position,{load:22,retain:34,priority:20});
  if(input.pressed('region_load')){const name='gallery-'+(this.next++%6);if(!this.manual.has(name))this.manual.set(name,scenes.requestRegion(name));}
  if(input.pressed('region_release')){for(const token of this.manual.values())scenes.releaseRegion(token);this.manual.clear();}
  if(input.pressed('liquid_load')&&!this.manual.has('liquid'))this.manual.set('liquid',scenes.requestRegion('liquid'));
  if(input.pressed('carry')){
   if(this.held){this.held=null;return;}
   const ray=world.viewRay,hit=physics.raycast(ray.origin,ray.direction,6,{ignored:[this.player.entity]});
   if(hit?.entity?.valid&&hit.entity.hasTag('physical')&&!hit.entity.hasTag('target')){scenes.adopt(hit.entity);this.held=hit.entity;}
  }
 }
 allowed(position){if(position.z>=-11.4)return true;
  const index=Math.max(0,Math.floor((-position.z-12)/24));
  return index<6&&scenes.regions.some(r=>r.id==='gallery-'+index&&r.state==='active');
 }
 constrain(velocity,dt){const p=this.player.entity.transform.position;const next=add(p,mul(velocity,Math.max(dt,.1)));this.waiting=!this.allowed(next);return this.waiting?{x:velocity.x,y:velocity.y,z:0}:velocity;}
 fixedUpdate(){if(!this.held?.valid){this.held=null;return;}
  const view=this.player.camera.pose(),target=add(view.eye,mul(view.direction,2.4)),body=this.held;
  const acceleration=sub(mul(sub(target,body.transform.position),22),mul(body.velocity,9));
  const force=mul(sub(acceleration,this.player.entity.character.gravity),body.mass);
  const magnitude=length(force);body.applyForce(magnitude>800?mul(force,800/magnitude):force);
 }
 uiUpdate(){if(++this.counter%10)return;const rows=scenes.regions,stats=scenes.streamingStats;
  const states=rows.map(r=>`${r.id}: ${localization.format('stream.'+r.state)}${r.pins.length?' ['+r.pins.join('; ')+']':''}${r.error?' ! '+r.error:''}`).join('\n');
  const text=localization.format('stream.status',{active:stats.active,pending:stats.pending,live:Math.round(stats.liveBytes/1024),retained:Math.round(stats.retainedBytes/1024),work:stats.integrationMs.toFixed(2),states});
  this.player.hud.doc.get('streaming').text=text;
  this.player.hud.doc.get('gate').visible=this.waiting;
  if(this.waiting)this.player.hud.doc.get('gate').text=localization.format('stream.wait');
 }
 destroy(){for(const token of this.manual.values())scenes.releaseRegion(token);scenes.removeInterest('range-travel');}
}
