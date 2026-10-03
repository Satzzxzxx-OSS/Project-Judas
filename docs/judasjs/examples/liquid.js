import {world,liquid} from 'judas';
export default class {
 constructor(){this.state={ready:false,transferred:false,conserved:false,sample:false};}
 fixedUpdate(){if(this.state.transferred)return;const source=world.entity('20')?.liquid,target=world.entity('21')?.liquid;if(!source||!target)return;this.state.ready=true;const before=liquid.accounting().total;const amount=source.transferTo(target,.8);this.state.transferred=Math.abs(amount-.8)<1e-9;const after=liquid.accounting();this.state.conserved=Math.abs(before-after.total)<=after.tolerance;const sample=liquid.sample({x:-3,y:.1,z:-3});this.state.sample=sample!==null;}
}
