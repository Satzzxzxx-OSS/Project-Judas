
import Demo from './demo.js';import {world,scenes,session} from 'judas';
export default class extends Demo {
 update(){super.update();if(this.state.checked||!this.a.info.ready)return;
 const assert=(v,m)=>{if(!v)throw Error(m);};
 if(session.get('ragdollReload')){assert(!world.entity('10').ragdoll.active&&!world.entity('11').ragdoll.active,'fresh scene has no articulation');this.state.checked=true;session.set('animationChecks',2);return;}
 this.frames=(this.frames||0)+1;
 const r=world.entity('10').ragdoll;
 if(this.frames===1){this.a.seek(.5);r.enter();assert(r.active,'activation');
   this.stale=r.body('Tip');assert(this.stale&&this.stale.valid,'safe mapped body');this.stale.applyImpulse({x:3,y:1,z:0});
   const root=this.spawn();root.ragdoll.enter();assert(root.ragdoll.active&&root.ragdoll.body('Root').id!==r.body('Root').id,'independent spawned ragdoll');root.destroy();assert(!root.valid,'hierarchy teardown');
 }
 if(this.frames===12){r.leave(.2);assert(!r.active&&!this.stale.valid,'released body handle');let rejected=false;try{this.stale.applyImpulse({x:1,y:0,z:0});}catch(e){rejected=true;}assert(rejected,'stale physics fails safely');}
 if(this.frames===30){assert(!r.active,'return complete');r.enabled=false;let rejected=false;try{r.enter();}catch(e){rejected=true;}assert(rejected,'disabled rejected');r.enabled=true;world.entity('11').ragdoll.enter();session.set('ragdollReload',true);scenes.reload();}
 }

}