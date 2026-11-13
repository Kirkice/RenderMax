// Associated EIDs: 834, 835
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0834_Off_Blended" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_RMCaptured_Time ("_RMCaptured_Time", Vector) = (0,0,0,0)
_VisualClarity ("_VisualClarity", Vector) = (0,0,0,0)
_TriggerLevelOffset ("_TriggerLevelOffset", Float) = 0
_MainTex_ST ("_MainTex_ST", Vector) = (0,0,0,0)
_UVTime ("_UVTime", Vector) = (0,0,0,0)
g_ExposureScale ("g_ExposureScale", Vector) = (0,0,0,0)
_LDREnable ("_LDREnable", Float) = 0
_LDRBrightness ("_LDRBrightness", Float) = 0
_CutOff ("_CutOff", Float) = 0
_FadeFactor ("_FadeFactor", Float) = 0
_TintColor ("_TintColor", Vector) = (0,0,0,0)
_TintColorMultiplier ("_TintColorMultiplier", Float) = 0
_MainTex ("_MainTex", 2D) = "white" {} }
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
float4 _VisualClarity;
float _TriggerLevelOffset;
float4 _MainTex_ST;
float4 _UVTime;
float4 g_ExposureScale;
float4 _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[4];
int _LDREnable;
float _LDRBrightness;
float _CutOff;
float _FadeFactor;
float4 _TintColor;
float _TintColorMultiplier;
Texture2D _MainTex; SamplerState sampler_MainTex;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float4 in_TEXCOORD0 : TEXCOORD0;
float4 in_COLOR0 : COLOR0; };
struct RMVaryings { float4 position : SV_POSITION;
float2 vs_TEXCOORD0 : TEXCOORD0;
float4 vs_TEXCOORD1 : TEXCOORD1; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;












float4 u_xlat0;
float4 u_xlat1;
float u_xlat16_2;
float2 u_xlat6;


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
    u_xlat0.xy = _RMCaptured_Time.xx * _UVTime.xy;
    u_xlat0.xy = u_xlat0.xy * float2(0.00999999978, 0.00999999978);
    u_xlat0.xy = frac(u_xlat0.xy);
    u_xlat6.xy = v.in_TEXCOORD0.xy * _MainTex_ST.xy + _MainTex_ST.zw;
    o.vs_TEXCOORD0.xy = u_xlat0.xy * float2(100.0, 100.0) + u_xlat6.xy;
    o.vs_TEXCOORD1 = v.in_COLOR0;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;












float4 u_xlat16_0;
float4 u_xlat16_1;
bool u_xlatb1;
bool u_xlatb2;
float4 u_xlat16_3;
float3 u_xlat16_4;


    u_xlat16_0 = _MainTex.Sample(sampler_MainTex, v.vs_TEXCOORD0.xy);
    u_xlatb1 = u_xlat16_0.w<_CutOff;
    if(u_xlatb1){discard;}
    u_xlat16_1 = _TintColor * ((float4)(_TintColorMultiplier));
    u_xlat16_1 = u_xlat16_1 * v.vs_TEXCOORD1;
    u_xlat16_0 = u_xlat16_0 * u_xlat16_1;
    u_xlatb2 = 0.0<g_ExposureScale.w;
    u_xlat16_3.xyz = min(u_xlat16_0.xyz, float3(55.0, 55.0, 55.0));
    u_xlat16_4.xyz = u_xlat16_3.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_4.xyz = u_xlat16_3.xyz * u_xlat16_4.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_3.xyz = u_xlat16_3.xyz * u_xlat16_4.xyz;
    u_xlat16_3.xyz = (bool(u_xlatb2)) ? u_xlat16_3.xyz : u_xlat16_0.xyz;
    o.SV_Target0.w = u_xlat16_0.w * _FadeFactor;
    o.SV_Target0.w = clamp(o.SV_Target0.w, 0.0, 1.0);
    u_xlat16_4.xyz = u_xlat16_3.yyy * _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[1].xyz;
    u_xlat16_3.xyw = _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[0].xyz * u_xlat16_3.xxx + u_xlat16_4.xyz;
    u_xlat16_3.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[2].xyz * u_xlat16_3.zzz + u_xlat16_3.xyw;
    u_xlat16_3.xyz = u_xlat16_3.xyz + _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[3].xyz;
    u_xlatb2 = _LDREnable==1;
    u_xlat16_4.xyz = u_xlat16_3.xyz * ((float3)(_LDRBrightness));
    o.SV_Target0.xyz = (bool(u_xlatb2)) ? u_xlat16_4.xyz : u_xlat16_3.xyz;
    return o;

}
ENDHLSL
} } }
