import {link} from './lab.js';
export default class {onTriggerEnter(e){if(e.other?.id==='10')link.lab?.checkpoint('ENTER');}onTriggerStay(e){if(e.other?.id==='10')link.lab.state.sensorStay++;}onTriggerExit(e){if(e.other?.id==='10')link.lab?.checkpoint('EXIT');}}
