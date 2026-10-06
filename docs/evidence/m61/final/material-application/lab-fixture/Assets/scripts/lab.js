import {world,input,ui,scenes,entity} from 'judas';
export default class {
 constructor(){this.position={x:0,y:4,z:15};this.yaw=0;this.pitch=-.12;this.state={elapsed:0};}
 start(){ui.debugOverlayVisible=false;input.pointerCapture=true;}
 update(dt){if(input.pressed('pause'))input.pointerCapture=!input.pointerCapture;if(input.pressed('restart'))scenes.reload();
 if(input.pointerCapture){this.yaw-=input.axis('look_x')*.002;this.pitch=Math.max(-1.3,Math.min(1.3,this.pitch-input.axis('look_y')*.002));}
 this.state.elapsed+=dt;const item=entity('40');if(item)item.material().set({baseColor:{x:.15+.1*Math.sin(this.state.elapsed),y:.3,z:.03,a:1},roughness:.2+.15*(1+Math.sin(this.state.elapsed))});
 if(input.pressed("spawn_navigator"))world.spawnPrefab("44914741099d012af2c3eca432018951",{position:{x:0,y:0,z:7}});
 if(input.pressed('blocker_toggle'))world.setAppearance({environmentBackground:!world.appearance.environmentBackground});
 if(input.pressed('camera_toggle')){let s=world.appearance;world.setAppearance({exposure:s.exposure>1? .7:2.5});}
 }
 fixedUpdate(dt){let x=input.axis('move_x'),z=input.axis('move_y');this.position.x+=(Math.cos(this.yaw)*x-Math.sin(this.yaw)*z)*dt*5;this.position.z+=(-Math.sin(this.yaw)*x-Math.cos(this.yaw)*z)*dt*5;}
 presentationUpdate(){let cy=Math.cos(this.yaw/2),sy=Math.sin(this.yaw/2),cp=Math.cos(this.pitch/2),sp=Math.sin(this.pitch/2);world.setView({position:this.position,rotation:{w:cy*cp,x:cy*sp,y:sy*cp,z:-sy*sp}},60);}
 destroy(){input.pointerCapture=false;world.clearView();}
}
