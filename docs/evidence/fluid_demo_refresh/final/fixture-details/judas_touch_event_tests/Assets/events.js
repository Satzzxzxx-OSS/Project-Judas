import {world,time} from 'judas';
 export const properties={destroy:{type:'boolean',default:false},disable:{type:'boolean',default:false}};
 export default class {constructor({entity,properties}){this.entity=entity;this.properties=properties;this.state={enter:0,stay:0,exit:0};}
 onTriggerEnter(e){this.state.enter++;this.state.other=e.other.id;this.state.safe=e.other.valid;this.state.point=Number.isFinite(e.point.x);this.state.fixed=time.fixed;
 if(this.properties.destroy)e.other.destroy();if(this.properties.disable)this.entity.setColliderEnabled(false);}
 onTriggerStay(e){this.state.stay++;}
 onTriggerExit(e){this.state.exit++;this.state.otherValid=e.other.valid;}
 onCollisionEnter(e){this.state.collision=true;}
 }