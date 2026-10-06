import {world} from 'judas';
export default class {
 constructor({entity}){this.entity=entity;this.state={elapsed:0};}
 fixedUpdate(dt){const motor=this.entity.character,nav=this.entity.navigation;if(!motor||!nav)return;this.state.elapsed-=dt;if(this.state.elapsed<=0){this.state.elapsed=.5;nav.setDestination({x:0,y:0,z:5});}const v=nav.state.steering,up=motor.up,own=motor.velocity;const normal=own.x*up.x+own.y*up.y+own.z*up.z;motor.velocity={x:v.x+normal*up.x,y:v.y+normal*up.y,z:v.z+normal*up.z};}
 destroy(){this.entity.navigation?.clear();}
}
