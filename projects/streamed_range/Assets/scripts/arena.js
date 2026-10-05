import {input,world,ui} from 'judas';
import {round} from './round.js';
export default class {
 constructor(){this.state={blocker:true,spawned:0};}
 update(){if(ui.get('range_ui').modal)return;
  if(input.pressed('blocker_toggle')){const blocker=world.entity('554');this.state.blocker=!this.state.blocker;blocker.setNavigationEnabled('obstacle',this.state.blocker);blocker.setColliderEnabled(this.state.blocker);blocker.transform={position:{x:-16,y:this.state.blocker?1.5:-10,z:-2}};round.message=this.state.blocker?'Corridor blocker installed; agents repath.':'Corridor cleared; tiles update without rebaking.';}
  if(input.pressed('spawn_navigator')){world.spawnPrefab('53535353535353535353535353535302',{position:{x:-20,y:1,z:20}});this.state.spawned++;}
 }
}
