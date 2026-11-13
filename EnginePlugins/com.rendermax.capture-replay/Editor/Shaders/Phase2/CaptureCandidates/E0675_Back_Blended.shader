// Associated EIDs: 675, 851
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0675_Back_Blended" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_MainTex_ST ("_MainTex_ST", Vector) = (0,0,0,0)
g_ExposureScale ("g_ExposureScale", Vector) = (0,0,0,0)
_LDREnable ("_LDREnable", Float) = 0
_LDRBrightness ("_LDRBrightness", Float) = 0
_Color ("_Color", Vector) = (0,0,0,0)
_MainTex ("_MainTex", 2D) = "white" {} }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Back
ZTest LEqual
ZWrite Off
Blend SrcAlpha OneMinusSrcAlpha, SrcAlpha OneMinusSrcAlpha
BlendOp Add, Add
ColorMask RGB
HLSLPROGRAM
#pragma target 3.5
#pragma prefer_hlslcc gles
#pragma vertex RMVertex
#pragma fragment RMFragment
#include "UnityCG.cginc"
float4 _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float4 _MainTex_ST;
float4 g_ExposureScale;
int _LDREnable;
float _LDRBrightness;
float4 _Color;
Texture2D _MainTex; SamplerState sampler_MainTex;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float3 in_NORMAL0 : NORMAL0;
float4 in_TEXCOORD0 : TEXCOORD0; };
struct RMVaryings { float4 position : SV_POSITION;
float2 vs_TEXCOORD0 : TEXCOORD0;
float3 vs_TEXCOORD1 : TEXCOORD1;
float3 vs_TEXCOORD2 : TEXCOORD2; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;










float4 u_xlat0;
float4 u_xlat1;
float u_xlat6;


    u_xlat0 = v.in_POSITION0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1];
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0] * v.in_POSITION0.xxxx + u_xlat0;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2] * v.in_POSITION0.zzzz + u_xlat0;
    u_xlat1 = u_xlat0 + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3];
    o.vs_TEXCOORD2.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3].xyz * v.in_POSITION0.www + u_xlat0.xyz;
    u_xlat0 = u_xlat1.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1];
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0] * u_xlat1.xxxx + u_xlat0;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2] * u_xlat1.zzzz + u_xlat0;
    o.position = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3] * u_xlat1.wwww + u_xlat0;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy * _MainTex_ST.xy + _MainTex_ST.zw;
    u_xlat0.x = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[0].xyz);
    u_xlat0.y = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[1].xyz);
    u_xlat0.z = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[2].xyz);
    u_xlat6 = dot(u_xlat0.xyz, u_xlat0.xyz);
    u_xlat6 = rsqrt(u_xlat6);
    o.vs_TEXCOORD1.xyz = ((float3)(u_xlat6)) * u_xlat0.xyz;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;







float4 u_xlat0;
float4 u_xlat16_0;
bool u_xlatb0;
float3 u_xlat16_1;
float3 u_xlat16_2;
bool u_xlatb3;


    u_xlat16_0 = _MainTex.Sample(sampler_MainTex, v.vs_TEXCOORD0.xy);
    u_xlat0 = u_xlat16_0 * _Color;
    u_xlat16_1.xyz = min(u_xlat0.xyz, float3(55.0, 55.0, 55.0));
    u_xlat16_2.xyz = u_xlat16_1.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_2.xyz = u_xlat16_1.xyz * u_xlat16_2.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_1.xyz = u_xlat16_1.xyz * u_xlat16_2.xyz;
    u_xlatb3 = 0.0<g_ExposureScale.w;
    u_xlat16_1.xyz = (bool(u_xlatb3)) ? u_xlat16_1.xyz : u_xlat0.xyz;
    o.SV_Target0.w = u_xlat0.w;
    u_xlat16_2.xyz = u_xlat16_1.xyz * ((float3)(_LDRBrightness));
    u_xlatb0 = _LDREnable==1;
    o.SV_Target0.xyz = (bool(u_xlatb0)) ? u_xlat16_2.xyz : u_xlat16_1.xyz;
    return o;

}
ENDHLSL
} } }
