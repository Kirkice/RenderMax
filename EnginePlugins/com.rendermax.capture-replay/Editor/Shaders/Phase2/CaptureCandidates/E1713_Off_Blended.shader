// Associated EIDs: 1713, 1745
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E1713_Off_Blended" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_MainTex ("_MainTex", 2D) = "white" {} }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Off
ZTest Always
ZWrite Off
Blend SrcAlpha One, SrcAlpha One
BlendOp Add, Add
ColorMask RGB
HLSLPROGRAM
#pragma target 3.5
#pragma prefer_hlslcc gles
#pragma vertex RMVertex
#pragma fragment RMFragment
#include "UnityCG.cginc"
float4 _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
Texture2D _MainTex; SamplerState sampler_MainTex;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float4 in_COLOR0 : COLOR0;
float2 in_TEXCOORD0 : TEXCOORD0; };
struct RMVaryings { float4 position : SV_POSITION;
float2 vs_TEXCOORD0 : TEXCOORD0;
float4 vs_TEXCOORD1 : TEXCOORD1; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;







float4 u_xlat0;
float4 u_xlat1;


    u_xlat0 = v.in_POSITION0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1];
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0] * v.in_POSITION0.xxxx + u_xlat0;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2] * v.in_POSITION0.zzzz + u_xlat0;
    u_xlat0 = u_xlat0 + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3];
    u_xlat1 = u_xlat0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1];
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0] * u_xlat0.xxxx + u_xlat1;
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2] * u_xlat0.zzzz + u_xlat1;
    o.position = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3] * u_xlat0.wwww + u_xlat1;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy;
    o.vs_TEXCOORD1 = v.in_COLOR0;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;




float4 u_xlat16_0;


    u_xlat16_0 = _MainTex.Sample(sampler_MainTex, v.vs_TEXCOORD0.xy);
    o.SV_Target0 = u_xlat16_0 * v.vs_TEXCOORD1;
    return o;

}
ENDHLSL
} } }
