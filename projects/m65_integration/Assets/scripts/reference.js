export const properties={target:{type:'entity',default:null}};
export default class {constructor({properties}){this.target=properties.target;this.state={seen:false};}fixedUpdate(){this.state.seen=this.target?.valid===true;}}
