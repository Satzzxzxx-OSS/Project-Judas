#!/usr/bin/env python3
"""Independent canonical default-scene golden packing; no Judas implementation imported."""
import hashlib
import struct

def text(value):
    return struct.pack('>Q', len(value.encode())) + value.encode()

for schema in (1, 2):
    data = text('Judas.AuthoredSceneFingerprint') + struct.pack('>II', schema, 3)
    data += text('') + struct.pack('>3d', 0, 0, 0)  # name and absolute origin
    data += struct.pack('>10f', .4, .7, .35, 1, .98, .92, .16, .17, .19, 1)
    if schema == 2:
        data += struct.pack('>2f', 30, 2)  # update Hz and hydrostatic drag /s
    data += struct.pack('>IQQ', 0, 1, 0)  # None policy, NextId, object count
    digest = hashlib.sha256(data).hexdigest()
    expected = {1: 'c51b7c4982f8e63fcb8e4c89b823dfe5142fe8c73d389427fd02916e6d98f955',
                2: '93bc87617161138c59355a3a1dcd1a79968977e4f9ef49256810b0b603968a32'}[schema]
    assert digest == expected
    print(f'schema={schema} bytes={len(data)} sha256={digest}')
