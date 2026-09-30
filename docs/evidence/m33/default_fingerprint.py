#!/usr/bin/env python3
"""Independent empty-scene canonical golden: explicit endian packing, no engine."""
import hashlib
import struct
label = b'Judas.AuthoredSceneFingerprint'
def packed(schema):
    return (struct.pack('>Q', len(label)) + label + struct.pack('>IIQ', schema, 3, 0)
            + struct.pack('>ddd', 0, 0, 0)
            + struct.pack('>ffffffffffff', .4, .7, .35, 1, .98, .92, .16, .17, .19, 1, 30, 2)
            + struct.pack('>IQQ', 0, 1, 0))
assert len(packed(3)) == 146
assert hashlib.sha256(packed(2)).hexdigest() == '93bc87617161138c59355a3a1dcd1a79968977e4f9ef49256810b0b603968a32'
assert hashlib.sha256(packed(3)).hexdigest() == 'a85d3fdd8857106fe17a9d109711ae79bd8b088c40686b28924375bf9d09f3a3'
print('Schema-3 independent golden PASS:', hashlib.sha256(packed(3)).hexdigest())
