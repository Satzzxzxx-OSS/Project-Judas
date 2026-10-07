
import Demo from './demo.js';import {world,scenes,session} from 'judas';
export default class extends Demo {
 update(){super.update();if(this.state.checked||!this.a.info.ready)return;
 const assert=(value,message)=>{if(!value)throw Error(message);};
 assert(this.a.clips.length===2,'shared clips');
 this.a.play('Wave');this.a.seek(.3);this.a.crossFade('Stretch',.5);
 assert(this.a.info.transitioning&&this.a.info.transitionFraction===0,'crossfade starts');
 this.a.crossFade('Wave',0);assert(!this.a.info.transitioning&&this.a.time===0,'immediate fade');
 this.a.layer('tip',{clip:'Stretch',weight:.5,mask:['Root/Elbow/Tip']});
 this.a.layer('tip',{weight:.25,enabled:false});
 assert(this.a.layers.length===1&&this.a.layers[0].weight===.25&&!this.a.layers[0].enabled,'layer update');
 this.a.layer('add',{clip:'Wave',weight:.4,additive:true,referenceClip:'Wave',referenceTime:0,mask:['Root/Elbow']});
 let rejected=false;try{this.a.layer('invalid',{clip:'Wave',mask:['missing']});}catch(e){rejected=true;}assert(rejected,'invalid mask rejected');
 this.a.removeLayer('tip');assert(this.a.layers.length===1,'remove layer');
 assert(this.b.info.clip==='Stretch'&&!this.b.info.transitioning&&this.b.layers.length===0,'independent mixer');
 const root=this.spawn();assert(root.animation&&root.animation.info.ready,'prefab animation');root.animation.crossFade('Stretch',.2);
 assert(root.animation.info.transitioning&&!this.b.info.transitioning,'prefab independent fade');
 const stale=root.animation;root.destroy();rejected=false;try{stale.info;}catch(e){rejected=true;}assert(rejected,'stale mixer fails safely');
 this.state.checked=true;session.set('animationChecks',(session.get('animationChecks')||0)+1);
 if(!session.get('animationReload')){session.set('animationReload',true);scenes.reload();}
 }
}