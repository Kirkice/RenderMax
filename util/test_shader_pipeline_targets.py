"""Isolated dual-pipeline generation and independent HLSL compilation checks."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
sys.path.insert(0, 'H:/tmp/RenderMax/util')
spec = importlib.util.spec_from_file_location('capture_generator', HERE/'generate_capture_shaders.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)

class PipelineTargets(unittest.TestCase):
    def test_pipeline_tags(self):
        self.assertEqual(generator.shader_pipeline_tags('urp'), ('Tags { "RenderPipeline"="UniversalPipeline" }', 'SRPDefaultUnlit'))
        self.assertEqual(generator.shader_pipeline_tags('builtin'), ('', 'Always'))
        with self.assertRaises(ValueError): generator.shader_pipeline_tags('hdrp')

    def test_generation_and_compilation(self):
        with tempfile.TemporaryDirectory(prefix='RenderMaxDualPipeline-') as temporary:
            root = Path(temporary)
            vertex = '#version 310 es\nprecision highp float;\nlayout(location=0) in vec3 in_POSITION0;\nlayout(location=0) out vec2 vs_TEXCOORD0;\nvoid main(){gl_Position=vec4(in_POSITION0,1.0);vs_TEXCOORD0=in_POSITION0.xy;}\n'
            pixel = '#version 310 es\nprecision highp float;\nlayout(location=0) in vec2 vs_TEXCOORD0;\nlayout(location=0) out vec4 SV_Target0;\nuniform sampler2D _MainTex;\nvoid main(){SV_Target0=texture(_MainTex,vs_TEXCOORD0);}\n'
            documents = {
                'vertex.json': {'source':vertex}, 'pixel.json': {'source':pixel},
                'pipeline.json': {'rasterizer': {'cullMode':'Back'}, 'depthTest': {'depthEnable':True,'depthWrites':True,'depthFunction':'Less Equal'}, 'colorBlends':[{'writeMask':15,'enabled':False}]},
                'draw.json': {'eventId':104,'kind':'draw','shaders':{'Vertex':'vertex.json','Pixel':'pixel.json'},'pipeline':'pipeline.json'}
            }
            for name,data in documents.items(): (root/name).write_text(json.dumps(data),encoding='utf-8')
            manifest = root/'manifest.json'
            manifest.write_text(json.dumps({'draws':[{'eventId':104,'path':'draw.json'}]}),encoding='utf-8')
            for pipeline in ('urp','builtin'):
                with self.subTest(pipeline=pipeline):
                    output=root/pipeline
                    run=subprocess.run([sys.executable,str(HERE/'generate_capture_shaders.py'),str(manifest),str(output),'--pipeline',pipeline],capture_output=True,text=True)
                    self.assertEqual(run.returncode,0,run.stdout+run.stderr)
                    report=json.loads((output/'GenerationReport.json').read_text())
                    self.assertEqual(report['graphicsShaderCount'],1,report)
                    self.assertEqual(report['targetPipeline'],pipeline)
                    mapping=json.loads((output/'SceneShaderMap.json').read_text())
                    self.assertEqual(mapping['targetPipeline'],pipeline)
                    self.assertEqual(mapping['generatorVersion'],generator.GENERATOR_VERSION)
                    self.assertEqual(mapping['sourceManifestSHA256'],hashlib.sha256(manifest.read_bytes()).hexdigest())
                    shader=(output/'E0104.shader').read_text()
                    self.assertIn(f'/{pipeline}/E0104',shader)
                    self.assertIn('Cull Back',shader); self.assertIn('ZTest LEqual',shader); self.assertIn('ZWrite On',shader)
                    self.assertIn('_MainTex',shader)
                    if pipeline=='urp': self.assertIn('"RenderPipeline"="UniversalPipeline"',shader); self.assertIn('"LightMode"="SRPDefaultUnlit"',shader)
                    else: self.assertNotIn('UniversalPipeline',shader); self.assertIn('"LightMode"="Always"',shader)
                    hlsl=re.search(r'HLSLPROGRAM(.*?)ENDHLSL',shader,re.S)[1]
                    hlsl=re.sub(r'^#pragma.*$','',hlsl,flags=re.M)
                    code=output/'check.hlsl'; code.write_text(hlsl)
                    for entry,target in [('RMVertex','vs_6_0'),('RMFragment','ps_6_0')]:
                        compiled=subprocess.run([str(generator.SDK/'dxc.exe'),'-T',target,'-E',entry,str(code),'-Fo',str(output/(target+'.dxil'))],capture_output=True,text=True)
                        self.assertEqual(compiled.returncode,0,compiled.stdout+compiled.stderr)

if __name__=='__main__': unittest.main(verbosity=2)
