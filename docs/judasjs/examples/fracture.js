import {world} from 'judas';
export default class {
 /** @param {import('judas').ScriptContext} context */
 constructor({entity}){this.entity=entity;this.state={submitted:false,notifications:0,staleRevision:false,queued:false,loaded:false,ordinary:false,invalid:false,staleThrows:false,complete:false};/** @type {import('judas').Entity|null} */ this.fragment=null;}
 fixedUpdate(){const f=this.entity.fracture;if(!f)return;
  if(!this.state.submitted){const s=f.state;if(s.rigid&&!s.parts[4].entity)return;this.state.staleRevision=!f.release(s.interfaces[0].key,0);
   this.state.queued=f.release(s.interfaces[0].key,s.revision);
   this.state.loaded=f.impulse(4,{x:0,y:0,z:-80});
   this.state.ordinary=s.parts.some(p=>p.entity?.valid&&p.entity.mass>0);
   this.fragment=world.spawnPrefab('63636363636363636363636363630007',{position:{x:5,y:8,z:3}});
   this.state.submitted=true;
  }

 }
 /** @param {import('judas').FractureEvent} e */
 onFracture(e){this.state.notifications++;this.state.complete=this.entity.fracture?.state.revision===e.revision&&e.interfaces.every(b=>['physical','explicit'].includes(b.cause));
  if(this.fragment?.valid&&this.fragment.fracture){const stale=this.fragment.fracture;this.fragment.destroy();this.state.invalid=!stale.valid;try{stale.state;}catch(e){this.state.staleThrows=e instanceof ReferenceError;}this.fragment=null;}
 }
}
