import {input,console} from 'judas';
import {vec} from './helpers.js';
export const properties={action:{type:'string',default:'collect_key'}};
export default class Key {
 constructor({entity,properties}){this.entity=entity;this.properties=properties;this.state={collected:false};}
 start(){this.showState();}
 showState(){if(this.state.collected)this.entity.transform={position:vec(-2,-10,0)};}
 update(){if(!this.state.collected && input.pressed(this.properties.action)){
   this.state.collected=true;this.entity.playAudio();this.entity.burst(30);this.showState();console.log('Key collected: saved JavaScript state.');
 }}
}
