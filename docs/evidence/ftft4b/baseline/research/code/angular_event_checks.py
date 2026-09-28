#!/usr/bin/env python3
"""High-precision analytical OBB/plane angular event witnesses.
No assertion that this is a general-purpose production angular TOI solver.
"""
import mpmath as mp,json,time
from pathlib import Path
mp.mp.dps=80

def main():
 start=time.perf_counter();rows=[];checks=0
 for aa in ['0.5','1','2']:
  for bb in ['0.01','0.05']:
   a=mp.mpf(aa);b=mp.mpf(bb);R=mp.sqrt(a*a+b*b);phi=mp.atan2(b,a)
   for fraction in ['0.35','0.7']:
    d=mp.mpf(fraction)*R;star=mp.asin(d/R)-phi
    for turns in [1,5]:
     omega=2*mp.pi*turns;h=2*mp.pi/omega
     def gap(theta):return d-a*abs(mp.sin(theta))-b*abs(mp.cos(theta))
     first=star/omega;delta=mp.mpf('1e-20')/omega
     assert gap(0)>0 and gap(omega*h)>0 and gap(mp.pi/2)<0
     assert abs(gap(omega*first))<mp.mpf('1e-75')
     assert gap(omega*(first-delta))>0 and gap(omega*(first+delta))<0;checks+=3
     # Starts exactly in contact but departing; it contacts again later.
     initial=mp.pi-star;return_time=2*star/omega
     assert abs(gap(initial))<mp.mpf('1e-75')
     assert gap(initial+omega*delta)>0
     assert gap(initial+omega*(return_time-delta))>0
     assert gap(initial+omega*(return_time+delta))<0;checks+=4
     rows.append(dict(half_length=aa,half_height=bb,center_height=str(d),omega=str(omega),h=str(h),
          first_hit_from_horizontal=str(first),departing_contact_return_time=str(return_time),
          initial_gap=float(gap(0)),minimum_gap=float(gap(mp.pi/2))))
 out=dict(status='PASS',shape_motion_cases=len(rows),event_witnesses=2*len(rows),assertions=checks,
      precision_decimal_digits=mp.mp.dps,records=rows,
      negative_rules_detected=['checking endpoint configurations only','skipping initially touching/departing pair for remainder of step'],
      runtime_seconds=time.perf_counter()-start)
 (Path(__file__).resolve().parents[1]/'results/angular_event_results.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:v for k,v in out.items() if k!='records'},indent=2))
if __name__=='__main__':main()
