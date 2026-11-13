// Associated EIDs: 422, 442, 480
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0422_Back_Blended" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_RMCaptured_Time ("_RMCaptured_Time", Vector) = (0,0,0,0)
_RMCaptured_WorldSpaceCameraPos ("_RMCaptured_WorldSpaceCameraPos", Vector) = (0,0,0,0)
_RMCaptured_ProjectionParams ("_RMCaptured_ProjectionParams", Vector) = (0,0,0,0)
_TriggerLevelOffset ("_TriggerLevelOffset", Float) = 0
_HSVConversionMatrix_G ("_HSVConversionMatrix_G", Vector) = (0,0,0,0)
_TintColor ("_TintColor", Vector) = (0,0,0,0)
_MainTex_ST ("_MainTex_ST", Vector) = (0,0,0,0)
_DissolveTexA_ST ("_DissolveTexA_ST", Vector) = (0,0,0,0)
_DissolveTexB_ST ("_DissolveTexB_ST", Vector) = (0,0,0,0)
_MainTexUV ("_MainTexUV", Vector) = (0,0,0,0)
_DissolveTexUV ("_DissolveTexUV", Vector) = (0,0,0,0)
_MainTexSceneUV ("_MainTexSceneUV", Float) = 0
_DTexASceneUV ("_DTexASceneUV", Float) = 0
_DTexBSceneUV ("_DTexBSceneUV", Float) = 0
g_ExposureScale ("g_ExposureScale", Vector) = (0,0,0,0)
_LDREnable ("_LDREnable", Float) = 0
_LDRBrightness ("_LDRBrightness", Float) = 0
_HSVConversionMatrix_R ("_HSVConversionMatrix_R", Vector) = (0,0,0,0)
_HSVConversionMatrix_B ("_HSVConversionMatrix_B", Vector) = (0,0,0,0)
_TexBWeight ("_TexBWeight", Float) = 0
_DissolveEdgeColorA ("_DissolveEdgeColorA", Vector) = (0,0,0,0)
_DissolveEdgeColorB ("_DissolveEdgeColorB", Vector) = (0,0,0,0)
_FresnelEdgeColor ("_FresnelEdgeColor", Vector) = (0,0,0,0)
_DissInt ("_DissInt", Float) = 0
_DissFea ("_DissFea", Float) = 0
_DissOffset ("_DissOffset", Float) = 0
_EdgeOpacity ("_EdgeOpacity", Float) = 0
_FrenelRange ("_FrenelRange", Float) = 0
_FrenelOpacity ("_FrenelOpacity", Float) = 0
_FresnelPower ("_FresnelPower", Float) = 0
_BlendType ("_BlendType", Float) = 0
_MainTex ("_MainTex", 2D) = "white" {}
_DissolveTexA ("_DissolveTexA", 2D) = "white" {}
_DissolveTexB ("_DissolveTexB", 2D) = "white" {} }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Back
ZTest LEqual
ZWrite Off
Blend SrcAlpha OneMinusSrcAlpha, SrcAlpha OneMinusSrcAlpha
BlendOp Add, Add
ColorMask RGBA
HLSLPROGRAM
#pragma target 3.5
#pragma prefer_hlslcc gles
#pragma vertex RMVertex
#pragma fragment RMFragment
#include "UnityCG.cginc"
float4 _RMCaptured_Time;
float3 _RMCaptured_WorldSpaceCameraPos;
float4 _RMCaptured_ProjectionParams;
float4 _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float _TriggerLevelOffset;
float4 _HSVConversionMatrix_G;
float4 _TintColor;
float4 _MainTex_ST;
float4 _DissolveTexA_ST;
float4 _DissolveTexB_ST;
float4 _MainTexUV;
float4 _DissolveTexUV;
int _MainTexSceneUV;
int _DTexASceneUV;
int _DTexBSceneUV;
float4 g_ExposureScale;
float4 _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[4];
int _LDREnable;
float _LDRBrightness;
float4 _HSVConversionMatrix_R;
float4 _HSVConversionMatrix_B;
float _TexBWeight;
float4 _DissolveEdgeColorA;
float3 _DissolveEdgeColorB;
float4 _FresnelEdgeColor;
float _DissInt;
float _DissFea;
float _DissOffset;
float _EdgeOpacity;
float _FrenelRange;
float _FrenelOpacity;
float _FresnelPower;
int _BlendType;
Texture2D _MainTex; SamplerState sampler_MainTex;
Texture2D _DissolveTexA; SamplerState sampler_DissolveTexA;
Texture2D _DissolveTexB; SamplerState sampler_DissolveTexB;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float4 in_TEXCOORD0 : TEXCOORD0;
float3 in_NORMAL0 : NORMAL0;
float4 in_COLOR0 : COLOR0; };
struct RMVaryings { float4 position : SV_POSITION;
float2 vs_TEXCOORD0 : TEXCOORD0;
float2 vs_TEXCOORD3 : TEXCOORD1;
float4 vs_TECOORD1 : TEXCOORD2;
float4 vs_TEXCOORD2 : TEXCOORD3; };
struct RMColor { float4 SV_TARGET0 : SV_TARGET0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;

























float4 u_xlat0;
float4 u_xlat1;
float4 u_xlat2;
float2 u_xlat16_3;
float3 u_xlat4;
float2 u_xlat8;
float u_xlat12;


    u_xlat0.x = _TriggerLevelOffset + _HSVConversionMatrix_G.w;
    u_xlat1 = v.in_POSITION0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1];
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0] * v.in_POSITION0.xxxx + u_xlat1;
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2] * v.in_POSITION0.zzzz + u_xlat1;
    u_xlat2 = u_xlat1 + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3];
    u_xlat4.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3].xyz * v.in_POSITION0.www + u_xlat1.xyz;
    u_xlat4.xyz = (-u_xlat4.xyz) + _RMCaptured_WorldSpaceCameraPos.xyz;
    u_xlat1 = u_xlat2.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1];
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0] * u_xlat2.xxxx + u_xlat1;
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2] * u_xlat2.zzzz + u_xlat1;
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3] * u_xlat2.wwww + u_xlat1;
    o.position.z = u_xlat0.x * -0.0399999991 + u_xlat1.z;
    o.position.xyw = u_xlat1.xyw;
    u_xlat0.x = dot(u_xlat4.xyz, u_xlat4.xyz);
    u_xlat0.x = rsqrt(u_xlat0.x);
    u_xlat0.xyz = u_xlat0.xxx * u_xlat4.xyz;
    u_xlat2.x = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[0].xyz);
    u_xlat2.y = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[1].xyz);
    u_xlat2.z = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[2].xyz);
    u_xlat12 = dot(u_xlat2.xyz, u_xlat2.xyz);
    u_xlat12 = rsqrt(u_xlat12);
    u_xlat2.xyz = ((float3)(u_xlat12)) * u_xlat2.xyz;
    u_xlat0.x = dot(u_xlat2.xyz, u_xlat0.xyz);
    u_xlat16_3.x = -abs(u_xlat0.x) + 1.0;
    o.vs_TEXCOORD3.x = abs(u_xlat0.x);
    o.vs_TEXCOORD3.y = sqrt(u_xlat16_3.x);
    u_xlat0.x = u_xlat1.y * _RMCaptured_ProjectionParams.x;
    u_xlat0.w = u_xlat0.x * 0.5;
    u_xlat0.xz = u_xlat1.xw * float2(0.5, 0.5);
    u_xlat0.xy = u_xlat0.zz + u_xlat0.xw;
    u_xlat0.xy = u_xlat0.xy / u_xlat1.ww;
    u_xlat8.xy = u_xlat0.xy * _MainTex_ST.xy + _MainTex_ST.zw;
    u_xlat16_3.xy = v.in_TEXCOORD0.xy * _MainTex_ST.xy + _MainTex_ST.zw;
    u_xlat8.xy = (int(_MainTexSceneUV) != 0) ? u_xlat8.xy : u_xlat16_3.xy;
    u_xlat1.xy = _RMCaptured_Time.xx * _MainTexUV.xy;
    u_xlat1.xy = frac(u_xlat1.xy);
    u_xlat8.xy = u_xlat8.xy + u_xlat1.xy;
    o.vs_TEXCOORD0.xy = u_xlat8.xy;
    u_xlat8.xy = u_xlat0.xy * _DissolveTexA_ST.xy + _DissolveTexA_ST.zw;
    u_xlat0.xy = u_xlat0.xy * _DissolveTexB_ST.xy + _DissolveTexB_ST.zw;
    u_xlat16_3.xy = v.in_TEXCOORD0.xy * _DissolveTexA_ST.xy + _DissolveTexA_ST.zw;
    u_xlat0.zw = (int(_DTexASceneUV) != 0) ? u_xlat8.xy : u_xlat16_3.xy;
    u_xlat1 = _RMCaptured_Time.xxxx * _DissolveTexUV;
    u_xlat1 = frac(u_xlat1);
    u_xlat16_3.xy = v.in_TEXCOORD0.xy * _DissolveTexB_ST.xy + _DissolveTexB_ST.zw;
    u_xlat0.xy = (int(_DTexBSceneUV) != 0) ? u_xlat0.xy : u_xlat16_3.xy;
    u_xlat2 = u_xlat0.zwxy + u_xlat1;
    o.vs_TECOORD1 = u_xlat2;
    o.vs_TEXCOORD2 = v.in_COLOR0 * _TintColor;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;



























float u_xlat0;
bool u_xlatb0;
float4 u_xlat16_1;
float3 u_xlat16_2;
float4 u_xlat3;
float3 u_xlat16_3;
float3 u_xlat16_4;
float3 u_xlat16_5;
float u_xlat6;
bool u_xlatb6;
float3 u_xlat7;
float2 u_xlat16_7;
bool u_xlatb7;
float2 u_xlat16_8;
float u_xlat16_9;
float u_xlat13;
float u_xlat14;
bool u_xlatb21;


    u_xlatb0 = _BlendType==1;
    u_xlat16_7.x = _DissolveTexB.Sample(sampler_DissolveTexB, v.vs_TECOORD1.zw).x;
    u_xlat16_1.x = (-u_xlat16_7.x) + 1.0;
    u_xlat16_7.y = _DissolveTexA.Sample(sampler_DissolveTexA, v.vs_TECOORD1.xy).x;
    u_xlat16_8.x = (-u_xlat16_7.y) + 1.0;
    u_xlat16_8.x = u_xlat16_8.x + u_xlat16_8.x;
    u_xlat16_1.x = (-u_xlat16_8.x) * u_xlat16_1.x + 1.0;
    u_xlatb21 = 0.5>=u_xlat16_7.x;
    u_xlat16_8.xy = u_xlat16_7.yx + u_xlat16_7.yy;
    u_xlat16_8.x = u_xlat16_7.x * u_xlat16_8.x;
    u_xlat16_1.x = (u_xlatb21) ? u_xlat16_8.x : u_xlat16_1.x;
    u_xlat16_8.x = u_xlat16_7.x * u_xlat16_7.y;
    u_xlat16_1.x = (u_xlatb0) ? u_xlat16_8.x : u_xlat16_1.x;
    u_xlat16_1.x = (_BlendType != 0) ? u_xlat16_1.x : u_xlat16_8.y;
    u_xlat16_1.x = (-u_xlat16_7.y) + u_xlat16_1.x;
    u_xlat16_1.x = _TexBWeight * u_xlat16_1.x + u_xlat16_7.y;
    u_xlat16_1.x = clamp(u_xlat16_1.x, 0.0, 1.0);
    u_xlatb0 = _TexBWeight==0.0;
    u_xlat0 = (u_xlatb0) ? u_xlat16_7.y : u_xlat16_1.x;
    u_xlat0 = u_xlat0 + (-_DissInt);
    u_xlat16_1 = _MainTex.Sample(sampler_MainTex, v.vs_TEXCOORD0.xy);
    u_xlat16_2.x = (-u_xlat16_1.w) + 1.0;
    u_xlat0 = u_xlat0 + (-u_xlat16_2.x);
    u_xlat16_2.x = (-_DissInt) + 1.0;
    u_xlat0 = u_xlat16_2.x * _DissFea + u_xlat0;
    u_xlat7.x = float(1.0) / _DissFea;
    u_xlat0 = u_xlat7.x * u_xlat0;
    u_xlat0 = clamp(u_xlat0, 0.0, 1.0);
    u_xlat7.x = u_xlat0 * -2.0 + 3.0;
    u_xlat0 = u_xlat0 * u_xlat0;
    u_xlat14 = u_xlat7.x * u_xlat0 + (-_DissOffset);
    u_xlat0 = u_xlat0 * u_xlat7.x;
    u_xlat16_2.x = (-u_xlat14) + 0.5;
    u_xlat16_2.x = u_xlat16_2.x + u_xlat16_2.x;
    u_xlat16_2.x = clamp(u_xlat16_2.x, 0.0, 1.0);
    u_xlat16_9 = u_xlat16_2.x * -2.0 + 3.0;
    u_xlat16_2.x = u_xlat16_2.x * u_xlat16_2.x;
    u_xlat16_2.x = u_xlat16_2.x * u_xlat16_9;
    u_xlat16_2.xyz = u_xlat16_2.xxx * _DissolveEdgeColorA.xyz;
    u_xlat7.xyz = (-_DissolveEdgeColorB.xyz) + float3(1.0, 1.0, 1.0);
    u_xlat7.xyz = ((float3)(u_xlat0)) * u_xlat7.xyz + _DissolveEdgeColorB.xyz;
    u_xlat0 = u_xlat0 * 4.0;
    u_xlat0 = min(u_xlat0, 1.0);
    u_xlat16_3.xyz = u_xlat16_1.xyz * v.vs_TEXCOORD2.xyz;
    u_xlat16_4.xyz = min(u_xlat16_3.xyz, float3(55.0, 55.0, 55.0));
    u_xlat16_5.xyz = u_xlat16_4.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_5.xyz = u_xlat16_4.xyz * u_xlat16_5.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_4.xyz = u_xlat16_4.xyz * u_xlat16_5.xyz;
    u_xlatb6 = 0.0<g_ExposureScale.w;
    u_xlat16_3.xyz = (bool(u_xlatb6)) ? u_xlat16_4.xyz : u_xlat16_3.xyz;
    u_xlat16_2.xyz = u_xlat16_3.xyz * u_xlat7.xyz + u_xlat16_2.xyz;
    u_xlat16_2.xyz = (-u_xlat16_3.xyz) + u_xlat16_2.xyz;
    u_xlat16_2.xyz = float3(float3(_EdgeOpacity, _EdgeOpacity, _EdgeOpacity)) * u_xlat16_2.xyz + u_xlat16_3.xyz;
    u_xlat7.xyz = u_xlat16_2.xyz + (-_FresnelEdgeColor.xyz);
    u_xlatb6 = _FresnelPower>=0.0;
    u_xlat16_2.xy = v.vs_TEXCOORD3.xy + float2(-1.0, -1.0);
    u_xlat16_2.x = _FresnelPower * u_xlat16_2.x + 1.0;
    u_xlat16_9 = (-_FresnelPower) * u_xlat16_2.y + 1.0;
    u_xlat16_2.x = (u_xlatb6) ? u_xlat16_2.x : u_xlat16_9;
    u_xlat6 = (-u_xlat16_2.x) + 1.0;
    u_xlat13 = u_xlat6 * u_xlat6;
    u_xlat13 = u_xlat13 * u_xlat13;
    u_xlat13 = u_xlat13 * u_xlat13 + (-u_xlat6);
    u_xlat16_2.x = (-_FrenelRange) + 1.0;
    u_xlat6 = u_xlat16_2.x * u_xlat13 + u_xlat6;
    u_xlat13 = (-u_xlat6) + 1.0;
    u_xlat6 = u_xlat6 * u_xlat6 + -1.0;
    u_xlat7.xyz = ((float3)(u_xlat13)) * u_xlat7.xyz + _FresnelEdgeColor.xyz;
    u_xlat16_2.xyz = u_xlat7.yyy * _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[1].xyz;
    u_xlat16_2.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[0].xyz * u_xlat7.xxx + u_xlat16_2.xyz;
    u_xlat16_2.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[2].xyz * u_xlat7.zzz + u_xlat16_2.xyz;
    u_xlat16_2.xyz = u_xlat16_2.xyz + _RMCaptured_hlslcc_mtx4x4g_ColorMatrixVFX[3].xyz;
    u_xlat16_3.xyz = u_xlat16_2.xyz * ((float3)(_LDRBrightness));
    u_xlatb7 = _LDREnable==1;
    u_xlat16_2.xyz = (bool(u_xlatb7)) ? u_xlat16_3.xyz : u_xlat16_2.xyz;
    u_xlat3.x = dot(_HSVConversionMatrix_R.xyz, u_xlat16_2.xyz);
    u_xlat3.y = dot(_HSVConversionMatrix_G.xyz, u_xlat16_2.xyz);
    u_xlat3.z = dot(_HSVConversionMatrix_B.xyz, u_xlat16_2.xyz);
    u_xlat7.x = u_xlat0 * -2.0 + 3.0;
    u_xlat0 = u_xlat0 * u_xlat0;
    u_xlat0 = u_xlat0 * u_xlat7.x;
    u_xlat0 = u_xlat0 * u_xlat16_1.w;
    u_xlat16_2.x = _FrenelOpacity;
    u_xlat16_2.x = clamp(u_xlat16_2.x, 0.0, 1.0);
    u_xlat7.x = u_xlat16_2.x * u_xlat6 + 1.0;
    u_xlat0 = u_xlat7.x * u_xlat0;
    u_xlat16_2.x = u_xlat0 * v.vs_TEXCOORD2.w;
    u_xlat16_2.x = clamp(u_xlat16_2.x, 0.0, 1.0);
    u_xlat3.w = u_xlat16_2.x * _HSVConversionMatrix_R.w;
    o.SV_TARGET0 = u_xlat3;
    return o;

}
ENDHLSL
} } }
