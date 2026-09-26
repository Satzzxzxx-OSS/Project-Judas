#!/usr/bin/env python3
"""Rerun original fixed-grid function definitions without importing unused Shapely.
No supplied source is modified. Moving-geometry functions and the original main
(which calls them and rewrites shipped results) are deliberately not executed.
"""
import ast
import hashlib
import json
import platform
import sys
import time
import zipfile
from pathlib import Path
import numpy as np

OUT = Path(__file__).resolve().parent
HANDOFF = OUT.parent / 'handoff'
SOURCE = HANDOFF / 'evidence/rechecked/compatibility_checks.py'
FUNCTIONS = {
    'clip', 'area', 'plic', 'flow', 'phase_flux', 'div', 'restrict_cells',
    'restrict_faces', 'momentum_face_flux', 'counterflow_check',
    'momentum_experiments', 'averaging_check',
}

def sha(data):
    return hashlib.sha256(data).hexdigest()

def check_integrity():
    manifest = json.loads((HANDOFF / 'SHA256.json').read_text())
    records = []
    for name, expected in manifest.items():
        actual = sha((HANDOFF / name).read_bytes())
        records.append(dict(path=name, expected=expected, actual=actual, passed=actual == expected))
    original = HANDOFF / 'evidence/original/Judas_R1_compatibility_research.zip'
    with zipfile.ZipFile(original) as z:
        nested = json.loads(z.read('SHA256.json'))
        for name, expected in nested.items():
            actual = sha(z.read(name))
            records.append(dict(path='original_zip/' + name, expected=expected, actual=actual, passed=actual == expected))
    assert all(r['passed'] for r in records), records
    return records

before = check_integrity()
source_bytes = SOURCE.read_bytes()
source = source_bytes.decode('utf-8')
tree = ast.parse(source, filename=str(SOURCE))
kept, omitted, selected = [], [], []
for node in tree.body:
    if isinstance(node, ast.FunctionDef):
        take = node.name in FUNCTIONS
        label = 'function ' + node.name
    elif isinstance(node, ast.ImportFrom):
        take = not node.module.startswith('shapely')
        label = 'import from ' + node.module
    elif isinstance(node, ast.Import):
        take = all(not alias.name.startswith('shapely') for alias in node.names)
        label = 'import ' + ','.join(alias.name for alias in node.names)
    else:
        take = False
        label = type(node).__name__
    record = dict(node=label, first_line=node.lineno, last_line=node.end_lineno)
    if take:
        kept.append(node)
        record['source_segment_sha256'] = sha(ast.get_source_segment(source, node).encode('utf-8'))
        selected.append(record)
    else:
        omitted.append(record)
assert {n.name for n in kept if isinstance(n, ast.FunctionDef)} == FUNCTIONS
namespace = {'__name__': 'fixed_grid_reference_subset', '__file__': str(SOURCE)}
exec(compile(ast.Module(body=kept, type_ignores=[]), str(SOURCE), 'exec'), namespace)
assert 'remap_experiments' not in namespace and 'grid_partition' not in namespace
start = time.perf_counter()
rows = namespace['momentum_experiments']()
counterflow = namespace['counterflow_check']()
averaging = namespace['averaging_check']()
# Independent literal arithmetic for the two analytic witnesses.
assert counterflow['momentum_transfer'] == .75
assert counterflow['net_mass_transfer'] == 0
assert averaging['correct_mass'] == 5 and averaging['correct_momentum'] == .5
assert averaging['correct_velocity'] == .1 and averaging['wrong_momentum'] == 2.5
runtime = time.perf_counter() - start
assert len(rows) == 18 and sum(r['steps'] for r in rows) == 216
results = dict(scope='Supporting supplied Python reference only; fixed-grid momentum and two analytic witnesses; no C++ acceptance or moving geometry',
               momentum_ledger=rows, counterflow=counterflow, averaging=averaging,
               runtime_seconds=runtime)
keys = ['normalized_total_mass_error', 'normalized_total_momentum_error',
        'normalized_dual_mass_ledger_error', 'normalized_mass_restriction_error',
        'uniform_velocity_error', 'sweep_mean_vs_shared_flux_error',
        'max_full_step_relative_energy_increase', 'max_velocity_bound_overshoot',
        'negative_control_old_mass_velocity_error']
summary = dict(cases=len(rows), timesteps=sum(r['steps'] for r in rows),
               analytic_witnesses=2, runtime_seconds=runtime,
               min_fraction=min(r['min_fraction'] for r in rows),
               max_fraction=max(r['max_fraction'] for r in rows),
               extrema={k:max(r[k] for r in rows if r[k] is not None) for k in keys})
shipped = json.loads((HANDOFF / 'evidence/rechecked/compatibility_results.json').read_text())
comparison = dict(momentum_rows_exact_match=rows == shipped['momentum_ledger'],
                  counterflow_exact_match=counterflow == shipped['counterflow'],
                  averaging_exact_match=averaging == shipped['averaging'])
after = check_integrity()
assert before == after
metadata = dict(python=sys.version, executable=sys.executable, numpy=np.__version__,
                platform=platform.platform(), source=str(SOURCE), source_sha256=sha(source_bytes),
                loader_sha256=sha(Path(__file__).read_bytes()), selected_original_nodes=selected,
                excluded_original_nodes=omitted, shapely_available=False,
                dependency_note='Shapely absent; unused imports and excluded moving-geometry functions omitted transparently through AST selection; original selected function bodies compiled unmodified.',
                command=[sys.executable, str(Path(__file__).resolve())],
                integrity_entries=len(before), all_integrity_entries_passed=True,
                comparison_to_shipped_fixed_grid_results=comparison)
for name, value in [('results.json', results), ('summary.json', summary),
                    ('RUN_METADATA.json', metadata), ('integrity.json', before)]:
    (OUT / name).write_text(json.dumps(value, indent=2) + '\n')
text = json.dumps(dict(summary=summary, comparison=comparison, counterflow=counterflow, averaging=averaging), indent=2)
(OUT / 'run_output.txt').write_text(text + '\n')
print(text)
