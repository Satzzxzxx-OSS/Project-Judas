// Collection navigation is project content, not an engine game mode.
import {input,scenes,ui,world} from 'judas';
export const properties={launcher:{type:'boolean',default:false}};
const scenesByKey={collection_skate:'Scenes/skate/park.judas',collection_rooftop:'Scenes/rooftop/rooftops.judas',collection_void:'Scenes/void/system.judas',collection_menu:'Scenes/launcher.judas'};
export default class {
 constructor({properties}){this.launcher=properties.launcher;}
 start(){if(this.launcher){input.pointerCapture=false;ui.debugOverlayVisible=false;world.setView({position:{x:0,y:1,z:5},rotation:{w:1,x:0,y:0,z:0}},70);}}
 uiUpdate(){for(const [action,scene] of Object.entries(scenesByKey))if(input.pressed(action)){scenes.load(scene);return;}}
 onUI(e){if(e.type==='click'&&e.document==='collection'){const scene=scenesByKey['collection_'+e.element];if(scene)scenes.load(scene);if(e.element==='quit')ui.quit();}}
 destroy(){if(this.launcher)world.clearView();}
}
