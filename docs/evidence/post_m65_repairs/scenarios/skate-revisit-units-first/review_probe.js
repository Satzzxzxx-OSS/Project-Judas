import {world,console} from 'judas';
import {game} from '../skate/scripts/game.js';
export default class {constructor(){this.n=0;this.state={};}fixedUpdate(){++this.n;
if(this.n===240||this.n===480){const s=game.skater, p=this.n===240?{x:0,y:3.28,z:23}:{x:36.6,y:-18.43,z:-250};s.e.transform={position:p};s.e.velocity={x:0,y:0,z:0};this.state.returning=this.n===480;console.log('REVIEW backtrack '+JSON.stringify(p));}}}
