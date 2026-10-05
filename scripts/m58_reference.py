#!/usr/bin/env python3
"""Verify stored shaping fixtures using the unmodified pinned upstream hb-shape.
Supply a built upstream utility; build instructions and font hashes accompany the
fixture. This verifies rather than rewriting expected data.
"""
import argparse,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('hb_shape');args=p.parse_args()
for case in json.loads((ROOT/'tests/fixtures/m58/reference.json').read_text()):
 font=ROOT/case['font'];assert hashlib.sha256(font.read_bytes()).hexdigest()==case['font_sha256']
 command=[args.hb_shape]+case['command'][1:];actual=json.loads(subprocess.check_output(command,cwd=ROOT))
 assert actual==case['glyphs'],case['name'];print('PASS',case['name'])
