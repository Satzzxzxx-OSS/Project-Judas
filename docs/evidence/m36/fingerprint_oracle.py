#!/usr/bin/env python3
# Independent big-endian schema-5 Scene{} reference, no engine calls.
import struct, hashlib
text=b'Judas.AuthoredSceneFingerprint'
packed=struct.pack('>Q',len(text))+text
packed+=struct.pack('>IIQ',5,3,0) # canonical schema, scene grammar, empty name
packed+=struct.pack('>3d',0,0,0)
packed+=struct.pack('>12f',.4,.7,.35, 1,.98,.92, .16,.17,.19, 1,30,2)
packed+=struct.pack('>IQQ',0,1,0) # None fidelity; NextId; zero objects
print('bytes',len(packed),'sha256',hashlib.sha256(packed).hexdigest())
