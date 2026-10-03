import {ui} from 'judas';
export const properties={asset:{type:'string',default:'49494949494949494949494949494901'}};
export default class {
  /** @param {import('judas').ScriptContext<{asset:string}>} context */
  constructor({properties}) {this.props=properties;this.state={clicked:false};}
  start() {
    this.document=ui.load(this.props.asset,'example_hud');
    this.document.modal=false;this.document.get('counter').text='Hello Judas';
    this.document.get('strength').value=.5;
  }
  /** @param {import('judas').UIEvent} event */
  onUI(event) {
    if(event.document==='example_hud'&&event.element==='start'&&event.type==='click') {
      this.state.clicked=true;if(this.document)this.document.get('counter').text='Clicked';
    }
  }
  destroy() { // Owner destruction may already have removed documents.
    const d=ui.get('example_hud');if(d)d.unload();
  }
}
