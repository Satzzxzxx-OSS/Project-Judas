#!/usr/bin/env python3
"""Create small generated codec fixtures outside Git; local FFmpeg only.
No runtime dependency and no modifications to demonstration media.
"""
import argparse, subprocess
from pathlib import Path
parser=argparse.ArgumentParser()
parser.add_argument('destination',nargs='?',default='.cache/m60-audio')
args=parser.parse_args();out=Path(args.destination);out.mkdir(parents=True,exist_ok=True)
for name,duration in [('long',60),('loop',.01)]:
 subprocess.run(['ffmpeg','-v','error','-f','lavfi','-i',f'sine=frequency=1000:sample_rate=48000:duration={duration}',
                 '-ac','2','-c:a','pcm_s16le','-y',str(out/(name+'.wav'))],check=True)
for extension,codec in [('mp3','libmp3lame'),('flac','flac')]:
 subprocess.run(['ffmpeg','-v','error','-i',str(out/'long.wav'),'-c:a',codec,'-y',str(out/('long.'+extension))],check=True)
subprocess.run(['ffmpeg','-v','error','-i',str(out/'long.wav'),'-c:a','libmp3lame','-write_xing','0','-y',str(out/'no-length.mp3')],check=True)
print('Generated bounded-stream fixtures:',out.resolve())
