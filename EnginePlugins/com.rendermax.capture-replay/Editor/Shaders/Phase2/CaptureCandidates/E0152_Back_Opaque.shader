// Associated EIDs: 152
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0152_Back_Opaque" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_AlbedoMap_ST ("_AlbedoMap_ST", Vector) = (0,0,0,0)
_AlbedoMap ("_AlbedoMap", 2D) = "white" {} }
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
float4 _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float4 _AlbedoMap_ST;
Texture2D _AlbedoMap; SamplerState sampler_AlbedoMap;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float2 in_TEXCOORD0 : TEXCOORD0; };
struct RMVaryings { float4 position : SV_POSITION;
float4 vs_NORMAL0 : TEXCOORD0;
float4 vs_TANGENT0 : TEXCOORD1;
float4 vs_POSITIONWS0 : TEXCOORD2;
float4 vs_TEXCOORD0 : TEXCOORD3; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;









float4 u_xlat0;
float4 u_xlat1;


    o.vs_NORMAL0 = float4(0.0, 0.0, 0.0, 0.0);
    o.vs_TANGENT0 = float4(0.0, 0.0, 0.0, 0.0);
    o.vs_POSITIONWS0.w = 0.0;
    u_xlat0.xyz = v.in_POSITION0.yyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1].xyz;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0].xyz * v.in_POSITION0.xxx + u_xlat0.xyz;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2].xyz * v.in_POSITION0.zzz + u_xlat0.xyz;
    u_xlat0.xyz = u_xlat0.xyz + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3].xyz;
    o.vs_POSITIONWS0.xyz = u_xlat0.xyz;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy * _AlbedoMap_ST.xy + _AlbedoMap_ST.zw;
    o.vs_TEXCOORD0.zw = float2(0.0, 0.0);
    u_xlat1 = u_xlat0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1];
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0] * u_xlat0.xxxx + u_xlat1;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2] * u_xlat0.zzzz + u_xlat1;
    o.position = u_xlat0 + _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3];
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;




float4 u_xlat16_0;


    u_xlat16_0.xy = v.vs_TEXCOORD0.xy * _AlbedoMap_ST.xy + _AlbedoMap_ST.zw;
    u_xlat16_0 = _AlbedoMap.Sample(sampler_AlbedoMap, u_xlat16_0.xy);
    o.SV_Target0 = u_xlat16_0;
    return o;

}
ENDHLSL
} } }
