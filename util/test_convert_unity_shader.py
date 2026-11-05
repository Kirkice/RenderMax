import unittest
from convert_unity_shader import convert, extract, pipeline_state

HEADER = '#version 320 es\n'
VS = HEADER + '''uniform vec4 matrix[4];
in highp vec4 in_POSITION0;
out highp vec2 uv;
void main() { uv = vec2(1.0); gl_Position = in_POSITION0; return; }
'''
PS = HEADER + '''uniform mediump sampler2D tex;
in highp vec2 uv;
layout(location = 0) out mediump vec4 SV_Target0;
void main() { SV_Target0 = texture(tex, uv); return; }
'''


class ConversionTests(unittest.TestCase):
    def test_color_write_mask(self):
        for bits, expected in ((15, 'RGBA'), (7, 'RGB'), (8, 'A'), (5, 'RB'), (0, '0')):
            state = pipeline_state({'colorBlends': [{'writeMask': bits}]})
            self.assertEqual(state[3], expected)
            shader = convert(VS, PS, 'Mask', state)
            self.assertIn('ColorMask ' + expected + '\n', shader)
        self.assertNotIn('ColorMask 15', convert(VS, PS, 'Default'))

    def test_pair(self):
        shader = convert(VS, PS, 'Test')
        self.assertIn('tex.Sample(samplertex, v.uv)', shader)
        self.assertIn('o.position = v.in_POSITION0', shader)
        self.assertIn('float4 matrix[4];', shader)
        self.assertIn('((float2)(1.0))', shader)

    def test_math_and_branch(self):
        body = 'float x; x = fract(sin(uv.x)) + sqrt(abs(cos(uv.y))); if (x > 0.0) { x = max(x, 0.1); } SV_Target0 = vec4(x); return;'
        shader = convert(VS, PS.replace('SV_Target0 = texture(tex, uv); return;', body), 'Math')
        self.assertIn('frac(sin(v.uv.x))', shader)
        self.assertIn('sqrt(abs(cos(v.uv.y)))', shader)
        self.assertIn('if (x > 0.0)', shader)
        self.assertIn('((float4)(x))', shader)

    def test_mod_not_silently_fmod(self):
        with self.assertRaisesRegex(ValueError, 'semantic'):
            convert(VS, PS.replace('texture(tex, uv)', 'vec4(mod(uv.x, 2.0))'), 'Mod')

    def test_interface_mismatch(self):
        with self.assertRaises(ValueError):
            convert(VS, PS.replace('vec2 uv', 'vec3 uv'), 'Test')

    def test_unknown_preprocessing(self):
        with self.assertRaises(ValueError):
            extract('#define UNKNOWN 1\n' + VS)

    def test_discard_and_no_uniform(self):
        shader = convert(VS, PS.replace('uniform mediump sampler2D tex;', '').replace('texture(tex, uv)', 'vec4(1.0)').replace('return;', 'discard; return;'), 'Discard')
        self.assertIn('discard;', shader)

    def test_nested_texture_and_lod(self):
        shader = convert(VS, PS.replace('texture(tex, uv)', 'textureLod(tex, vec2(uv.x, uv.y), 0.0)'), 'Lod')
        self.assertIn('tex.SampleLevel(samplertex, float2(v.uv.x, v.uv.y), 0.0)', shader)

    def test_fragment_coord(self):
        shader = convert(VS, PS.replace('texture(tex, uv)', 'gl_FragCoord'), 'Coord')
        self.assertIn('o.SV_Target0 = v.position', shader)

    def test_unsupported_control_flow(self):
        with self.assertRaises(ValueError):
            extract(PS.replace('return;', 'do { } while (true);'))


if __name__ == '__main__':
    unittest.main()
