// E63 editable-scene candidate, not faithful GPU replay.
// Fragment sampling/ST calculation follows the captured source.
// Vertex clip position uses Unity's live object/view/projection matrices.
// Captured normal/tangent/world-position calculations were removed because
// the E63 fragment stage does not consume them. No captured engine-named
// uniforms are declared: Unity's GLSL translator reserves those names.
Shader "RenderMax/Phase2/Event63_EditableCandidate"
{
    Properties
    {
        _RMCull ("Cull", Float) = 2
        _RMZTest ("Depth test", Float) = 4
        _RMZWrite ("Depth write", Float) = 1
        _RMColorMask ("Color mask", Float) = 15
        _AlbedoMap ("Captured Albedo", 2D) = "white" {}
        _AlbedoMap_ST ("Captured Albedo ST", Vector) = (1,1,0,0)
    }
    SubShader
    {
        Pass
        {
            // Captured E63 Pipeline State:
            // Rasterizer: Solid, Back-face culling, FrontCCW=true.
            // FrameBuffer Output: blending and logic operation disabled, RGBA write mask.
            // Depth: LessEqual, depth test enabled, depth writes enabled.
            Cull Back
            ZTest LEqual
            ZWrite On
            Blend Off
            ColorMask RGBA
            HLSLPROGRAM
            #pragma target 3.5
            #pragma vertex RMVertex
            #pragma fragment RMFragment
            #include "UnityCG.cginc"
            Texture2D _AlbedoMap;
            SamplerState sampler_AlbedoMap;
            float4 _AlbedoMap_ST;
            struct RMInput
            {
                float4 position : POSITION;
                float2 uv : TEXCOORD0;
            };
            struct RMVaryings
            {
                float4 position : SV_POSITION;
                float2 uv : TEXCOORD0;
            };
            RMVaryings RMVertex(RMInput v)
            {
                RMVaryings o;
                o.position = UnityObjectToClipPos(v.position);
                o.uv = v.uv;
                return o;
            }
            float4 RMFragment(RMVaryings v) : SV_Target
            {
                float2 uv = v.uv * _AlbedoMap_ST.xy + _AlbedoMap_ST.zw;
                return _AlbedoMap.Sample(sampler_AlbedoMap, uv);
            }
            ENDHLSL
        }
    }
}
