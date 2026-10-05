#!/usr/bin/env python3
"""Launch an unmodified moved export from /tmp, make installation read-only,
and close only our own SDL window with the normal WM_DELETE_WINDOW protocol.
No script/assets/OS volume changes; this is startup/lifetime proof, not listening.
"""
import ctypes as C,json,os,re,shutil,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class Data(C.Union):_fields_=[('b',C.c_char*20),('s',C.c_short*10),('l',C.c_long*5)]
class Client(C.Structure):_fields_=[('type',C.c_int),('serial',C.c_ulong),('send_event',C.c_int),('display',C.c_void_p),('window',C.c_ulong),('message_type',C.c_ulong),('format',C.c_int),('data',Data)]
class Event(C.Union):_fields_=[('client',Client),('pad',C.c_long*24)]
def close_window(pid):
 tree=subprocess.check_output(['xwininfo','-root','-tree'],text=True)
 for window in re.findall(r'^\s*(0x[0-9a-f]+) ',tree,re.M):
  record=subprocess.run(['xprop','-id',window,'_NET_WM_PID'],capture_output=True,text=True).stdout
  if not re.search(r'= '+str(pid)+r'\b',record):continue
  x=C.CDLL('libX11.so.6');x.XOpenDisplay.restype=C.c_void_p;display=x.XOpenDisplay(None)
  x.XInternAtom.argtypes=[C.c_void_p,C.c_char_p,C.c_int];x.XInternAtom.restype=C.c_ulong
  x.XSendEvent.argtypes=[C.c_void_p,C.c_ulong,C.c_int,C.c_long,C.POINTER(Event)];x.XFlush.argtypes=[C.c_void_p];x.XCloseDisplay.argtypes=[C.c_void_p]
  e=Event();e.client.type=33;e.client.display=display;e.client.window=int(window,16);e.client.message_type=x.XInternAtom(display,b'WM_PROTOCOLS',0);e.client.format=32;e.client.data.l[0]=x.XInternAtom(display,b'WM_DELETE_WINDOW',0)
  x.XSendEvent(display,int(window,16),0,0,C.byref(e));x.XFlush(display);x.XCloseDisplay(display);return window
 raise RuntimeError('Own exported window not found; no other window was closed')
def main():
 source=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();label=sys.argv[3];out.mkdir(parents=True,exist_ok=True)
 moved=Path('/tmp')/('Judas_M60_'+label);assert not moved.exists();shutil.copytree(source,moved)
 for p in moved.rglob('*'):
  if p.is_file():p.chmod(0o555 if p.name=='judas' else 0o444)
 for p in sorted((p for p in moved.rglob('*') if p.is_dir()),key=lambda p:len(p.parts),reverse=True):p.chmod(0o555)
 moved.chmod(0o555)
 env=os.environ.copy();env.pop('JUDAS_TEST_SCRIPT',None);env['JUDAS_PROFILE']='1';env['JUDAS_PROFILE_OUTPUT']=str(out/'profile.json')
 before=time.monotonic()
 with (out/'runtime.log').open('w') as log:
  process=subprocess.Popen([str(moved/'judas')],cwd='/tmp',env=env,stdout=log,stderr=subprocess.STDOUT)
  try:
   time.sleep(4);assert process.poll() is None,'Standalone exited before close';window=close_window(process.pid);code=process.wait(timeout=15)
  finally:
   if process.poll() is None:process.terminate();process.wait(timeout=15)
 text=(out/'runtime.log').read_text();assert code==0 and 'Project:' in text and 'script asset=' not in text and 'Asset problem:' not in text
 result={'exit_code':code,'seconds':time.monotonic()-before,'package':str(moved),'cwd':'/tmp','installation_read_only':True,'closed_own_window':window,'package_bytes':sum(p.stat().st_size for p in moved.rglob('*') if p.is_file()),'checks':'normal standalone project startup, no script/asset fault, clean own-window close','human_listening':'NOT CLAIMED'}
 (out/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
