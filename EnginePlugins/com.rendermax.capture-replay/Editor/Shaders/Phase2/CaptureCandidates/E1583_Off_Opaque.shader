// Associated EIDs: 1583, 1594, 1605, 1616, 1627, 1638
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E1583_Off_Opaque" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_BloomSrcTex_TexelSize ("_BloomSrcTex_TexelSize", Vector) = (0,0,0,0)
_SampleScale ("_SampleScale", Float) = 0
_BloomSrcTex ("_BloomSrcTex", 2D) = "white" {}
_BloomTex ("_BloomTex", 2D) = "white" {} }
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
float _SampleScale;
Texture2D _BloomSrcTex; SamplerState sampler_BloomSrcTex;
Texture2D _BloomTex; SamplerState sampler_BloomTex;
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






float3 u_xlat16_0;
float4 u_xlat16_1;
float4 u_xlat16_2;
float4 u_xlat16_3;
float3 u_xlat16_4;
float3 u_xlat16_5;
float3 u_xlat16_6;
float3 u_xlat16_7;
float2 u_xlat16_15;


    u_xlat16_0.x = _SampleScale * 0.5;
    u_xlat16_7.xy = u_xlat16_0.xx * _BloomSrcTex_TexelSize.xy;
    u_xlat16_1 = u_xlat16_7.xyxy * float4(-2.0, 0.0, -1.0, 1.0) + v.vs_TEXCOORD0.xyxy;
    u_xlat16_2.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_1.xy, 0.0).xyz;
    u_xlat16_3.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_1.zw, 0.0).xyz;
    u_xlat16_1.xyz = u_xlat16_3.xyz * float3(2.0, 2.0, 2.0) + u_xlat16_2.xyz;
    u_xlat16_2 = u_xlat16_7.xyxy * float4(0.0, 2.0, 2.0, 0.0) + v.vs_TEXCOORD0.xyxy;
    u_xlat16_3 = u_xlat16_7.xyxy * float4(1.0, -1.0, 0.0, -2.0) + v.vs_TEXCOORD0.xyxy;
    u_xlat16_4.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_2.xy, 0.0).xyz;
    u_xlat16_5.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_2.zw, 0.0).xyz;
    u_xlat16_7.xyz = u_xlat16_1.xyz + u_xlat16_4.xyz;
    u_xlat16_1.xy = _BloomSrcTex_TexelSize.xy * u_xlat16_0.xx + v.vs_TEXCOORD0.xy;
    u_xlat16_15.xy = (-_BloomSrcTex_TexelSize.xy) * u_xlat16_0.xx + v.vs_TEXCOORD0.xy;
    u_xlat16_4.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_15.xy, 0.0).xyz;
    u_xlat16_6.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_1.xy, 0.0).xyz;
    u_xlat16_0.xyz = u_xlat16_6.xyz * float3(2.0, 2.0, 2.0) + u_xlat16_7.xyz;
    u_xlat16_0.xyz = u_xlat16_5.xyz + u_xlat16_0.xyz;
    u_xlat16_5.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_3.xy, 0.0).xyz;
    u_xlat16_6.xyz = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_3.zw, 0.0).xyz;
    u_xlat16_0.xyz = u_xlat16_5.xyz * float3(2.0, 2.0, 2.0) + u_xlat16_0.xyz;
    u_xlat16_0.xyz = u_xlat16_6.xyz + u_xlat16_0.xyz;
    u_xlat16_0.xyz = u_xlat16_4.xyz * float3(2.0, 2.0, 2.0) + u_xlat16_0.xyz;
    u_xlat16_4.xyz = _BloomTex.SampleLevel(sampler_BloomTex, v.vs_TEXCOORD0.xy, 0.0).xyz;
    o.SV_Target0.xyz = u_xlat16_0.xyz * float3(0.0833333358, 0.0833333358, 0.0833333358) + u_xlat16_4.xyz;
    o.SV_Target0.w = 1.0;
    return o;

}
ENDHLSL
} } }
