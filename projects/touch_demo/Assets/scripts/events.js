export const totals={collisionEnter:0,collisionStay:0,collisionExit:0,triggerEnter:0,triggerStay:0,triggerExit:0,last:'Waiting for contacts'};
export const properties={speed:{type:'number',default:0},patrol:{type:'boolean',default:false}};
export default class {
 constructor({entity,properties}){this.entity=entity;this.properties=properties;this.direction=1;this.state={events:0};}
 fixedUpdate(){if(!this.properties.speed)return;const t=this.entity.transform;
  if(this.properties.patrol){if(t.position.x>4)this.direction=-1;if(t.position.x< -4)this.direction=1;}
  else if(t.position.x> -0.8)return;
  this.entity.velocity={x:this.direction*this.properties.speed,y:0,z:0};
 }
 record(type,e){this.state.events++;totals[type]++;totals.last=`${type}: ${this.entity.id} / ${e.other.id}`;if(!type.endsWith('Stay'))console.log(totals.last);}
 onCollisionEnter(e){this.record('collisionEnter',e)} onCollisionStay(e){this.record('collisionStay',e)} onCollisionExit(e){this.record('collisionExit',e)}
 onTriggerEnter(e){this.record('triggerEnter',e)} onTriggerStay(e){this.record('triggerStay',e)} onTriggerExit(e){this.record('triggerExit',e)}
}
