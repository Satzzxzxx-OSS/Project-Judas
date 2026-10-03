import {physics,world} from 'judas';
import {register,unregister} from './round.js';
export const properties={hinge:{type:'string',default:''},points:{type:'number',default:100}};
export default class {
 constructor({entity,properties}){this.entity=entity;this.props=properties;this.state={ready:true,hitOnce:false,cooldown:0,angle:0};}
 start(){this.joint=physics.joint(world.entity(this.props.hinge));register(this);}
 fixedUpdate(dt){const s=this.joint.state;this.state.angle=s.coordinate;
  if(!this.state.ready){this.state.cooldown+=dt;
   // Only observations. Solver limits/spring return the plate, NEVER a transform reset.
   if(this.state.cooldown>.55&&Math.abs(s.coordinate)<.12&&Math.abs(this.entity.angularVelocity.x)<.65)this.state.ready=true;}
 }
 destroy(){unregister(this.entity.id);}
}
