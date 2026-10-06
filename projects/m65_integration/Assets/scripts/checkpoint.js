import {link} from './runner.js';
export const properties={index:{type:'number',default:0},finish:{type:'boolean',default:false}};
export default class {constructor({properties}){this.props=properties;}onTriggerEnter(e){if(e.other?.id!==link.runner?.entity.id)return;if(this.props.finish)link.course?.finish();else link.course?.checkpoint(this.props.index);}}
