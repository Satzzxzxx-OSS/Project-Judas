import {world,physics} from 'judas';
export const properties={obstacle:{type:'string',default:''}};
export default class {
 constructor({entity,properties}){this.entity=entity;this.props=properties;this.state={events:0,physical:0,open:false};}
 onFracture(e){this.state.events++;this.state.physical+=e.interfaces.filter(b=>b.cause==='physical').length;}
 fixedUpdate(){if(!this.props.obstacle||this.state.open)return;
  // The floor is baked independently. Remove its ordinary obstacle only after
  // a real central opening exists; debris elsewhere still remains physical.
  if(!physics.raycast({x:0,y:1,z:2},{x:0,y:0,z:-1},4)?.entity&&!physics.raycast({x:0,y:2.2,z:2},{x:0,y:0,z:-1},4)?.entity){this.state.open=true;world.entity(this.props.obstacle)?.setNavigationEnabled('obstacle',false);}
 }
}
