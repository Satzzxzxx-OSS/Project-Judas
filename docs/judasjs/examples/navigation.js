import {navigation,world} from 'judas';
// Attach to an entity with NavigationAgent + CharacterMotor and a baked surface.
export default class {
  /** @param {import("judas").ScriptContext} context */
  constructor({entity}){this.entity=entity;this.state={sample:false,path:false,controlled:false};}
  fixedUpdate(){
    const motor=this.entity.character, agent=this.entity.navigation;
    if(!motor||!agent)return;
    const t=this.entity.transform;
    const foot={x:t.position.x-motor.up.x*.9,y:t.position.y-motor.up.y*.9,z:t.position.z-motor.up.z*.9};
    const sampled=navigation.sample(foot);
    if(!sampled)return; // ordinary asynchronous load
    this.state.sample=true;
    const target={x:sampled.position.x+2,y:sampled.position.y,z:sampled.position.z};
    if(!agent.state.hasDestination)agent.setDestination(target);
    this.state.path=navigation.path(sampled.position,target).status==='complete';
    const v=agent.steering, current=motor.velocity, up=motor.up;
    const vertical=current.x*up.x+current.y*up.y+current.z*up.z;
    motor.velocity={x:v.x+up.x*vertical,y:v.y+up.y*vertical,z:v.z+up.z*vertical};
    this.state.controlled=true;
    if(agent.state.onLink){
      const link=world.entity(agent.state.linkId);
      // Project-specific traversal goes here; acknowledge AFTER crossing.
      if(!link?.valid)agent.clear();
    }
  }
}
