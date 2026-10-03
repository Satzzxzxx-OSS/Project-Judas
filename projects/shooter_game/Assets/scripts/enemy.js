// GAME behaviour. Navigation proposes velocity; the ordinary motor resolves it.
import {world,ui} from 'judas';
import {round,navigators} from './round.js';
import {add,mul,sub,dot,length,tangent} from './math.js';
export default class {
 constructor({entity}){this.entity=entity;this.state={health:2,defeated:false,repaths:0,link:false};this.next=0;}
 start(){this.motor=this.entity.character;this.agent=this.entity.navigation;this.proxy=this.entity.children[0];this.player=world.entity('10');navigators.set(this.proxy.id,this);}
 hit(){if(this.state.defeated)return false;if(--this.state.health<=0){this.state.defeated=true;this.agent.clear();this.agent.enabled=false;this.proxy.setColliderEnabled(false);this.motor.enabled=false;round.score+=250;round.hits++;round.message='Navigator defeated +250';}else {round.message='Navigator hit - one more shot';round.hits++;}return true;}
 fixedUpdate(dt){if(this.state.defeated||!this.player?.valid)return;
  // Authoritative root drives its separate query sensor. Presentation poses never feed physics.
  this.proxy.transform=this.entity.transform;
  this.next-=dt;const pose=this.entity.transform,up=this.motor.up;
  if(this.next<=0){this.next=.7;this.agent.setDestination(sub(this.player.transform.position,mul(this.player.character.up,.9)));this.state.repaths++;}
  const nav=this.agent.state;this.state.link=nav.onLink;
  // This project's link means a slow hover bridge; engine link semantics remain generic.
  if(nav.onLink){const endpoint=add(nav.linkEnd,mul(up,.9)),delta=sub(endpoint,pose.position);this.motor.configure({gravityScale:0});if(length(delta)<.35){this.agent.completeLink();this.motor.configure({gravityScale:1});}else this.motor.velocity=mul(delta,2.8/Math.max(.1,length(delta)));return;}
  const distance=length(sub(this.player.transform.position,pose.position));
  const desired=distance<1.5?{x:0,y:0,z:0}:nav.steering;
  this.motor.velocity=add(desired,mul(up,dot(this.motor.velocity,up)));
 }
 update(){if(this.state.defeated){const t=this.entity.transform;t.scale={x:.32,y:.12,z:.32};this.entity.transform=t;}}
 destroy(){navigators.delete(this.proxy?.id);}
}
