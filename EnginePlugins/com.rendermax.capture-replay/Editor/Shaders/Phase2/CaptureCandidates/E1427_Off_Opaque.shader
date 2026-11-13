// Associated EIDs: 1427, 1452, 1476, 1500, 1524, 1548, 1572
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E1427_Off_Opaque" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_BloomSrcTex_TexelSize ("_BloomSrcTex_TexelSize", Vector) = (0,0,0,0)
_Params ("_Params", Vector) = (0,0,0,0)
_BloomSrcUvScale ("_BloomSrcUvScale", Vector) = (0,0,0,0)
_BloomSrcTex ("_BloomSrcTex", 2D) = "white" {} }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Off
ZTest Always
ZWrite Off
Blend Off
ColorMask RGBA
HLSLPROGRAM
#pragma target 3.5
#pragma prefer_hlslcc gles
#pragma vertex RMVertex
#pragma fragment RMFragment
#include "UnityCG.cginc"
float4 _BloomSrcTex_TexelSize;
float4 _Params;
float2 _BloomSrcUvScale;
Texture2D _BloomSrcTex; SamplerState sampler_BloomSrcTex;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float2 in_TEXCOORD0 : TEXCOORD0; };
struct RMVaryings { float4 position : SV_POSITION;
float2 vs_TEXCOORD0 : TEXCOORD0; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;





    o.position.xyw = v.in_POSITION0.xyw;
    o.position.z = 1.0;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;






float4 u_xlat16_0;
float4 u_xlat16_1;
float3 u_xlat16_2;
float3 u_xlat16_3;
float u_xlat16_4;
float3 u_xlat16_5;
float3 u_xlat16_6;


    u_xlat16_0 = _BloomSrcTex_TexelSize.xyxy * _BloomSrcUvScale.xyxy;
    u_xlat16_1 = u_xlat16_0.xyzy * float4(-1.0, -1.0, 1.0, -1.0) + v.vs_TEXCOORD0.xyxy;
    u_xlat16_0 = u_xlat16_0.xwzw * float4(-1.0, 1.0, 1.0, 1.0) + v.vs_TEXCOORD0.xyxy;
    u_xlat16_2.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_1.zw, 0.0).xyz;
    u_xlat16_3.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_1.xy, 0.0).xyz;
    u_xlat16_1.x = dot(u_xlat16_2.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_1.x = u_xlat16_1.x + _Params.y;
    u_xlat16_1.x = float(1.0) / u_xlat16_1.x;
    u_xlat16_6.xyz = u_xlat16_1.xxx * u_xlat16_2.xyz;
    u_xlat16_4 = dot(u_xlat16_3.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_4 = u_xlat16_4 + _Params.y;
    u_xlat16_4 = float(1.0) / u_xlat16_4;
    u_xlat16_6.xyz = u_xlat16_3.xyz * ((float3)(u_xlat16_4)) + u_xlat16_6.xyz;
    u_xlat16_1.x = u_xlat16_1.x + u_xlat16_4;
    u_xlat16_2.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_0.xy, 0.0).xyz;
    u_xlat16_3.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_0.zw, 0.0).xyz;
    u_xlat16_0.x = dot(u_xlat16_2.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_0.x = u_xlat16_0.x + _Params.y;
    u_xlat16_0.x = float(1.0) / u_xlat16_0.x;
    u_xlat16_5.xyz = u_xlat16_2.xyz * u_xlat16_0.xxx + u_xlat16_6.xyz;
    u_xlat16_0.x = u_xlat16_0.x + u_xlat16_1.x;
    u_xlat16_1.x = dot(u_xlat16_3.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_1.x = u_xlat16_1.x + _Params.y;
    u_xlat16_1.x = float(1.0) / u_xlat16_1.x;
    u_xlat16_5.xyz = u_xlat16_3.xyz * u_xlat16_1.xxx + u_xlat16_5.xyz;
    u_xlat16_0.x = u_xlat16_0.x + u_xlat16_1.x;
    u_xlat16_2.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, v.vs_TEXCOORD0.xy, 0.0).xyz;
    u_xlat16_1.x = dot(u_xlat16_2.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_1.x = u_xlat16_1.x + _Params.y;
    u_xlat16_1.x = float(1.0) / u_xlat16_1.x;
    u_xlat16_6.xyz = u_xlat16_1.xxx * u_xlat16_2.xyz;
    u_xlat16_0.x = u_xlat16_1.x * 4.0 + u_xlat16_0.x;
    u_xlat16_5.xyz = u_xlat16_6.xyz * float3(4.0, 4.0, 4.0) + u_xlat16_5.xyz;
    o.SV_Target0.xyz = u_xlat16_5.xyz / u_xlat16_0.xxx;
    o.SV_Target0.w = 1.0;
    return o;

}
ENDHLSL
} } }
