// Associated EIDs: 1806, 1811
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E1806_Back_Opaque" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_depth ("_depth", Float) = 0 }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Back
ZTest Always
ZWrite On
Blend Off
ColorMask 0
HLSLPROGRAM
#pragma target 3.5
#pragma prefer_hlslcc gles
#pragma vertex RMVertex
#pragma fragment RMFragment
#include "UnityCG.cginc"
float4 _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float _depth;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0; };
struct RMVaryings { float4 position : SV_POSITION;
 };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;




float4 u_xlat0;
float3 u_xlat1;


    u_xlat0 = v.in_POSITION0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1];
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0] * v.in_POSITION0.xxxx + u_xlat0;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2] * v.in_POSITION0.zzzz + u_xlat0;
    u_xlat0 = u_xlat0 + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3];
    u_xlat1.xyz = u_xlat0.yyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1].xyw;
    u_xlat1.xyz = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0].xyw * u_xlat0.xxx + u_xlat1.xyz;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2].xyw * u_xlat0.zzz + u_xlat1.xyz;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3].xyw * u_xlat0.www + u_xlat0.xyz;
    o.position.xyw = u_xlat0.xyz / u_xlat0.zzz;
    o.position.z = _depth;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;



    o.SV_Target0 = float4(0.0, 0.0, 0.0, 0.0);
    return o;

}
ENDHLSL
} } }
