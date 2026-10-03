// Shared only inside this scene's VM. Reload creates a fresh round and handles.
export const round={score:0,shots:0,hits:0,unique:0,elapsed:0,message:'Hit all 12 plates. Every ready plate can score again.',flash:0};
export const targets=new Map();
export function register(target){targets.set(target.entity.id,target);}
export function unregister(id){targets.delete(id);}
export function hit(entity){const target=targets.get(entity.id);if(!target||!target.state.ready)return false;
 target.state.ready=false;target.state.cooldown=0;round.score+=target.props.points;round.hits++;
 if(!target.state.hitOnce){target.state.hitOnce=true;round.unique++;}
 round.flash=.2;round.message=round.unique===targets.size?'RANGE CLEARED! Keep practising or restart.':`+${target.props.points} - physical hit!`;
 return true;
}
