import {physics,console} from 'judas';
export default class {constructor({entity}){this.entity=entity;this.n=0;}check(ok,label){if(!ok)throw new Error(label);console.log('PASS '+this.entity.id+' '+label);}
 fixedUpdate(){this.n++;const e=this.entity,g=physics.gravity(e.transform.position);
  if(this.n===1)this.check(e.id==='3'?Math.abs(g.x-4)<.001&&Math.abs(g.y)<.001:Math.abs(g.y+9.81)<.001&&Math.abs(g.x)<.001,'same body script samples applicable uniform/radial gravity');
  if(this.n===2){this.check(e.id==='3'?e.velocity.x>0:e.velocity.y<0,'ordinary rigid body uses the same acceleration');if(e.id==='3')e.transform={position:{x:0,y:12,z:0}};}
  if(e.id==='3'&&this.n===3){this.check(Math.abs(g.y+9.81)<.001,'crossing restores radial query');e.transform={position:{x:1,y:0,z:0}};}
  if(e.id==='3'&&this.n===4)this.check(Math.abs(g.x-4)<.001&&Math.abs(g.y)<.001,'returning restores oblique uniform query');
 }
}