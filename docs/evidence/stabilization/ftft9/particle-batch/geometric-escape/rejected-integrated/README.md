# Geometric endpoint candidate rejected

The two bounded endpoint exclusion attempts passed their isolated boundary fixtures (12/60 and 14/78), but the integrated dense020 case leaked two particles far below its floor (reported y approximately -71 and -69), with 143 unresolved geometry events and zero contact caps. This is worse than the earlier three particles trapped near y=-0.075. The candidate is therefore rejected. Its isolated PASS is not production acceptance.

Full current FluidWorld/header/boundary source and hashes are preserved here before rollback. ROLLBACK_PREPARED_NOT_APPLIED.patch restores only the earlier collision/projection code and original 8-case/24-check boundary suite; it does not alter the restored Spiky pressure kernel, other pressure diagnostics or the separate auxiliary normal-PCG safeguard. Geometry diagnostic fields remain available but emit zero after the implementation is removed, with no remaining capability claim.

The six newly added boundary fixtures remain in this source snapshot as OPEN physical probes for future geometric policy. Removing candidate-only tests from the active suite follows rejection of their unaccepted implementation; no accepted boundary assertion is weakened. No third geometry policy, further testing or commit is authorized in this rollback task.

The prepared patch was applied after the parent confirmed combined validation finished with exit 1 and recorded its source fingerprints. ROLLBACK_APPLIED.json identifies the restored source. No build or test was rerun as part of rollback. Raw integrated outputs remain in the parent's fullbody evidence directory and must be linked here when their final path is known.
