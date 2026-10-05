import {physics,world} from 'judas';
import {round,hit,navigators} from './round.js';
import {add,mul,sub,norm} from './math.js';
export default class {
 constructor(player,impulse){this.player=player;this.impulse=impulse;this.cooldown=0;this.impact=world.entity('20');this.flash=world.entity('21');this.report={hit:null,scored:false,point:null};}
 tick(dt){this.cooldown=Math.max(0,this.cooldown-dt);}
 fire(){if(this.cooldown>0)return false;this.cooldown=.18;round.shots++;
  const view=this.player.camera.pose(),filter={ignored:[this.player.entity],includeSensors:true};
  const sight=physics.raycast(view.position,view.direction,100,filter);
  const intended=sight?sight.point:add(view.position,mul(view.direction,100));
  // Reticle selects intent; eye/player ray then checks intervening geometry.
  const direction=norm(sub(intended,view.eye));
  const result=physics.raycast(view.eye,direction,100,filter);let scored=false;
  if(result?.entity?.valid){const e=result.entity;
   if(e.hasTag('physical'))e.applyImpulseAtPoint(mul(direction,this.impulse),result.point);
   if(e.hasTag('target'))scored=hit(e);
   if(e.hasTag('navigator'))scored=navigators.get(e.id)?.hit()||false;
   this.impact.transform={position:result.point};this.impact.burst(18);this.impact.playAudioOneShot();}
  this.flash.transform={position:add(view.eye,mul(direction,.45)),rotation:view.rotation};this.flash.burst(4);this.player.entity.playAudioOneShot();
  if(!scored){round.message=result?'Impact - plate cooling down or non-scoring prop.':'Miss - aim at a coloured plate.';}
  this.report={hit:result?.entity?.id||null,scored,point:result?.point||null};return true;
 }
}
