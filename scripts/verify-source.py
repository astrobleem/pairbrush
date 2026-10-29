"""Check included source against its recorded R2 identities."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / 'validation/source-sha256.json').read_text())
pins = json.loads((root / 'validation/FINALPIN.JSON').read_text())
for item in manifest:
    path = root / 'src' / item['name']
    data = path.read_bytes()
    if len(data) != item['bytes'] or hashlib.sha256(data).hexdigest().upper() != item['sha256']:
        raise SystemExit('Source mismatch: ' + item['name'])
for item in pins['source_pins']:
    path = root / 'src' / item['path'].replace('\\', '/').split('/')[-1]
    if hashlib.sha256(path.read_bytes()).hexdigest().upper() != item['sha256']:
        raise SystemExit('Historical pin mismatch: ' + path.name)
print(f"PASS {len(manifest)} source/build files; {len(pins['source_pins'])} historical pins")
