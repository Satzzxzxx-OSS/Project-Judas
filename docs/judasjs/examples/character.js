import {input} from 'judas';
export const properties={speed:{type:'number',default:4},launch:{type:'number',default:5}};
export default class {
  /** @param {import('judas').ScriptContext<{speed:number,launch:number}>} context */
  constructor({entity,properties}){this.entity=entity;this.props=properties;this.state={controlled:false,launched:false};}
  fixedUpdate() {
    const motor=this.entity.character;if(!motor)return;
    const s=motor.state,t=this.entity.transform;
    const forward=rotate(t.rotation,{x:0,y:0,z:-1}),right=rotate(t.rotation,{x:1,y:0,z:0});
    const x=input.axis('move_x'),y=input.axis('move_y');
    let v=s.velocity;
    if(s.supported) {
      v={x:(right.x*x+forward.x*y)*this.props.speed+s.supportVelocity.x,
         y:(right.y*x+forward.y*y)*this.props.speed+s.supportVelocity.y,
         z:(right.z*x+forward.z*y)*this.props.speed+s.supportVelocity.z};
      if(input.pressed('jump')){v={x:v.x+s.up.x*this.props.launch,y:v.y+s.up.y*this.props.launch,z:v.z+s.up.z*this.props.launch};this.state.launched=true;}
    }
    motor.velocity=v;this.state.controlled=true;
    // Optional game composition: inspect actualDisplacement/supported to choose an
    // authored animation clip/crossFade; the motor doesn't own animation or camera.
  }
}
/** @param {import('judas').Quat} q @param {import('judas').Vec3} v */
function rotate(q,v){const tx=2*(q.y*v.z-q.z*v.y),ty=2*(q.z*v.x-q.x*v.z),tz=2*(q.x*v.y-q.y*v.x);return {x:v.x+q.w*tx+q.y*tz-q.z*ty,y:v.y+q.w*ty+q.z*tx-q.x*tz,z:v.z+q.w*tz+q.x*ty-q.y*tx};}
