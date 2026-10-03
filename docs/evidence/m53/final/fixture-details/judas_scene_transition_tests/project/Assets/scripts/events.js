import {scenes,session} from 'judas';
export const properties={speed:{type:'number',default:0},patrol:{type:'boolean',default:false}};
export default class {
 constructor({entity,properties}){this.entity=entity;this.properties=properties;this.state={events:0};}
 fixedUpdate(){if(this.properties.speed)this.entity.velocity={x:this.properties.speed,y:0,z:0};}
 onTriggerEnter(e){if(this.entity.id!=='6'||scenes.current!=='Scenes/a.judas')return;
  session.set('hasKey',true);session.set('visits',(session.get('visits')||0)+1);
  scenes.load('Scenes/b.judas');
  // This statement must execute before the old world can be destroyed.
  session.set('callbackCompleted',true);this.state.events++;
 }
}
