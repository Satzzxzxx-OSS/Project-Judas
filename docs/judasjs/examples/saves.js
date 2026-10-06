import {saves, session} from 'judas';
export const properties={cancelForFixture:{type:'boolean',default:false}};
export default class {
 /** @param {import('judas').ScriptContext} context */
 constructor({entity,properties}){this.entity=entity;this.props=properties;this.request=0;this.state={key:'',resolved:false,queued:false,cancelled:false,invalidTokens:false,score:10};}
 start(){
  this.state.key=saves.reference(this.entity)||'';
  this.state.resolved=saves.resolve(this.state.key)?.id===this.entity.id;
  session.set('save_example',{score:this.state.score,held:this.state.key});
  this.request=saves.save('example',{name:'Example — 日本語',metadata:{score:this.state.score}});
  this.state.queued=saves.status(this.request)?.state==='queued';
  if(this.props.cancelForFixture){let rejected=0;for(const token of [NaN,Infinity,-1,.5])for(const operation of [saves.status,saves.cancel]){try{operation(token);}catch{rejected++;}}this.state.invalidTokens=rejected===8;}
  // Only the fixture cancels, proving the documented pre-commit boundary.
  if(this.props.cancelForFixture)this.state.cancelled=saves.cancel(this.request);
 }
 restore(){this.state.resolved=saves.resolve(this.state.key)?.id===this.entity.id;}
 uiUpdate(){
  if(!this.request)return;
  const result=saves.status(this.request);
  if(result?.state==='failed')throw Error(result.error);
  if(result?.state==='completed'||result?.state==='cancelled')this.request=0;
 }
}
