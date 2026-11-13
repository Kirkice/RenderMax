// Associated EIDs: 209, 1195, 1204, 1368
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0209_Off_Blended" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_RMCaptured_Time ("_RMCaptured_Time", Vector) = (0,0,0,0)
g_ExposureScale ("g_ExposureScale", Vector) = (0,0,0,0)
_VisualClarity ("_VisualClarity", Vector) = (0,0,0,0)
_TriggerLevelOffset ("_TriggerLevelOffset", Float) = 0
_MainTex1_ST ("_MainTex1_ST", Vector) = (0,0,0,0)
_MainTex2_ST ("_MainTex2_ST", Vector) = (0,0,0,0)
_UVMode_MainUV ("_UVMode_MainUV", Float) = 0
_ScrollX ("_ScrollX", Float) = 0
_ScrollY ("_ScrollY", Float) = 0
_Scroll2X ("_Scroll2X", Float) = 0
_Scroll2Y ("_Scroll2Y", Float) = 0
_MMultiplier ("_MMultiplier", Float) = 0
_Color ("_Color", Vector) = (0,0,0,0)
_LDREnable ("_LDREnable", Float) = 0
_LDRBrightness ("_LDRBrightness", Float) = 0
_UVXX ("_UVXX", Vector) = (0,0,0,0)
_MainTex1 ("_MainTex1", 2D) = "white" {}
_MainTex2 ("_MainTex2", 2D) = "white" {} }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Off
ZTest LEqual
ZWrite Off
Blend SrcAlpha OneMinusSrcAlpha, Zero One
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
float4 g_ExposureScale;
float4 _VisualClarity;
float _TriggerLevelOffset;
float4 _MainTex1_ST;
float4 _MainTex2_ST;
float _UVMode_MainUV;
float _ScrollX;
float _ScrollY;
float _Scroll2X;
float _Scroll2Y;
float _MMultiplier;
float4 _Color;
float4 _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[4];
int _LDREnable;
float _LDRBrightness;
float4 _UVXX;
Texture2D _MainTex1; SamplerState sampler_MainTex1;
Texture2D _MainTex2; SamplerState sampler_MainTex2;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float4 in_TEXCOORD0 : TEXCOORD0;
float4 in_TEXCOORD1 : TEXCOORD1;
float4 in_COLOR0 : COLOR0; };
struct RMVaryings { float4 position : SV_POSITION;
float4 vs_TEXCOORD0 : TEXCOORD0;
float4 vs_TEXCOORD1 : TEXCOORD1; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;





















float4 u_xlat0;
float4 u_xlat1;
bool u_xlatb1;
float3 u_xlat16_2;
float3 u_xlat16_3;


    u_xlat0 = v.in_POSITION0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1];
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0] * v.in_POSITION0.xxxx + u_xlat0;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2] * v.in_POSITION0.zzzz + u_xlat0;
    u_xlat0 = u_xlat0 + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3];
    u_xlat1 = u_xlat0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1];
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0] * u_xlat0.xxxx + u_xlat1;
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2] * u_xlat0.zzzz + u_xlat1;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3] * u_xlat0.wwww + u_xlat1;
    u_xlat16_2.x = _VisualClarity.w + _TriggerLevelOffset;
    o.position.z = u_xlat16_2.x * -0.0399999991 + u_xlat0.z;
    o.position.xyw = u_xlat0.xyw;
    u_xlat16_2.xy = (-v.in_TEXCOORD0.xy) + v.in_TEXCOORD1.xy;
    u_xlat16_2.xy = ((float2)(_UVMode_MainUV)) * u_xlat16_2.xy + v.in_TEXCOORD0.xy;
    u_xlat0.xy = u_xlat16_2.xy * _MainTex1_ST.xy + _MainTex1_ST.zw;
    u_xlat0.zw = u_xlat16_2.xy * _MainTex2_ST.xy + _MainTex2_ST.zw;
    u_xlat1 = _RMCaptured_Time.xxxx * float4(_ScrollX, _ScrollY, _Scroll2X, _Scroll2Y);
    u_xlat1 = frac(u_xlat1);
    o.vs_TEXCOORD0 = u_xlat0 + u_xlat1;
    u_xlat0 = ((float4)(_MMultiplier)) * _Color;
    u_xlat0 = u_xlat0 * v.in_COLOR0;
    u_xlat16_2.xyz = min(u_xlat0.xyz, float3(55.0, 55.0, 55.0));
    u_xlat16_3.xyz = u_xlat16_2.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_3.xyz = u_xlat16_2.xyz * u_xlat16_3.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_2.xyz = u_xlat16_2.xyz * u_xlat16_3.xyz;
    u_xlatb1 = 0.0<g_ExposureScale.w;
    o.vs_TEXCOORD1.xyz = (bool(u_xlatb1)) ? u_xlat16_2.xyz : u_xlat0.xyz;
    o.vs_TEXCOORD1.w = u_xlat0.w;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;










float2 u_xlat0;
float4 u_xlat16_0;
float3 u_xlat16_1;
float4 u_xlat16_2;
bool u_xlatb3;
float3 u_xlat16_4;
float3 u_xlat16_5;


    u_xlat16_0 = _MainTex1.Sample(sampler_MainTex1, v.vs_TEXCOORD0.xy);
    u_xlat16_1.xyz = min(u_xlat16_0.xyz, float3(55.0, 55.0, 55.0));
    u_xlat16_2.xyz = u_xlat16_1.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_2.xyz = u_xlat16_1.xyz * u_xlat16_2.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_1.xyz = u_xlat16_1.xyz * u_xlat16_2.xyz;
    u_xlatb3 = 0.0<g_ExposureScale.w;
    u_xlat16_1.xyz = (bool(u_xlatb3)) ? u_xlat16_1.xyz : u_xlat16_0.xyz;
    u_xlat0.xy = u_xlat16_1.xx * _UVXX.xx + v.vs_TEXCOORD0.zw;
    u_xlat16_2 = _MainTex2.Sample(sampler_MainTex2, u_xlat0.xy);
    u_xlat16_4.xyz = min(u_xlat16_2.xyz, float3(55.0, 55.0, 55.0));
    u_xlat16_5.xyz = u_xlat16_4.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_5.xyz = u_xlat16_4.xyz * u_xlat16_5.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_4.xyz = u_xlat16_4.xyz * u_xlat16_5.xyz;
    u_xlat16_4.xyz = (bool(u_xlatb3)) ? u_xlat16_4.xyz : u_xlat16_2.xyz;
    u_xlat16_0.w = u_xlat16_0.w * u_xlat16_2.w;
    u_xlat16_0.xyz = u_xlat16_1.xyz * u_xlat16_4.xyz;
    u_xlat16_0 = u_xlat16_0 * v.vs_TEXCOORD1;
    u_xlat16_1.xyz = u_xlat16_0.yyy * _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[1].xyz;
    u_xlat16_1.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[0].xyz * u_xlat16_0.xxx + u_xlat16_1.xyz;
    u_xlat16_1.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[2].xyz * u_xlat16_0.zzz + u_xlat16_1.xyz;
    o.SV_Target0.w = u_xlat16_0.w;
    u_xlat16_1.xyz = u_xlat16_1.xyz + _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[3].xyz;
    u_xlat16_4.xyz = u_xlat16_1.xyz * ((float3)(_LDRBrightness));
    u_xlatb3 = _LDREnable==1;
    o.SV_Target0.xyz = (bool(u_xlatb3)) ? u_xlat16_4.xyz : u_xlat16_1.xyz;
    return o;

}
ENDHLSL
} } }
