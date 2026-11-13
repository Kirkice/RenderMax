// Associated EIDs: 591
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0591_Back_Blended" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_RMCaptured_Time ("_RMCaptured_Time", Vector) = (0,0,0,0)
_VisualClarity ("_VisualClarity", Vector) = (0,0,0,0)
_TriggerLevelOffset ("_TriggerLevelOffset", Float) = 0
_MainTex1_ST ("_MainTex1_ST", Vector) = (0,0,0,0)
_MainTex2_ST ("_MainTex2_ST", Vector) = (0,0,0,0)
_ScrollX ("_ScrollX", Float) = 0
_ScrollY ("_ScrollY", Float) = 0
_Scroll2X ("_Scroll2X", Float) = 0
_Scroll2Y ("_Scroll2Y", Float) = 0
_MMultiplier ("_MMultiplier", Float) = 0
_Color ("_Color", Vector) = (0,0,0,0)
g_ExposureScale ("g_ExposureScale", Vector) = (0,0,0,0)
_UVXX ("_UVXX", Vector) = (0,0,0,0)
_Saturation ("_Saturation", Float) = 0
_MainTex1 ("_MainTex1", 2D) = "white" {}
_MainTex2 ("_MainTex2", 2D) = "white" {} }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Back
ZTest LEqual
ZWrite Off
Blend SrcAlpha One, Zero One
BlendOp Add, Add
ColorMask RGBA
HLSLPROGRAM
#pragma target 3.5
#pragma prefer_hlslcc gles
#pragma vertex RMVertex
#pragma fragment RMFragment
#include "UnityCG.cginc"
float4 _RMCaptured_Time;
float4 _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float4 _VisualClarity;
float _TriggerLevelOffset;
float4 _MainTex1_ST;
float4 _MainTex2_ST;
float _ScrollX;
float _ScrollY;
float _Scroll2X;
float _Scroll2Y;
float _MMultiplier;
float4 _Color;
float4 g_ExposureScale;
float4 _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[4];
float4 _UVXX;
float _Saturation;
Texture2D _MainTex1; SamplerState sampler_MainTex1;
Texture2D _MainTex2; SamplerState sampler_MainTex2;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float4 in_TEXCOORD0 : TEXCOORD0;
float4 in_COLOR0 : COLOR0; };
struct RMVaryings { float4 position : SV_POSITION;
float4 vs_TEXCOORD0 : TEXCOORD0;
float4 vs_TEXCOORD1 : TEXCOORD1; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;


















float4 u_xlat0;
float4 u_xlat1;
float u_xlat16_2;


    u_xlat0 = v.in_POSITION0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1];
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0] * v.in_POSITION0.xxxx + u_xlat0;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2] * v.in_POSITION0.zzzz + u_xlat0;
    u_xlat0 = u_xlat0 + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3];
    u_xlat1 = u_xlat0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1];
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0] * u_xlat0.xxxx + u_xlat1;
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2] * u_xlat0.zzzz + u_xlat1;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3] * u_xlat0.wwww + u_xlat1;
    u_xlat16_2 = _VisualClarity.w + _TriggerLevelOffset;
    o.position.z = u_xlat16_2 * -0.0399999991 + u_xlat0.z;
    o.position.xyw = u_xlat0.xyw;
    u_xlat0.xy = v.in_TEXCOORD0.xy * _MainTex1_ST.xy + _MainTex1_ST.zw;
    u_xlat1 = _RMCaptured_Time.xxxx * float4(_ScrollX, _ScrollY, _Scroll2X, _Scroll2Y);
    u_xlat1 = frac(u_xlat1);
    o.vs_TEXCOORD0.xy = u_xlat0.xy + u_xlat1.xy;
    u_xlat0.xy = v.in_TEXCOORD0.xy * _MainTex2_ST.xy + _MainTex2_ST.zw;
    o.vs_TEXCOORD0.zw = u_xlat1.zw + u_xlat0.xy;
    u_xlat0 = ((float4)(_MMultiplier)) * _Color;
    u_xlat0 = u_xlat0 * v.in_COLOR0;
    o.vs_TEXCOORD1 = u_xlat0;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;










float4 u_xlat16_0;
float2 u_xlat1;
float4 u_xlat16_1;
float3 u_xlat16_2;
float3 u_xlat16_3;
float3 u_xlat16_4;
bool u_xlatb5;
float u_xlat16_20;


    u_xlat16_0 = _MainTex1.Sample(sampler_MainTex1, v.vs_TEXCOORD0.xy);
    u_xlat1.xy = u_xlat16_0.xx * _UVXX.xx + v.vs_TEXCOORD0.zw;
    u_xlat16_1 = _MainTex2.Sample(sampler_MainTex2, u_xlat1.xy);
    u_xlat16_0 = u_xlat16_0 * u_xlat16_1;
    u_xlat16_1 = u_xlat16_0 * v.vs_TEXCOORD1;
    u_xlat16_2.x = u_xlat16_1.y + u_xlat16_1.x;
    u_xlat16_2.x = u_xlat16_0.z * v.vs_TEXCOORD1.z + u_xlat16_2.x;
    u_xlat16_2.xyz = (-u_xlat16_2.xxx) * float3(0.333333343, 0.333333343, 0.333333343) + u_xlat16_1.xyz;
    u_xlat16_2.xyz = clamp(u_xlat16_2.xyz, 0.0, 1.0);
    u_xlat16_2.xyz = (-u_xlat16_2.xyz) * (-((float3)(_Saturation))) + u_xlat16_1.xyz;
    o.SV_Target0.w = u_xlat16_1.w * _VisualClarity.z;
    o.SV_Target0.w = clamp(o.SV_Target0.w, 0.0, 1.0);
    u_xlat16_2.xyz = u_xlat16_2.xyz * _VisualClarity.xxx;
    u_xlat16_2.xyz = clamp(u_xlat16_2.xyz, 0.0, 1.0);
    u_xlat16_20 = u_xlat16_2.y + u_xlat16_2.x;
    u_xlat16_20 = u_xlat16_2.z + u_xlat16_20;
    u_xlat16_3.xyz = (-((float3)(u_xlat16_20))) * float3(0.333333343, 0.333333343, 0.333333343) + u_xlat16_2.xyz;
    u_xlat16_3.xyz = max(u_xlat16_3.xyz, float3(0.0, 0.0, 0.0));
    u_xlat16_20 = (-_VisualClarity.y) + 1.0;
    u_xlat16_2.xyz = (-u_xlat16_3.xyz) * ((float3)(u_xlat16_20)) + u_xlat16_2.xyz;
    u_xlat16_3.xyz = u_xlat16_2.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_3.xyz = u_xlat16_2.xyz * u_xlat16_3.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_3.xyz = u_xlat16_2.xyz * u_xlat16_3.xyz;
    u_xlat16_4.xyz = u_xlat16_3.xyz * float3(0.5, 0.5, 0.5);
    u_xlatb5 = 1.0>=g_ExposureScale.w;
    u_xlat16_3.xyz = (bool(u_xlatb5)) ? u_xlat16_3.xyz : u_xlat16_4.xyz;
    u_xlatb5 = 0.0<_RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].w;
    o.SV_Target0.xyz = (bool(u_xlatb5)) ? u_xlat16_3.xyz : u_xlat16_2.xyz;
    return o;

}
ENDHLSL
} } }
