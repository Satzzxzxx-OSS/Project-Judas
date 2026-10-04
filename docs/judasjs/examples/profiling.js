import {profiler} from "judas";
export default class {
  constructor(){this.state={calls:0,error:false,value:0,invalid:false};}
  start(){
    this.state.value=profiler.scope("Example nested",()=>profiler.scope("Example child",()=>{this.state.calls++;return 42;}));
    try{profiler.scope("Example throw",()=>{this.state.calls++;throw Error("expected example error");});}catch(e){this.state.error=e instanceof Error&&e.message==="expected example error";}
    try{profiler.counter("invalid",NaN);}catch(e){this.state.invalid=true;}
  }
  fixedUpdate(){profiler.scope("Example policy",()=>{this.state.calls++;profiler.counter("Example fixed calls",1);profiler.counter("Example gauge",this.state.calls,"latest");profiler.counter("Example maximum",this.state.calls,"max");});}
}
