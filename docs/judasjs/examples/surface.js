import * as api from 'judas';
export default class {
  /** @type {{surface:string[]}} */
  state={surface:[]};
  start(){
    for(const [key,value] of Object.entries(api)){
      this.state.surface.push(key);
      if(typeof value==='function'&&/^class /.test(Function.prototype.toString.call(value))){
        const instance=Reflect.construct(value,['0','element']);
        for(const member of Object.getOwnPropertyNames(value.prototype))this.state.surface.push(key+'.'+member);
        for(const member of Object.keys(instance))this.state.surface.push(key+'.'+member);
      }else if(value&&typeof value==='object')for(const member of Object.getOwnPropertyNames(value))this.state.surface.push(key+'.'+member);
    }
    this.state.surface=[...new Set(this.state.surface)].sort();
  }
}
