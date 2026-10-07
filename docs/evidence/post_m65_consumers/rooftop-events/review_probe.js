import {console} from 'judas';
export default class {constructor(){this.state={enter:0,stay:0,exit:0,triggers:0,valid:false};}
onCollisionEnter(e){++this.state.enter;this.state.valid=e.other?.valid===true;this.state.other=e.other?.id??null;}
onCollisionStay(e){++this.state.stay;}onCollisionExit(e){++this.state.exit;}onTriggerEnter(e){++this.state.triggers;}uiUpdate(){console.log('REVIEW '+JSON.stringify(this.state));}}
