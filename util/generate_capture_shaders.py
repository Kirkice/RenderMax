"""Generate auditable Unity shader candidates using glslang and SPIRV-Cross.
Does not modify evidence, materials, scenes, or the old package shader map.
Generated candidates are NOT a faithful full-frame replay implementation.
"""
import argparse
import concurrent.futures
import hashlib
import json
import re
import subprocess
from pathlib import Path

SDK = Path('C:/VulkanSDK/1.4.313.2/Bin')

def run(args):
    p = subprocess.run([str(x) for x in args], capture_output=True, text=True, encoding='utf-8', errors='replace')
    if p.returncode:
        raise RuntimeError(p.stdout + p.stderr)
    return p.stdout

def captured(n):
    return '_RMCaptured_' + n.lstrip('_') if n.startswith(('hlslcc_', 'unity_')) or n in ('_Time','_WorldSpaceCameraPos','_ProjectionParams','_ScreenParams','_ZBufferParams') else n

def declarations(source):
    names = set(re.findall(r'\b(?:float|int|uint|bool|[biu]?vec[234]|mat[234]|sampler\w+)\s+(\w+)\s*(?:\[\s*\d+\s*\])?\s*;', source))
    return names

def stage_convert(entry, evidence, work):
    path, kind = entry
    info = json.loads((evidence/path).read_text(encoding='utf-8-sig'))
    source = info.get('source')
    if not source:
        raise ValueError('No captured GLSL source: '+path)
    folder = work/hashlib.sha256(path.encode()).hexdigest()[:20]
    folder.mkdir(parents=True, exist_ok=True)
    ext = {'Vertex':'vert','Pixel':'frag','Compute':'comp'}[kind]
    # glslang OpenGL SPIR-V requires ESSL 310+. The 300 sources use only compatible constructs.
    source = re.sub(r'#version\s+300\s+es', '#version 310 es', source)
    src=folder/('source.'+ext); src.write_text(source,encoding='utf-8')
    spv=folder/'stage.spv'; hlsl=folder/'stage.hlsl'
    run([SDK/'glslangValidator.exe','-G','--auto-map-bindings','--auto-map-locations',src,'-o',spv])
    run([SDK/'spirv-cross.exe',spv,'--hlsl','--shader-model','50','--output',hlsl])
    text=hlsl.read_text(encoding='utf-8')
    return dict(kind=kind,path=path,source=source,hlsl=text,folder=str(folder))

def adapt_stage(stage, prefix):
    text=stage['hlsl']; originals=declarations(stage['source']); uniforms={}; remap={}
    def block(m):
        for typ,name,array in re.findall(r'\b((?:float|int|uint|bool)(?:[234](?:x[234])?)?)\s+(\w+)\s*(\[\d+\])?\s*:\s*packoffset\([^)]*\)\s*;',m[1]):
            suffix=re.sub(r'^_\d+_', '', name)
            choices=[n for n in originals if n.lstrip('_')==suffix.lstrip('_')]
            if len(choices)!=1: raise ValueError('Ambiguous captured uniform '+name)
            original=choices[0]; target=captured(original)
            remap[name]=target;uniforms[target]=(typ,array or '')
        return ''
    text=re.sub(r'cbuffer\s+\w+\s*:\s*register\([^)]*\)\s*\{(.*?)\};',block,text,flags=re.S)
    def uniform(m):
        typ,name,array=m.groups(); target=captured(name);remap[name]=target;uniforms[target]=(typ,array or '');return ''
    text=re.sub(r'^uniform\s+(\w+)\s+(\w+)\s*(\[\d+\])?\s*;',uniform,text,flags=re.M)
    # Resources retain their reflected names for the Unity material binder.
    resources=[]
    def resource(m):
        resources.append(m[1]);return ''
    text=re.sub(r'^((?:RW)?(?:Texture\w+(?:<[^>]+>)?|Buffer<[^>]+>|RWBuffer<[^>]+>|StructuredBuffer<[^>]+>|RWStructuredBuffer<[^>]+>|SamplerState|SamplerComparisonState)\s+\w+(?:\[\d+\])?\s*:\s*register\([^)]*\)\s*;)',resource,text,flags=re.M)
    # Global temporaries/functions/structs are stage-local; avoid VS/PS collisions.
    local=set(re.findall(r'^static\s+(?:const\s+)?\w+(?:<[^>]+>)?\s+(\w+)',text,re.M))
    local.update(re.findall(r'^struct\s+(\w+)',text,re.M))
    local.update(re.findall(r'^(?:void|\w+)\s+(\w+)\s*\([^;]*?\)\s*\{',text,re.M))
    for n in local:remap[n]=prefix+n
    if remap:
        pattern=r'\b(?:'+ '|'.join(re.escape(x) for x in sorted(remap,key=len,reverse=True))+r')\b'
        text=re.sub(pattern,lambda m:remap[m[0]],text)
    # A captured variable may itself be named SV_Target: never rename HLSL semantics.
    text=re.sub(r'(:\s*)'+prefix+r'(SV_\w+)', r'\1\2', text)
    return text,uniforms,resources

def interface(text,struct):
    m=re.search(r'\bstruct\s+'+struct+r'\s*\{(.*?)\};',text,re.S)
    if not m:return []
    return re.findall(r'(?:nointerpolation\s+|precise\s+|centroid\s+|noperspective\s+)*(\w+)\s+(\w+)\s*:\s*(\w+)\s*;',m[1])

GENERATOR_VERSION = 2

def shader_pipeline_tags(target_pipeline):
    if target_pipeline == 'urp':
        return 'Tags { "RenderPipeline"="UniversalPipeline" }', 'SRPDefaultUnlit'
    if target_pipeline == 'builtin':
        return '', 'Always'
    raise ValueError('Unsupported target pipeline: ' + target_pipeline)

def create_graphics(vs,ps,label,state,target_pipeline='urp',capture_namespace='Capture'):
    v,vu,vr=adapt_stage(vs,'RMVS_');p,pu,pr=adapt_stage(ps,'RMPS_')
    uniforms=dict(vu)
    for n,t in pu.items():
        if n in uniforms and uniforms[n]!=t:raise ValueError('VS/PS uniform type mismatch: '+n)
        uniforms[n]=t
    vi=interface(v,'RMVS_SPIRV_Cross_Input');vo=interface(v,'RMVS_SPIRV_Cross_Output');pi=interface(p,'RMPS_SPIRV_Cross_Input')
    # Match varyings by name, not independently auto-assigned stage locations.
    semantics={n.removeprefix('RMVS_'):'TEXCOORD'+str(i) for i,(t,n,s) in enumerate(x for x in vo if not x[2].upper().startswith('SV_'))}
    for t,n,s in pi:
        if not s.upper().startswith('SV_') and n.removeprefix('RMPS_') not in semantics: raise ValueError('Fragment input not written by VS: '+n)
    def fix_struct(text,name,vertex_input=False):
        def replace(m):
            body=m[1]
            def field(f):
                start,n,s=f.groups()
                if vertex_input:
                    x=re.fullmatch(r'in_(POSITION|NORMAL|TANGENT|TEXCOORD|COLOR)(\d+)',n.removeprefix('RMVS_'))
                    if x:s=x[1]+x[2]
                    elif n.removeprefix('RMVS_') == 'vertex':s='POSITION0'
                    elif not s.upper().startswith('SV_'):raise ValueError('Unsupported mesh semantic '+n)
                elif n.removeprefix('RMVS_').removeprefix('RMPS_') in semantics:s=semantics[n.removeprefix('RMVS_').removeprefix('RMPS_')]
                return start+n+' : '+s+';'
            body=re.sub(r'(\b(?:\w+\s+)+)(\w+)\s*:\s*(\w+)\s*;',field,body)
            return 'struct '+name+'\n{'+body+'};'
        return re.sub(r'\bstruct\s+'+name+r'\s*\{(.*?)\};',replace,text,flags=re.S)
    v=fix_struct(v,'RMVS_SPIRV_Cross_Input',True);v=fix_struct(v,'RMVS_SPIRV_Cross_Output');p=fix_struct(p,'RMPS_SPIRV_Cross_Input')
    props=['_RMUseLivePosition ("Use live Unity vertex positions (not exact capture replay)", Float) = 1']
    for n,(t,a) in sorted(uniforms.items()):
        if a:continue
        if t in ('float','int','uint','bool'):props.append(f'{n} ("{n}", Float) = 0')
        elif re.fullmatch(r'(?:float|int|uint)[234]',t):props.append(f'{n} ("{n}", Vector) = (0,0,0,0)')
    resources={}
    for line in vr+pr:
        line=re.sub(r'\s*:\s*register\([^)]*\)', '', line)
        m=re.match(r'(\w+(?:<[^>]+>)?)\s+(\w+)',line)
        if not m:raise ValueError('Unrecognized resource: '+line)
        t,n=m.groups()
        if n in resources and resources[n]!=line:raise ValueError('VS/PS resource type mismatch: '+n)
        resources[n]=line
        if t.startswith('Texture'):
            dimension='Cube' if 'Cube' in t else '2DArray' if '2DArray' in t else '3D' if '3D' in t else '2D'
            prop=f'{n} ("{n}", {dimension}) = "" {{}}'
            if prop not in props:props.append(prop)
    u='\n'.join(f'{t} {n}{a};' for n,(t,a) in sorted(uniforms.items()))
    # Match the existing package behavior: live scene clip positions, captured shading constants.
    pos=next(((t,n) for t,n,s in vi if 'in_POSITION' in n),None)
    output_pos=next((n for t,n,s in vo if s.upper()=='SV_POSITION'),None)
    if output_pos is None:raise ValueError('No clip output')
    # Procedural fullscreen passes intentionally have SV_VertexID instead of mesh POSITION.
    live=f'if (_RMUseLivePosition > 0.5) o.{output_pos} = mul(unity_MatrixVP, mul(unity_ObjectToWorld, float4(v.{pos[1]}.xyz, 1.0)));' if pos else ''
    # Preserve captured OpenGL positions; live-position mode uses Unity matrices.
    # Captured clip-space conversion on other graphics backends needs separate validation.
    wrapper=f'RMVS_SPIRV_Cross_Output RMVertex(RMVS_SPIRV_Cross_Input v) {{ RMVS_SPIRV_Cross_Output o = RMVS_main(v); {live} return o; }}'
    pm=re.search(r'(\w+)\s+RMPS_main\s*\(([^)]*)\)',p)
    if not pm:raise ValueError('Missing fragment entry')
    wrapper+='\n'+pm[1]+' RMFragment('+pm[2]+') { return RMPS_main('+(', '.join(x.strip().split()[-1] for x in pm[2].split(',')) if pm[2].strip() else '')+'); }'
    from convert_unity_shader import pipeline_state
    cull,ztest,zwrite,mask,blend=pipeline_state(state)
    source='// GENERATED from captured GLSL through glslang/SPIRV-Cross; NOT pixel-equivalence validated.\n'
    subshader_tags, light_mode = shader_pipeline_tags(target_pipeline)
    source+=f'// Target pipeline: {target_pipeline}; generator version: {GENERATOR_VERSION}\n'
    source+=f'Shader "RenderMax/Phase2/Editable/{capture_namespace}/{target_pipeline}/{label}" {{\nProperties {{\n'+ '\n'.join(props)+f'\n}}\nSubShader {{ {subshader_tags} Pass {{\n'
    source+=f'Tags {{ "LightMode"="{light_mode}" }}\nCull {cull}\nZTest {ztest}\nZWrite {zwrite}\n{blend}\nColorMask {mask}\n'
    source+='HLSLPROGRAM\n#pragma target 5.0\n#pragma vertex RMVertex\n#pragma fragment RMFragment\nfloat4x4 unity_ObjectToWorld;\nfloat4x4 unity_MatrixVP;\nfloat _RMUseLivePosition;\n'+u+'\n'+'\n'.join(resources.values())+'\n'+v+'\n'+p+'\n'+wrapper+'\nENDHLSL\n} } }\n'
    return source

def main():
    parser=argparse.ArgumentParser();parser.add_argument('manifest');parser.add_argument('output');parser.add_argument('--update',action='store_true');parser.add_argument('--pipeline',choices=['urp','builtin'],default='urp');args=parser.parse_args()
    evidence=Path(args.manifest).resolve().parent;out=Path(args.output).resolve()
    if out.exists() and any(out.iterdir()) and not (args.update and (out/'GenerationReport.json').is_file()):raise ValueError('Output must be new/empty, or use --update for this generator output')
    out.mkdir(parents=True,exist_ok=True);work=out/'CompilerEvidence~';work.mkdir(exist_ok=True)
    manifest_hash=hashlib.sha256(Path(args.manifest).read_bytes()).hexdigest()
    capture_namespace='Capture_'+manifest_hash[:12]
    manifest=json.loads(Path(args.manifest).read_text(encoding='utf-8-sig'));draws=[json.loads((evidence/d['path']).read_text(encoding='utf-8-sig')) for d in manifest['draws']]
    stages={p:k for d in draws for k,p in d.get('shaders',{}).items()};converted={};errors={}
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        futures={pool.submit(stage_convert,(p,k),evidence,work):p for p,k in stages.items()}
        for f,p in futures.items():
            try:converted[p]=f.result()
            except Exception as e:errors[p]=str(e)
    print('Stage compilation:',len(converted),'of',len(stages),flush=True)
    reports=[];entries=[];cache={}
    for d in draws:
        eid=d['eventId'];report={'eventId':eid,'kind':d.get('kind'),'status':'Unsupported'}
        try:
            refs=d.get('shaders',{})
            for path in refs.values():
                if path in errors:raise ValueError(errors[path])
            if 'Compute' in refs:
                stage=converted[refs['Compute']];text,u,r=adapt_stage(stage,'RMCS_')
                # Keep storage buffer bindings as generated: no automatic resource allocation or dispatch.
                text='#pragma kernel RMCS_main\n// Candidate only: captured buffers/images must be allocated and bound before dispatch.\n'+'\n'.join(f'{t} {n}{a};' for n,(t,a) in u.items())+'\n'+'\n'.join(r)+'\n'+text
                file=f'E{eid:04d}.compute';(out/file).write_text(text,encoding='utf-8');report.update(status='GeneratedComputeCandidate',shader=file)
            else:
                pipeline=json.loads((evidence/d['pipeline']).read_text(encoding='utf-8-sig'))
                state={k:pipeline.get(k) for k in ('rasterizer','depthTest','colorBlends')}
                key=json.dumps({'refs':refs,'state':state},sort_keys=True)
                if key not in cache:
                    label=f'E{eid:04d}';source=create_graphics(converted[refs['Vertex']],converted[refs['Pixel']],label,pipeline,args.pipeline,capture_namespace)
                    (out/(label+'.shader')).write_text(source,encoding='utf-8');cache[key]={'shader':label,'eventIds':[]};entries.append(cache[key])
                cache[key]['eventIds'].append(eid);report.update(status='GeneratedGraphicsCandidate',shader=cache[key]['shader']+'.shader')
        except Exception as e:report['reason']=str(e)
        reports.append(report)
    (out/'SceneShaderMap.json').write_text(json.dumps({'entries':entries,'sourceManifestSHA256':manifest_hash,'targetPipeline':args.pipeline,'generatorVersion':GENERATOR_VERSION,'unsupportedEventIds':[r['eventId'] for r in reports if r['status']=='Unsupported'],'nonGraphicsEventIds':[r['eventId'] for r in reports if r['kind']=='dispatch']},indent=2),encoding='utf-8')
    (out/'GenerationReport.json').write_text(json.dumps({'targetPipeline':args.pipeline,'generatorVersion':GENERATOR_VERSION,'stageCount':len(stages),'stageErrors':errors,'graphicsShaderCount':len(entries),'events':reports,'limitations':['Generated candidates are not GPU/pixel-equivalence validated.','Live Unity clip positions are used by default; shading uses captured constants.','Compute shaders are generated only; storage resources and dispatch scheduling are not implemented.','Viewport, stencil, MRT targets, samplers and frame dependencies require validation.']},indent=2),encoding='utf-8')
    from collections import Counter
    print('Events:',dict(Counter(r['status'] for r in reports)),'graphics shader files:',len(entries),flush=True)

if __name__=='__main__':
    import sys
    sys.path.insert(0,'H:/tmp/RenderMax/util')
    main()
