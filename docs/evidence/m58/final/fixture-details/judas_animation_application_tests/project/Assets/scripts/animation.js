
import Demo from './demo.js';import {world,scenes,session} from 'judas';
export default class extends Demo {
 update(){super.update();if(this.state.checked||!this.a.info.ready)return;
 const assert=(value,message)=>{if(!value)throw Error(message);};
 assert(this.a.clips.length===2&&this.a.clips[0].name==='Wave','named clips');
 this.a.play('Stretch');this.a.loop=false;this.a.speed=2;this.a.seek(.5);this.a.pause();assert(this.a.time===.5&&!this.a.playing,'seek/pause');
 this.a.resume();assert(this.a.playing,'resume');this.a.stop();assert(this.a.time===0&&!this.a.playing,'stop/reset');
 this.a.play('Wave');this.a.loop=true;this.a.speed=1;this.a.seek(.25);
 assert(this.b.info.clip==='Stretch'&&this.b.speed===1,'independent instance');
 let rejected=false;try{this.a.play('missing');}catch(e){rejected=true;}assert(rejected,'unknown clip rejected');
 const root=this.spawn();assert(root.animation&&root.animation.info.ready,'prefab animation');const stale=root.animation;root.destroy();
 rejected=false;try{stale.info;}catch(e){rejected=true;}assert(rejected,'stale animation fails safely');
 this.state.checked=true;session.set('animationChecks',(session.get('animationChecks')||0)+1);
 if(!session.get('animationReload')){session.set('animationReload',true);scenes.reload();}
 }
}