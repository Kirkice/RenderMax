import unittest
from prepare_unity_shaders import interfaces


class InterfaceTests(unittest.TestCase):
    def test_typed_bindings(self):
        data = interfaces('uniform highp vec4 color; uniform int mode; uniform mat4 view; uniform vec4 rows[4]; UNITY_LOCATION(0) uniform mediump sampler2D tex;')
        by_name = {v['name']: v for v in data}
        self.assertEqual(by_name['color']['propertyCandidate'], 'Vector')
        self.assertEqual(by_name['tex']['propertyCandidate'], '2D')
        self.assertTrue(by_name['mode']['requiresTypedBinding'])
        self.assertIsNone(by_name['rows']['propertyCandidate'])
        self.assertIsNone(by_name['view']['propertyCandidate'])

    def test_comments(self):
        self.assertEqual(interfaces('// uniform float fake;\n/* uniform int other; */ uniform float real;')[0]['name'], 'real')
        self.assertEqual(len(interfaces('// uniform float fake;')), 0)


if __name__ == '__main__':
    unittest.main()
