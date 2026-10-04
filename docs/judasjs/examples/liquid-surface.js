import {world,liquid} from 'judas';
// Author a dynamic basin and a compatible storage basin in the project. IDs
// below identify content in liquid_surface_demo, not any engine convention.
export default class {
 constructor(){this.state={ready:false,conserved:false,impulse:false,presented:false};}
 fixedUpdate(){
  const source=world.entity('20')?.liquid,store=world.entity('21')?.liquid;
  if(!source||!store||this.state.ready)return;
  const before=liquid.accounting().total;
  const amount=source.transferTo(store,.8);
  store.transferTo(source,amount);
  const sample=liquid.sample({x:-3,y:2.55,z:-3});
  if(sample)this.state.impulse=source.applyImpulse(sample.surfacePoint,{x:2,y:0,z:0});
  const ledger=liquid.accounting();
  this.state.conserved=Math.abs(before-ledger.total)<=ledger.tolerance;
  source.surfaceEnabled=false; // pause dynamics; retain owned water and flow
  source.surfaceEnabled=true;
  this.state.ready=source.state.surface!==null;
 }
 presentationUpdate(){
  const sample=liquid.samplePresented({x:-3,y:2.55,z:-3});
  this.state.presented=sample!==null;
 }
}
