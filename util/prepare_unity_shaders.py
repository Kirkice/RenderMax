"""Build auditable Unity conversion jobs from a RenderMax asset package.
No shaders/materials are replaced. Source evidence remains authoritative.
Usage: python prepare_unity_shaders.py package/manifest.json output-directory
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

# Declaration extraction is deliberately conservative, not a GLSL compiler.
UNIFORM = re.compile(r'\buniform\s+(?:(?:lowp|mediump|highp)\s+)?(\w+)\s+(\w+)\s*(\[\s*\d+\s*\])?\s*;')


def interfaces(source):
    source = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
    result = []
    for typ, name, array in UNIFORM.findall(source):
        prop = None
        if not array:
            if typ in ('float', 'int', 'bool'):
                prop = 'Float'  # Unity 2019 Properties has no native integer type.
            elif typ in ('vec2', 'vec3', 'vec4'):
                prop = 'Vector'
            elif typ in ('sampler2D', 'samplerCube', 'sampler3D'):
                prop = {'sampler2D': '2D', 'samplerCube': 'Cube', 'sampler3D': '3D'}[typ]
        result.append(dict(name=name, sourceType=typ, array=array or None,
                           propertyCandidate=prop, bindingStatus='Unvalidated',
                           requiresTypedBinding=bool(array) or prop is None or typ in ('int', 'bool')))
    return result


def prepare(manifest_path, output):
    manifest_path = Path(manifest_path).resolve()
    root = manifest_path.parent
    manifest = json.loads(manifest_path.read_text(encoding='utf-8-sig'))
    if manifest.get('format') != 'rendermax.asset-package' or manifest.get('status') not in ('completed', 'completedWithWarnings'):
        raise ValueError('Expected completed RenderMax asset package')
    listed = {f['path']: f for f in manifest['files']}

    def read(path):
        full = (root / path).resolve()
        if full == root or root not in full.parents or path not in listed:
            raise ValueError('Invalid/unlisted evidence path: ' + path)
        data = full.read_bytes()
        record = listed[path]
        if len(data) != record['bytes'] or hashlib.sha256(data).hexdigest() != record['sha256']:
            raise ValueError('Evidence checksum mismatch: ' + path)
        return json.loads(data.decode('utf-8-sig'))

    output = Path(output).resolve()
    if output == root or root in output.parents:
        raise ValueError('Output must be outside the source evidence package')
    if output.exists() and any(output.iterdir()):
        raise ValueError('Output directory must be empty')
    output.mkdir(parents=True, exist_ok=True)
    stages, jobs = {}, {}
    for entry in manifest['draws']:
        draw = read(entry['path'])
        references = draw.get('shaders', {})
        key = hashlib.sha256(json.dumps(references, sort_keys=True).encode()).hexdigest()
        if not references:
            continue
        if key not in jobs:
            jobs[key] = dict(id=key, stages=references, draws=[], status='PendingConversion',
                             validation='NotCompiledOrGPUValidated')
        jobs[key]['draws'].append(dict(eventId=draw['eventId'], draw=entry['path'],
                                       constants=draw.get('constants'), bindings=draw.get('bindings'),
                                       pipeline=draw.get('pipeline')))
        for stage, path in references.items():
            if path in stages:
                continue
            shader = read(path)
            source = shader.get('source') or ''
            sid = hashlib.sha256(path.encode()).hexdigest()
            directory = output / 'stages' / sid
            directory.mkdir(parents=True)
            (directory / 'reflection.json').write_text(json.dumps(shader, indent=2), encoding='utf-8')
            (directory / 'source.txt').write_text(source, encoding='utf-8')
            name = shader.get('shaderName') or shader.get('name')
            stages[path] = dict(id=sid, stage=stage, name=name,
                                nameStatus='Confirmed' if name else 'UnrecoverableInThisPackage',
                                evidence=path, sourceAvailable=bool(source),
                                language='GLSL' if '#version' in source else 'Unknown',
                                interfaces=interfaces(source),
                                interfaceStatus='InferredConservativeExtraction',
                                reflection='stages/' + sid + '/reflection.json',
                                source='stages/' + sid + '/source.txt')
    inventory = dict(format='rendermax.unity-shader-jobs', schemaVersion='1.0',
                     sourceManifest=str(manifest_path), stages=list(stages.values()), jobs=list(jobs.values()),
                     limitations=['Declaration extraction is not preprocessing or full syntax parsing.',
                                  'Properties candidates are not verified GPU bindings.',
                                  'Matrix/array/struct and stage-specific uniform binding must preserve layout.',
                                  'Source identity does not establish computation or rendering equivalence.'])
    (output / 'ShaderList.json').write_text(json.dumps(inventory, indent=2), encoding='utf-8')
    return inventory


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest')
    parser.add_argument('output')
    args = parser.parse_args()
    inventory = prepare(args.manifest, args.output)
    print(f"Prepared {len(inventory['stages'])} stages, {len(inventory['jobs'])} shader combinations")
