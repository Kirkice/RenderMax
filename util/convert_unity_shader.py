"""Conservative GLSL subset adapter. Unsupported constructs fail closed.
Generated output is an unvalidated candidate, never proof of faithful rendering.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

DECL = re.compile(r'^(?:(UNITY_LOCATION\(\d+\)|layout\(location\s*=\s*\d+\))\s+)?(uniform|in|out)\s+(?:(?:highp|mediump|lowp)\s+)?(\w+)\s+(\w+)\s*(\[\s*\d+\s*\])?\s*;$', re.M)
TYPES = {'vec2':'float2', 'vec3':'float3', 'vec4':'float4', 'ivec2':'int2', 'ivec3':'int3', 'ivec4':'int4', 'uvec2':'uint2', 'uvec3':'uint3', 'uvec4':'uint4', 'mat2':'float2x2', 'mat3':'float3x3', 'mat4':'float4x4', 'float':'float', 'int':'int', 'bool':'bool', 'sampler2D':'sampler2D'}
TYPES.update(samplerCube='samplerCube', sampler2DShadow='sampler2DShadow', bvec2='bool2', bvec3='bool3', bvec4='bool4', uint='uint')
INTRINSICS = {'lessThan', 'greaterThan', 'greaterThanEqual', 'lessThanEqual', 'equal', 'notEqual', 'uint', 'bvec2', 'bvec3', 'bvec4', 'exp2', 'log2','dot','inversesqrt','texture','vec2','vec3','vec4','ivec2','ivec3','ivec4','uvec2','uvec3','uvec4','float','int','bool','min','max','clamp','abs','fract','sqrt','sin','cos','tan','pow','exp','log','floor','ceil','sign','mix','mod','normalize','length','distance','reflect','step','smoothstep','textureLod','texelFetch'}


def extract(source):
    # Only the known HLSLCC header may be removed without preprocessing.
    # This extension guard changes availability, not arithmetic. HLSL has explicit LOD support.
    source = re.sub(r'#ifdef GL_EXT_shader_texture_lod\s*\n#extension GL_EXT_shader_texture_lod : enable\s*\n#endif', '', source)
    first = re.search(r'^(?:uniform\b|in\b|out\b|layout\s*\(|UNITY_LOCATION\(\d+\)\s+uniform\b)', source, re.M)
    if not first:
        raise ValueError('No supported uniform section')
    header = source[:first.start()]
    allowed = ('#version 320 es', '#define HLSLCC_ENABLE_UNIFORM_BUFFERS 1', '#if HLSLCC_ENABLE_UNIFORM_BUFFERS', '#define UNITY_UNIFORM', '#else', '#define UNITY_UNIFORM uniform', '#endif', '#define UNITY_SUPPORTS_UNIFORM_LOCATION 1', '#if UNITY_SUPPORTS_UNIFORM_LOCATION', '#define UNITY_LOCATION(x) layout(location = x)', '#define UNITY_BINDING(x) layout(binding = x, std140)', '#define UNITY_LOCATION(x)', '#define UNITY_BINDING(x) layout(std140)', 'precision highp float;', 'precision highp int;')
    if any(line.strip() and line.strip() not in allowed for line in header.splitlines()):
        raise ValueError('Unsupported preprocessor header')
    source = source[first.start():]
    match = re.fullmatch(r'(.*?)void\s+main\s*\(\s*\)\s*\{(.*)\}\s*', source, re.S)
    if not match:
        raise ValueError('Expected single main function')
    declarations, body = match.groups()
    records = [(m.group(2), m.group(3), m.group(4), m.group(5) or '') for m in DECL.finditer(declarations)]
    locals_ = DECL.sub('', declarations)
    if any(typ not in TYPES for _,typ,_,_ in records):
        raise ValueError('Unsupported interface type')
    local_pattern = r'\s*(?:(?:highp|mediump|lowp)\s+)?(?:vec[234]|ivec[234]|uvec[234]|bvec[234]|mat[234]|float|int|uint|bool)\s+\w+(?:\s*\[\s*\d+\s*\])?\s*;'
    if re.sub(local_pattern, '', locals_).strip():
        raise ValueError('Unsupported global declaration')
    # Restrict this first adapter to arithmetic assignment code, not a general compiler.
    calls = set(re.findall(r'\b(\w+)\s*\(', body))
    if calls - INTRINSICS - {'if', 'for', 'while', 'switch'}:
        raise ValueError('Unsupported functions: ' + str(sorted(calls)))
    if re.search(r'\b(?:switch|do)\b|[#]', body):
        raise ValueError('Unsupported control flow')
    if body.count('{') != body.count('}'):
        raise ValueError('Unbalanced control-flow braces')
    return records, locals_, body


def comparison_calls(text):
    operators = {'lessThan':'<', 'greaterThan':'>', 'lessThanEqual':'<=', 'greaterThanEqual':'>=', 'equal':'==', 'notEqual':'!='}
    pattern = re.compile(r'\b(' + '|'.join(operators) + r')\s*\(')
    while True:
        matches = list(pattern.finditer(text))
        if not matches:
            return text
        m = matches[-1]; depth = 1; mark = m.end(); args = []
        for end in range(mark, len(text)):
            c = text[end]
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if not depth:
                    args.append(text[mark:end]); break
            elif c == ',' and depth == 1:
                args.append(text[mark:end]); mark = end + 1
        if depth or len(args) != 2:
            raise ValueError('Malformed vector comparison')
        text = text[:m.start()] + f'(({args[0]}) {operators[m[1]]} ({args[1]}))' + text[end+1:]


def texture_calls(text, uniforms):
    # Balanced argument scanner: nested constructors/calls cannot be parsed with [^()].
    pattern = re.compile(r'\b(texture|textureLod|texelFetch)\s*\(')
    while True:
        matches = list(pattern.finditer(text))
        if not matches:
            return text
        m = matches[-1]
        start = m.end(); depth = 1; args = []; mark = start; end = start
        for end in range(start, len(text)):
            c = text[end]
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    args.append(text[mark:end].strip()); break
            elif c == ',' and depth == 1:
                args.append(text[mark:end].strip()); mark = end + 1
        if depth:
            raise ValueError('Unbalanced texture arguments')
        name = args[0]
        if name not in uniforms or not uniforms[name][0].startswith('sampler'):
            raise ValueError('Texture expression must use a reflected sampler uniform')
        typ = uniforms[name][0]; fn = m[1]
        if typ == 'sampler2DShadow':
            # Capture shadow coordinate is vec3(uv, comparison depth).
            if not ((fn == 'textureLod' and len(args) == 3 and args[2] in ('0', '0.0')) or (fn == 'texture' and len(args) == 2)):
                raise ValueError('Unsupported shadow texture operation')
            expr = f'{name}.SampleCmpLevelZero(sampler{name}, ({args[1]}).xy, ({args[1]}).z)'
        elif fn == 'texelFetch' and typ == 'sampler2D' and len(args) == 3:
            expr = f'{name}.Load(int3({args[1]}, {args[2]}))'
        elif fn == 'textureLod' and len(args) == 3:
            expr = f'{name}.SampleLevel(sampler{name}, {args[1]}, {args[2]})'
        elif fn == 'texture' and len(args) == 2:
            expr = f'{name}.Sample(sampler{name}, {args[1]})'
        elif fn == 'texture' and len(args) == 3:
            expr = f'{name}.SampleBias(sampler{name}, {args[1]}, {args[2]})'
        else:
            raise ValueError('Unsupported texture operation: ' + fn)
        text = text[:m.start()] + expr + text[end+1:]


def pipeline_state(draw_pipeline):
    raster = draw_pipeline.get('rasterizer') or {}
    depth = draw_pipeline.get('depthTest') or {}
    blends = draw_pipeline.get('colorBlends') or []
    blend = blends[0] if blends else {}
    cull = {'No Cull': 'Off', 'None': 'Off', 'Back': 'Back', 'Front': 'Front'}.get(raster.get('cullMode'), 'Off')
    ztest = {'Never': 'Never', 'Less': 'Less', 'Equal': 'Equal', 'Less Equal': 'LEqual', 'Greater': 'Greater', 'Not Equal': 'NotEqual', 'Greater Equal': 'GEqual', 'Always': 'Always'}.get(depth.get('depthFunction'), 'Always') if depth.get('depthEnable') else 'Always'
    zwrite = 'On' if depth.get('depthEnable') and depth.get('depthWrites') else 'Off'
    mask_bits = int(blend.get('writeMask', 15))
    if mask_bits < 0 or mask_bits > 15:
        raise ValueError('Unsupported color write mask: ' + str(mask_bits))
    mask = ''.join(channel for bit, channel in ((1, 'R'), (2, 'G'), (4, 'B'), (8, 'A')) if mask_bits & bit) or '0'
    if blend.get('logicOperationEnabled'):
        raise ValueError('Logic operations require explicit backend support')
    factors = {'Zero':'Zero', 'One':'One', 'Src Color':'SrcColor', '1 - Src Color':'OneMinusSrcColor', 'Dst Color':'DstColor', '1 - Dst Color':'OneMinusDstColor', 'Src Alpha':'SrcAlpha', '1 - Src Alpha':'OneMinusSrcAlpha', 'Dst Alpha':'DstAlpha', '1 - Dst Alpha':'OneMinusDstAlpha', 'Src Alpha Saturate':'SrcAlphaSaturate'}
    operations = {'Add':'Add', 'Subtract':'Sub', 'Reverse Subtract':'RevSub', 'Minimum':'Min', 'Maximum':'Max'}
    blend_line = 'Blend Off'
    if blend.get('enabled'):
        try:
            c, a = blend['colorBlend'], blend['alphaBlend']
            blend_line = f"Blend {factors[c['source']]} {factors[c['destination']]}, {factors[a['source']]} {factors[a['destination']]}\nBlendOp {operations[c['operation']]}, {operations[a['operation']]}"
        except KeyError as error:
            raise ValueError('Unsupported blend factor/operation: ' + str(error)) from error
    return cull, ztest, zwrite, mask, blend_line


def convert(vertex, pixel, name, state=None):
    vs, vl, vb = extract(vertex)
    ps, pl, pb = extract(pixel)
    uniforms = {}
    for stage, records in [('Vertex',vs), ('Pixel',ps)]:
        for direction, typ, var, array in records:
            if direction == 'uniform':
                value = (typ, array)
                if var in uniforms and uniforms[var] != value:
                    raise ValueError('Conflicting stage uniform: ' + var)
                uniforms[var] = value
    vin = [(t,n) for d,t,n,a in vs if d == 'in']
    vout = [(t,n) for d,t,n,a in vs if d == 'out']
    pin = [(t,n) for d,t,n,a in ps if d == 'in']
    pout = [(t,n) for d,t,n,a in ps if d == 'out']
    if not pout or any(t != 'vec4' or not re.fullmatch(r'SV_(?:Target|TARGET)[0-7]', n) for t,n in pout):
        raise ValueError('Unsupported fragment output semantic')
    if any((t,n) not in vout for t,n in pin):
        raise ValueError('VS/FS interface mismatch')
    semantics = {}
    for typ,var in vin:
        m = re.fullmatch(r'in_(POSITION|NORMAL|TANGENT|TEXCOORD|COLOR)(\d+)',var)
        if not m:
            raise ValueError('Unsupported input semantic: ' + var)
        semantics[var] = m.group(1) + m.group(2)
    def adapt(text):
        text = re.sub(r'\b(highp|mediump|lowp)\s+', '', text)
        text = comparison_calls(text)
        text = texture_calls(text, uniforms)
        # GLSL scalar splats need a cast in HLSL (float3(x) is not portable).
        text = re.sub(r'\b([iu]?vec)([234])\(\s*([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*|[-+]?[0-9.]+)\s*\)', lambda m: '((' + {'vec':'float','ivec':'int','uvec':'uint'}[m[1]] + m[2] + ')(' + m[3] + '))', text)
        if re.search(r'\b(?:mod|textureLod|texelFetch)\s*\(', text):
            raise ValueError('Unsupported semantic texture/mod expression')
        for old,new in dict(TYPES, inversesqrt='rsqrt', fract='frac', mix='lerp').items():
            text = re.sub(r'\b'+old+r'\b',new,text)
        return text
    def function(records, local, body, vertex_stage):
        body = re.sub(r'\breturn\s*;', 'return o;', body)
        for direction,typ,var,array in records:
            if direction in ('in','out'):
                body = re.sub(r'\b'+re.escape(var)+r'\b',('v.' if direction=='in' else 'o.')+var,body)
        if not vertex_stage:
            # SV_POSITION pixel-stage w is reciprocal clip w, as in gl_FragCoord.
            # Cross-backend viewport origin/depth still requires GPU validation.
            body = re.sub(r'\bgl_FragCoord\b', 'v.position', body)
        if vertex_stage:
            body = re.sub(r'\bgl_Position\b','o.position',body)
            # Explicit OpenGL -> D3D clip-depth mapping. GPU validation still required.
            body = body.replace('return o;', 'o.position.z = o.position.z * _RMClipDepth.x + o.position.w * _RMClipDepth.y; return o;')
        return adapt(local + '\n' + body)
    props = ['_RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)',
             '_RMCull ("Cull", Float) = 0', '_RMZTest ("Depth test", Float) = 8',
             '_RMZWrite ("Depth write", Float) = 0', '_RMColorMask ("Color mask", Float) = 15']
    for var,(typ,array) in uniforms.items():
        if not array and typ in ('float','int','bool'):
            props.append(f'{var} ("{var}", Float) = 0')
        elif not array and typ in ('vec2','vec3','vec4'):
            props.append(f'{var} ("{var}", Vector) = (0,0,0,0)')
        elif typ in ('sampler2D', 'samplerCube', 'sampler2DShadow') and not array:
            dimension = 'Cube' if typ == 'samplerCube' else '2D'
            props.append(f'{var} ("{var}", {dimension}) = "white" {{}}')
    fields = lambda items: '\n'.join(f'{TYPES[t]} {n} : {s};' for t,n,s in items)
    declarations = '\n'.join((f"{'TextureCube' if t == 'samplerCube' else 'Texture2D'} {n}; {'SamplerComparisonState' if t == 'sampler2DShadow' else 'SamplerState'} sampler{n};" if t.startswith('sampler') and not a else f'{TYPES[t]} {n}{a};') for n,(t,a) in uniforms.items())
    cull, ztest, zwrite, mask, blend_line = state or ('Off', 'Always', 'Off', 'RGBA', 'Blend Off')
    return f'''// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/{name}" {{
Properties {{ {chr(10).join(props)} }}
SubShader {{ Pass {{
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull {cull}
ZTest {ztest}
ZWrite {zwrite}
{blend_line}
ColorMask {mask}
HLSLPROGRAM
#pragma target 3.5
#pragma prefer_hlslcc gles
#pragma vertex RMVertex
#pragma fragment RMFragment
#include "HLSLSupport.cginc"
{declarations}
float4 _RMClipDepth;
struct RMInput {{ {fields([(t,n,semantics[n]) for t,n in vin])} }};
struct RMVaryings {{ float4 position : SV_POSITION;
{fields([(t,n,'TEXCOORD'+str(i)) for i,(t,n) in enumerate(vout)])} }};
struct RMColor {{ {fields([(t,n,n) for t,n in pout])} }};
RMVaryings RMVertex(RMInput v) {{ RMVaryings o = (RMVaryings)0;
{function(vs,vl,vb,True)}
}}
RMColor RMFragment(RMVaryings v) {{ RMColor o = (RMColor)0;
{function(ps,pl,pb,False)}
}}
ENDHLSL
}} }} }}
'''


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('jobs')
    parser.add_argument('output')
    args = parser.parse_args()
    root = Path(args.jobs).resolve().parent
    inventory = json.loads(Path(args.jobs).read_text(encoding='utf-8'))
    stages = {s['evidence']:s for s in inventory['stages']}
    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    reports = []
    for job in inventory['jobs']:
        report = dict(id=job['id'], eventIds=[d['eventId'] for d in job['draws']], status='Unsupported')
        try:
            v = stages[job['stages']['Vertex']]
            p = stages[job['stages']['Pixel']]
            manifest_root = Path(inventory['sourceManifest']).resolve().parent
            variants = {}
            for draw in job['draws']:
                pipeline = json.loads((manifest_root / draw['pipeline']).read_text(encoding='utf-8'))
                key = json.dumps({k: pipeline.get(k) for k in ('rasterizer', 'depthTest', 'colorBlends')}, sort_keys=True)
                variant_id = hashlib.sha256(key.encode('utf-8')).hexdigest()[:16]
                if variant_id not in variants:
                    variants[variant_id] = dict(id=variant_id, eventIds=[], state=pipeline_state(pipeline), sourcePipeline=draw['pipeline'])
                variants[variant_id]['eventIds'].append(draw['eventId'])
            directory = out/job['id']
            directory.mkdir(exist_ok=True)
            report['variants'] = []
            for variant in variants.values():
                cull, _, _, _, blend = variant['state']
                label = 'E' + str(min(variant['eventIds'])).zfill(4) + '_' + cull + '_' + ('Opaque' if blend == 'Blend Off' else 'Blended')
                relative = label + '.shader'
                shader = convert((root/v['source']).read_text(encoding='utf-8'), (root/p['source']).read_text(encoding='utf-8'), label, variant['state'])
                shader = '// Associated EIDs: ' + ', '.join(map(str, variant['eventIds'])) + '\n// Original shader name: unavailable in exported evidence.\n' + shader
                (directory/relative).write_text(shader, encoding='utf-8')
                report['variants'].append(dict(id=variant['id'], displayName=label, shader=job['id']+'/'+relative, eventIds=variant['eventIds'], sourcePipeline=variant['sourcePipeline']))
            # Compatibility sample only; per-Draw consumers MUST use variants[].eventIds.
            (directory/'Faithful.shader').write_text(shader, encoding='utf-8')
            report.update(status='GeneratedCandidate', validation='NotCompiledOrGPUValidated', pipeline='PerDrawStateVariantsGenerated_WindingAndViewportUnvalidated', constants='BindingRequired', provenance='Inferred')
        except (ValueError, KeyError) as error:
            report['reason'] = str(error)
        reports.append(report)
    (out/'ConversionReport.json').write_text(json.dumps(reports,indent=2),encoding='utf-8')
    print('Generated',sum(r['status']=='GeneratedCandidate' for r in reports),'of',len(reports),'combinations')
