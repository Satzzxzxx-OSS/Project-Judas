# Expanded fixture follow-up

The first expanded run reached the surface-crossing proof, then rejected the
new disconnected-surface fixture. It copied an already prepared three-node asset
and appended nodes while retaining the old three-entry mass-weight stream.
The test incorrectly continued into simulation after reporting that rejection,
causing a bounds failure. The original log is retained.

The fixture now clears derived/default streams before preparing its six-node
topology. Instance initialization also rejects unprepared topology rather than
assuming caller preparation. No physical tolerance was changed.
