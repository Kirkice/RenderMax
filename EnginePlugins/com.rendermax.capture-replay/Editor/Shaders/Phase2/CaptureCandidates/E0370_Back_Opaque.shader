// Associated EIDs: 370
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0370_Back_Opaque" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_RMCaptured_WorldSpaceCameraPos ("_RMCaptured_WorldSpaceCameraPos", Vector) = (0,0,0,0)
g_FogParam ("g_FogParam", Vector) = (0,0,0,0)
g_FogColor ("g_FogColor", Vector) = (0,0,0,0)
_AlbedoMap_ST ("_AlbedoMap_ST", Vector) = (0,0,0,0)
g_BgExposureScale ("g_BgExposureScale", Vector) = (0,0,0,0)
g_AlphaOutput ("g_AlphaOutput", Float) = 0
_AlbedoColor ("_AlbedoColor", Vector) = (0,0,0,0)
_BlendMode ("_BlendMode", Float) = 0
_EmissiveColor_R ("_EmissiveColor_R", Vector) = (0,0,0,0)
_EmissiveColor_G ("_EmissiveColor_G", Vector) = (0,0,0,0)
_EmissiveColor_B ("_EmissiveColor_B", Vector) = (0,0,0,0)
_UseHeightFog ("_UseHeightFog", Float) = 0
_ReverseTP ("_ReverseTP", Float) = 0
_AlbedoMap ("_AlbedoMap", 2D) = "white" {}
_EmissiveMap ("_EmissiveMap", 2D) = "white" {} }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Back
ZTest LEqual
ZWrite On
Blend Off
ColorMask RGBA
HLSLPROGRAM
#pragma target 3.5
#pragma prefer_hlslcc gles
#pragma vertex RMVertex
#pragma fragment RMFragment
#include "UnityCG.cginc"
float3 _RMCaptured_WorldSpaceCameraPos;
float4 _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float4 g_FogParam;
float4 g_FogColor;
float4 _AlbedoMap_ST;
float4 g_BgExposureScale;
float g_AlphaOutput;
float4 _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[4];
float4 _AlbedoColor;
float _BlendMode;
float3 _EmissiveColor_R;
float3 _EmissiveColor_G;
float3 _EmissiveColor_B;
int _UseHeightFog;
float _ReverseTP;
Texture2D _AlbedoMap; SamplerState sampler_AlbedoMap;
Texture2D _EmissiveMap; SamplerState sampler_EmissiveMap;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float2 in_TEXCOORD0 : TEXCOORD0;
float4 in_COLOR0 : COLOR0; };
struct RMVaryings { float4 position : SV_POSITION;
float2 vs_TEXCOORD0 : TEXCOORD0;
float4 vs_TEXCOORD1 : TEXCOORD1;
float4 vs_COLOR0 : TEXCOORD2; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;












float4 u_xlat0;
float4 u_xlat1;
float4 u_xlat2;
float u_xlat16_3;
float u_xlat12;


    u_xlat0 = v.in_POSITION0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1];
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0] * v.in_POSITION0.xxxx + u_xlat0;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2] * v.in_POSITION0.zzzz + u_xlat0;
    u_xlat1 = u_xlat0 + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3];
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3].xyz * v.in_POSITION0.www + u_xlat0.xyz;
    u_xlat2 = u_xlat1.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1];
    u_xlat2 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0] * u_xlat1.xxxx + u_xlat2;
    u_xlat2 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2] * u_xlat1.zzzz + u_xlat2;
    o.position = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3] * u_xlat1.wwww + u_xlat2;
    u_xlat1.xy = v.in_TEXCOORD0.xy * _AlbedoMap_ST.xy + _AlbedoMap_ST.zw;
    o.vs_TEXCOORD0.xy = u_xlat1.xy;
    u_xlat1.xyz = (-u_xlat0.xyz) + _RMCaptured_WorldSpaceCameraPos.xyz;
    u_xlat12 = dot(u_xlat1.xyz, u_xlat1.xyz);
    u_xlat12 = sqrt(u_xlat12);
    u_xlat12 = u_xlat12 + (-g_FogParam.x);
    u_xlat12 = u_xlat12 / g_FogParam.y;
    u_xlat12 = clamp(u_xlat12, 0.0, 1.0);
    u_xlat1.x = u_xlat0.y + (-g_FogParam.z);
    o.vs_TEXCOORD1.xyz = u_xlat0.xyz;
    u_xlat0.x = u_xlat1.x / g_FogParam.w;
    u_xlat0.x = clamp(u_xlat0.x, 0.0, 1.0);
    u_xlat16_3 = u_xlat0.x + u_xlat12;
    u_xlat16_3 = min(u_xlat16_3, 1.0);
    u_xlat16_3 = u_xlat16_3 * g_FogColor.w;
    o.vs_TEXCOORD1.w = u_xlat16_3;
    o.vs_COLOR0 = v.in_COLOR0;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;
















float4 u_xlat0;
float4 u_xlat16_0;
bool u_xlatb0;
float3 u_xlat16_1;
bool u_xlatb1;
float4 u_xlat16_2;
float3 u_xlat16_3;
float3 u_xlat16_4;
bool u_xlatb5;
float u_xlat16_17;


    u_xlat16_0 = _AlbedoMap.Sample(sampler_AlbedoMap, v.vs_TEXCOORD0.xy);
    u_xlat0 = u_xlat16_0 * _AlbedoColor;
    u_xlat16_1.xyz = _EmissiveMap.Sample(sampler_EmissiveMap, v.vs_TEXCOORD0.xy).xyz;
    u_xlat16_2.xyz = u_xlat16_1.xxx * _EmissiveColor_R.xyz + u_xlat0.xyz;
    u_xlat16_2.xyz = u_xlat16_1.yyy * _EmissiveColor_G.xyz + u_xlat16_2.xyz;
    u_xlat16_2.xyz = u_xlat16_1.zzz * _EmissiveColor_B.xyz + u_xlat16_2.xyz;
    u_xlat16_3.xyz = u_xlat16_2.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_3.xyz = u_xlat16_2.xyz * u_xlat16_3.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_3.xyz = u_xlat16_2.xyz * u_xlat16_3.xyz;
    u_xlat16_4.xyz = (-u_xlat16_2.xyz) + float3(1.03499997, 1.03499997, 1.03499997);
    u_xlat16_4.xyz = max(u_xlat16_4.xyz, float3(0.00999999978, 0.00999999978, 0.00999999978));
    u_xlat16_2.xyz = u_xlat16_2.xyz / u_xlat16_4.xyz;
    u_xlat16_2.xyz = u_xlat16_2.xyz * float3(0.187000006, 0.187000006, 0.187000006) + (-u_xlat16_3.xyz);
    u_xlat16_2.xyz = float3(float3(_ReverseTP, _ReverseTP, _ReverseTP)) * u_xlat16_2.xyz + u_xlat16_3.xyz;
    u_xlat0.xyz = (-u_xlat16_2.xyz) * g_BgExposureScale.xyz + g_FogColor.xyz;
    u_xlat16_2.xyz = u_xlat16_2.xyz * g_BgExposureScale.xyz;
    u_xlat0.xyz = v.vs_TEXCOORD1.www * u_xlat0.xyz + u_xlat16_2.xyz;
    u_xlatb1 = 0<_UseHeightFog;
    u_xlat16_2.xyz = (bool(u_xlatb1)) ? u_xlat0.xyz : u_xlat16_2.xyz;
    u_xlat16_3.xyz = u_xlat16_2.yyy * _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[1].xyz;
    u_xlat16_2.xyw = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[0].xyz * u_xlat16_2.xxx + u_xlat16_3.xyz;
    u_xlat16_2.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[2].xyz * u_xlat16_2.zzz + u_xlat16_2.xyw;
    u_xlat16_2.xyz = u_xlat16_2.xyz + _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].xyz;
    u_xlat16_3.xyz = u_xlat16_2.xyz + float3(0.155000001, 0.155000001, 0.155000001);
    u_xlat16_3.xyz = u_xlat16_2.xyz / u_xlat16_3.xyz;
    u_xlat16_2.xyz = min(u_xlat16_2.xyz, float3(65504.0, 65504.0, 65504.0));
    u_xlat16_3.xyz = u_xlat16_3.xyz * float3(1.01900005, 1.01900005, 1.01900005);
    u_xlat16_17 = dot(u_xlat16_3.xyz, float3(0.333299994, 0.333299994, 0.333299994));
    u_xlatb0 = 0.0>=_BlendMode;
    u_xlatb5 = g_AlphaOutput<1.0;
    u_xlatb0 = u_xlatb5 && u_xlatb0;
    u_xlat16_17 = (u_xlatb0) ? u_xlat16_17 : u_xlat0.w;
    u_xlatb0 = 0.0>=_RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].w;
    o.SV_Target0.w = (u_xlatb0) ? u_xlat16_17 : u_xlat0.w;
    o.SV_Target0.xyz = (bool(u_xlatb0)) ? u_xlat16_3.xyz : u_xlat16_2.xyz;
    return o;

}
ENDHLSL
} } }
