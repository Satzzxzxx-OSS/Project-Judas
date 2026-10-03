import {scenes,session} from 'judas';
export default class {
  constructor() {this.state={queued:false};}
  start() {
    const count=session.get('example_visits');
    session.set('example_visits',typeof count==='number'?count+1:1);
  }
  uiUpdate() {
    if(session.get('example_visits')===1&&!this.state.queued)this.state.queued=scenes.reload();
  }
}
