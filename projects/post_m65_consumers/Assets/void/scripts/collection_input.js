// Preserve this game's logical bindings inside the shared project.
import {input as engine} from 'judas';
const name=n=>n.startsWith('ui_')?n:'void_'+n;
export const input={axis:n=>engine.axis(name(n)),held:n=>engine.held(name(n)),pressed:n=>engine.pressed(name(n)),released:n=>engine.released(name(n)),get pointerCapture(){return engine.pointerCapture;},set pointerCapture(v){engine.pointerCapture=v;}};
