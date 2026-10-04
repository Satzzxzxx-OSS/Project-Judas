import {world,liquid,LiquidVolume,scenes,session,console,ui} from 'judas';
export const properties={radial:{type:'boolean',default:false}};
export default class {
 constructor(){this.clock=0;this.pending=false;}
 fixedUpdate(dt){if(this.pending)return;this.clock+=dt;if(this.clock<.2)return;
  const stage=session.get('m55Stage')||0,id=stage===2?'120':'20',e=world.entity(id);if(!e.valid)return;const main=e.liquid;if(!main)return;
  if(liquid.errors.length)throw Error(liquid.errors[0].message);
  const total=liquid.accounting();if(Math.abs(total.error)>total.tolerance)throw Error('ledger');
  if(!main.state.surface||Math.abs(main.state.surface.volume-main.state.volume)>1e-8)throw Error('partition');
  if(stage===0){const target=world.entity('21').liquid;if(!target)return;session.set('m55Old',main.handle);if(Math.abs(main.transferTo(target,.8)-.8)>1e-9||Math.abs(main.state.volume-.2)>1e-9)throw Error('800 L');session.set('m55Stage',1);console.log('M55_DRAIN_1000_TO_200');this.pending=true;scenes.reload();}
  else if(stage===1){if(Math.abs(main.state.volume-1)>1e-9)throw Error('reload quantity');const old=new LiquidVolume(session.get('m55Old'));if(old.valid)throw Error('stale world handle');let threw=false;try{old.state;}catch(e){threw=true;}if(!threw)throw Error('stale access');console.log('M55_RELOAD_SAFE');session.set('m55Stage',2);this.pending=true;scenes.load('Scenes/radial.judas');}
  else if(stage===2){if(Math.abs(main.state.volume-65)>1e-8)throw Error('radial quantity');const sample=liquid.sample({x:0,y:20.6,z:0});if(!sample||sample.up.y<.99)throw Error('radial query');const storage=world.entity('121').liquid;main.transferTo(storage,.8);storage.transferTo(main,.8);console.log('M55_RADIAL_REFILL_QUERY_PASS');session.set('m55Stage',3);this.pending=true;scenes.load('Scenes/lab.judas');}
  else {if(Math.abs(main.state.volume-1)>1e-9||liquid.accounting().detached!==0)throw Error('transition leak');console.log('M55_TRANSITIONS_RELOAD_EXPORT_PASS');ui.quit();}
 }
}
