"""Validate RenderMax v1 asset package integrity and decoded GPU evidence (stdlib only).
Usage: python util/validate_rmasset.py <package.rmasset> [report.json]
No Unity rendering equivalence claim is made.
"""
import csv
import hashlib
import json
import math
import struct
import sys
from pathlib import Path


def validate(root):
    root = Path(root).resolve()
    errors, warnings, meshes, textures = [], [], [], []

    def package_path(relative):
        if not isinstance(relative, str) or not relative:
            raise ValueError('Invalid package path: ' + repr(relative))
        candidate = Path(relative)
        if candidate.is_absolute():
            raise ValueError('Unsafe relative path: ' + relative)
        path = (root / candidate).resolve()
        try:
            path.relative_to(root)
        except ValueError:
            raise ValueError('Unsafe relative path: ' + relative)
        return path

    verified_paths = set()

    def verified_path(relative):
        path = package_path(relative)
        if relative not in verified_paths:
            raise ValueError('Unlisted or unverified reference: ' + relative)
        return path

    def load(path, manifest=False):
        source = package_path(path) if manifest else verified_path(path)
        return json.loads(source.read_text(encoding='utf-8'))

    def check(ok, message):
        if not ok:
            errors.append(message)

    manifest = load('manifest.json', manifest=True)
    check(manifest.get('format') == 'rendermax.asset-package', 'Wrong package format')
    check(manifest.get('status') in ('completed', 'completedWithWarnings'), 'Package is incomplete')
    paths = set()
    total = 0
    for f in manifest.get('files', []):
        relative = f.get('path')
        if not isinstance(relative, str) or not relative:
            errors.append('Invalid manifest path: ' + repr(relative))
            continue

        # Resolve existing symlinks as well as '..' components before allowing any
        # filesystem access. Merely recording an error here is insufficient: an
        # untrusted package could otherwise make the validator hash files outside it.
        try:
            path = package_path(relative)
        except ValueError as error:
            errors.append(str(error))
            continue

        check(relative not in paths, 'Duplicate manifest path: ' + relative)
        paths.add(relative)
        if not path.is_file():
            errors.append('Missing file: ' + relative)
            continue
        h = hashlib.sha256()
        with path.open('rb') as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b''):
                h.update(block)
        size = path.stat().st_size
        total += size
        size_matches = size == f['bytes']
        hash_matches = h.hexdigest() == f['sha256']
        check(size_matches, 'Size mismatch: ' + relative)
        check(hash_matches, 'Hash mismatch: ' + relative)
        if size_matches and hash_matches:
            verified_paths.add(relative)

    def refs(obj):
        if isinstance(obj, dict):
            for v in obj.values():
                refs(v)
        elif isinstance(obj, list):
            for v in obj:
                refs(v)
        elif isinstance(obj, str) and obj.startswith(('assets/', 'draws/', 'events/')):
            check(obj in paths, 'Unlisted reference: ' + obj)

    for path in paths:
        if path.endswith('.json'):
            refs(load(path))
    constants, variables, shader_count = 0, 0, 0
    for d in manifest.get('draws', []):
        draw = load(d['path'])
        block_data = load(draw['constants'])['blocks']
        constants += len(block_data)
        variables += sum(len(b['variables']) for b in block_data)
        if draw.get('meshStatus') == 'failed':
            errors.append('Mesh export failed at event ' + str(draw['eventId']))
        if 'mesh' not in draw:
            continue
        m = load(draw['mesh'])
        with verified_path(m['vertices']).open(newline='') as f:
            rows = list(csv.reader(f))
        header, rows = rows[0], rows[1:]
        check(len(rows) == m['vertexCount'], 'Wrong vertex count: ' + d['path'])
        check(all(len(row) == len(header) for row in rows), 'Ragged vertex CSV: ' + d['path'])
        nonfinite = sum(not math.isfinite(float(c)) for row in rows for c in row[2:])
        if nonfinite:
            warnings.append({'eventId': draw['eventId'], 'nonFiniteVertexComponents': nonfinite})
        with verified_path(m['indices']).open(newline='') as f:
            idx = list(csv.DictReader(f))
        check(len(idx) == m['indexCount'], 'Wrong index count: ' + d['path'])
        check(all(0 <= int(i['index']) < len(rows) for i in idx), 'Out of range mesh index: ' + d['path'])
        if m['topology'] == 'Triangle List':
            check(len(idx) % 3 == 0, 'Incomplete triangle list: ' + d['path'])
        # Independent FLOAT32 decode from original stream bytes, not CSV self-consistency only.
        streams = {s['slot']: s for s in m['streams']}
        blobs = {}
        compared = 0
        col = 2
        bounds = {}
        for attr in m['attributes']:
            if attr['perInstance']:
                continue
            n = attr['components']
            if attr['format'].endswith('_FLOAT') and '32' in attr['format'] and not attr['generic']:
                stream = streams[attr['slot']]
                if stream['path'] not in blobs:
                    blobs[stream['path']] = verified_path(stream['path']).read_bytes()
                b = blobs[stream['path']]
                mins, maxs = [float('inf')] * n, [float('-inf')] * n
                for row in rows:
                    offset = int(stream['byteOffset']) + attr['byteOffset'] + int(row[1]) * stream['byteStride']
                    if offset + 4 * n > len(b):
                        errors.append('Raw vertex fetch out of range: ' + d['path'])
                        break
                    raw = struct.unpack_from('<' + 'f' * n, b, offset)
                    vals = [float(x) for x in row[col:col + n]]
                    check(all(x == y or (math.isnan(x) and math.isnan(y)) for x, y in zip(raw, vals)),
                          'CSV differs from original vertex buffer: ' + d['path'])
                    compared += n
                    for j, v in enumerate(vals):
                        if math.isfinite(v):
                            mins[j], maxs[j] = min(mins[j], v), max(maxs[j], v)
                bounds[attr['name']] = {'min': mins, 'max': maxs}
            col += n
        meshes.append({'eventId': draw['eventId'], 'vertices': len(rows), 'indices': len(idx),
                       'comparedRawFloatComponents': compared, 'bounds': bounds})

    for path in sorted(paths):
        if path.endswith('/shader.json'):
            shader_count += 1
        if not path.endswith('/texture.json'):
            continue
        t = load(path)
        for image in t['images']:
            p = verified_path(image['path'])
            b = p.read_bytes()
            mip = max(image['mip'], 0)
            width, height = max(t['width'] >> mip, 1), max(t['height'] >> mip, 1)
            if p.suffix == '.png':
                check(b[:8] == b'\x89PNG\r\n\x1a\n', 'Invalid PNG: ' + str(p))
                check(struct.unpack_from('>II', b, 16) == (width, height), 'Wrong PNG dimensions: ' + str(p))
            elif p.suffix == '.tga':
                check(struct.unpack_from('<HH', b, 12) == (width, height), 'Wrong TGA dimensions: ' + str(p))
                check(b[16] == 32, 'Alpha not preserved as RGBA32 TGA: ' + str(p))
            elif p.suffix == '.dds':
                check(b[:4] == b'DDS ', 'Invalid DDS: ' + str(p))
                check(struct.unpack_from('<II', b, 12) == (height, width), 'Wrong DDS dimensions: ' + str(p))
                if t['cubemap']:
                    caps2 = struct.unpack_from('<I', b, 112)[0]
                    check(caps2 & 0xfe00 == 0xfe00, 'DDS missing cube face flags: ' + str(p))
            elif p.suffix == '.exr':
                check(b[:4] == b'\x76\x2f\x31\x01', 'Invalid EXR: ' + str(p))
        textures.append({'eventId': t['eventId'], 'resourceId': t['resourceId'], 'format': t['format'],
                         'width': t['width'], 'height': t['height'], 'cubemap': t['cubemap'], 'images': len(t['images'])})
    return {'package': str(root), 'passed': not errors, 'errors': errors, 'warnings': warnings,
            'filesChecked': len(paths), 'bytesChecked': total, 'draws': len(manifest.get('draws', [])),
            'shaderAssets': shader_count, 'constantBlocks': constants, 'constantVariables': variables,
            'meshes': meshes, 'textures': textures,
            'limitations': ['No Unity import/render comparison performed.',
                            'EXR header checked; pixel numerical equivalence not tested.',
                            'Texture previews are derived exports, not raw GPU bytes.',
                            'Whole capture currently exports draw/dispatch states, not copy/clear resource history.']}


if __name__ == '__main__':
    report = validate(sys.argv[1])
    text = json.dumps(report, ensure_ascii=False, indent=2, allow_nan=True)
    if len(sys.argv) > 2:
        Path(sys.argv[2]).write_text(text, encoding='utf-8')
    print(json.dumps({k: v for k, v in report.items() if k not in ('meshes', 'textures')}, indent=2))
    sys.exit(0 if report['passed'] else 1)
