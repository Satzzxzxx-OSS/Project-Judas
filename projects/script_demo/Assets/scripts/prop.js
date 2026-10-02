import {input} from 'judas';
import {vec} from './helpers.js';
export const properties={pulse:{type:'number',default:1},active:{type:'boolean',default:true}};
export default class Prop {
 constructor({entity,properties}){this.entity=entity;this.properties=properties;this.state={steps:0,pulses:0};}
 fixedUpdate(){this.state.steps++;if(this.properties.active&&input.pressed('pulse')){this.state.pulses++;this.entity.applyImpulse(vec(0,this.properties.pulse,0));this.entity.applyTorque(vec(0,0.1,0));}}
}
