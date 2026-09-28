from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parents[1]
data=json.loads((ROOT/'SHA256.json').read_text())
for name,expected in data.items():
    path=ROOT/name
    assert path.is_file(),f'missing {name}'
    actual=hashlib.sha256(path.read_bytes()).hexdigest()
    assert actual==expected,(name,actual,expected)
print(f'PASS: {len(data)} file hashes')
