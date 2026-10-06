import Lab from './lab.js';
import {world,saves,input,ui,session} from 'judas';
// Project policy only. Engine participants save water, poses, bodies and audio.
export default class extends Lab {
 constructor(context){super(context);this.state={ticks:0,slot:'slot-a'};this.request=0;this.message='';}
 start(){this.initialize();}
 restore(){this.initialize();} // Reacquire UI; do not recreate or restart physical state.
 initialize(){super.start();this.request=saves.refresh();this.hud.get('controls').text='F5 save | F8 load | F6 slot | B ragdoll/return | M crossfade | K seek music | Esc menu | V waves | G/P cup';input.pointerCapture=true;}
 fixedUpdate(dt){super.fixedUpdate(dt);this.state.ticks++;session.set('labTicks',this.state.ticks);}
 uiUpdate(){super.uiUpdate();this.hud.get('title').text='SAVE LAB / water, articulation, music';
  if(input.pressed('save_slot'))this.state.slot=this.state.slot==='slot-a'?'slot-b':'slot-a';
  if(input.pressed('save_game'))this.command('save');if(input.pressed('load_game'))this.command('load');
  if(input.pressed('ragdoll')){const r=world.entity('6100').ragdoll;if(r.active)r.leave(.5);else r.enter();}
  if(input.pressed('blend'))world.entity('6101').animation.crossFade('Stretch',1);
  if(input.pressed('seek_music'))world.entity('6200').seekAudio(4);
  if(this.request){const result=saves.status(this.request);if(result){this.message=result.state+(result.error?' — '+result.error:'');if(['completed','failed','cancelled'].includes(result.state))this.request=0;}}
  this.hud.get('progress').text+=' | '+this.state.slot+' | '+this.message+' | ticks '+this.state.ticks;
 }
 command(operation){if(this.request)return;try{if((operation==='delete'||operation==='save'&&saves.exists(this.state.slot))&&this.confirm!==operation){this.confirm=operation;this.message='Press '+operation+' again to confirm';return;}this.confirm='';this.request=operation==='save'?saves.save(this.state.slot,{name:'Water / poses — '+this.state.slot,metadata:{ticks:this.state.ticks}}):operation==='load'?saves.load(this.state.slot):saves.delete(this.state.slot);}catch(e){this.message=String(e);}}
 onUI(event){if(event.type==='click'&&event.element==='slot'){this.state.slot=this.state.slot==='slot-a'?'slot-b':'slot-a';this.confirm='';return;}if(event.type==='click'&&['save','load','delete'].includes(event.element)){this.command(event.element);return;}super.onUI(event);}
}
