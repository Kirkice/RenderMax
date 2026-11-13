// Associated EIDs: 281
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0281_Back_Opaque" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
g_OrthoCamera ("g_OrthoCamera", Float) = 0
g_ShadowNearClipPlane ("g_ShadowNearClipPlane", Float) = 0
g_DepthBias ("g_DepthBias", Vector) = (0,0,0,0)
g_dxCamera ("g_dxCamera", Vector) = (0,0,0,0)
g_dyCamera ("g_dyCamera", Vector) = (0,0,0,0)
g_ShadowPancaking ("g_ShadowPancaking", Float) = 0 }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Back
ZTest LEqual
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
float4 _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[4];
float4 _RMCaptured_hlslcc_mtx4x4glstate_matrix_projection[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixV[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[4];
float g_OrthoCamera;
float g_ShadowNearClipPlane;
float3 g_DepthBias;
float3 g_dxCamera;
float3 g_dyCamera;
float g_ShadowPancaking;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float3 in_NORMAL0 : NORMAL0;
float4 in_TANGENT0 : TANGENT0;
float2 in_TEXCOORD0 : TEXCOORD0;
float4 in_TEXCOORD1 : TEXCOORD1; };
struct RMVaryings { float4 position : SV_POSITION;
float4 vs_NORMAL0 : TEXCOORD0;
float4 vs_TANGENT0 : TEXCOORD1;
float4 vs_POSITIONWS0 : TEXCOORD2;
float4 vs_TEXCOORD0 : TEXCOORD3; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;




















float4 u_xlat0;
float4 u_xlat1;
bool u_xlatb1;
float3 u_xlat2;
float3 u_xlat3;
float3 u_xlat4;
float4 u_xlat5;
float4 u_xlat6;
float u_xlat7;
float3 u_xlat8;
float u_xlat14;
float u_xlat15;
float u_xlat16;
float u_xlat21;
bool u_xlatb21;
float u_xlat23;


    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[1].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[0].yyy;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[0].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[0].xxx + u_xlat0.xyz;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[2].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[0].zzz + u_xlat0.xyz;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[3].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[0].www + u_xlat0.xyz;
    u_xlat1.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[1].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[1].yyy;
    u_xlat1.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[0].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[1].xxx + u_xlat1.xyz;
    u_xlat1.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[2].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[1].zzz + u_xlat1.xyz;
    u_xlat1.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[3].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[1].www + u_xlat1.xyz;
    u_xlat2.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[1].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[2].yyy;
    u_xlat2.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[0].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[2].xxx + u_xlat2.xyz;
    u_xlat2.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[2].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[2].zzz + u_xlat2.xyz;
    u_xlat2.xyz = _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[3].xyz * _RMCaptured_hlslcc_mtx4x4unity_MatrixInvV[2].www + u_xlat2.xyz;
    u_xlat3.xyz = v.in_POSITION0.yyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1].xyz;
    u_xlat3.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0].xyz * v.in_POSITION0.xxx + u_xlat3.xyz;
    u_xlat3.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2].xyz * v.in_POSITION0.zzz + u_xlat3.xyz;
    u_xlat3.xyz = u_xlat3.xyz + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3].xyz;
    u_xlat4.x = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[0].xyz);
    u_xlat4.y = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[1].xyz);
    u_xlat4.z = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[2].xyz);
    u_xlat21 = dot(u_xlat4.xyz, u_xlat4.xyz);
    u_xlat21 = rsqrt(u_xlat21);
    u_xlat4.xyz = ((float3)(u_xlat21)) * u_xlat4.xyz;
    u_xlat5.xyz = v.in_TANGENT0.yyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1].xyz;
    u_xlat5.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0].xyz * v.in_TANGENT0.xxx + u_xlat5.xyz;
    u_xlat5.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2].xyz * v.in_TANGENT0.zzz + u_xlat5.xyz;
    u_xlat21 = dot(u_xlat5.xyz, u_xlat5.xyz);
    u_xlat21 = rsqrt(u_xlat21);
    u_xlat5.xyz = ((float3)(u_xlat21)) * u_xlat5.xyz;
    u_xlat6.xyz = u_xlat3.yyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixV[1].xyz;
    u_xlat6.xyz = _RMCaptured_hlslcc_mtx4x4unity_MatrixV[0].xyz * u_xlat3.xxx + u_xlat6.xyz;
    u_xlat6.xyz = _RMCaptured_hlslcc_mtx4x4unity_MatrixV[2].xyz * u_xlat3.zzz + u_xlat6.xyz;
    u_xlat6.xyz = u_xlat6.xyz + _RMCaptured_hlslcc_mtx4x4unity_MatrixV[3].xyz;
    u_xlat0.x = dot(u_xlat0.xyz, v.in_NORMAL0.xyz);
    u_xlat0.y = dot(u_xlat1.xyz, v.in_NORMAL0.xyz);
    u_xlat0.z = dot(u_xlat2.xyz, v.in_NORMAL0.xyz);
    u_xlat21 = dot(u_xlat0.xyz, u_xlat0.xyz);
    u_xlat21 = rsqrt(u_xlat21);
    u_xlat0.xyz = ((float3)(u_xlat21)) * u_xlat0.xyz;
    u_xlatb21 = 0.0<g_ShadowPancaking;
    u_xlatb1 = 0.0>=u_xlat6.z;
    u_xlat1.x = u_xlatb1 ? 1.0 : float(0.0);
    u_xlat1.x = u_xlat6.z * u_xlat1.x;
    u_xlat6.w = (u_xlatb21) ? u_xlat1.x : u_xlat6.z;
    u_xlatb21 = 0.0<g_OrthoCamera;
    if(u_xlatb21){
        u_xlat1.x = abs(u_xlat0.z);
        u_xlat1.x = max(u_xlat1.x, 0.200000003);
        u_xlat8.x = u_xlat0.x * g_dxCamera.x;
        u_xlat8.x = u_xlat8.x / u_xlat1.x;
        u_xlat15 = u_xlat0.y * g_dyCamera.y;
        u_xlat1.x = u_xlat15 / u_xlat1.x;
        u_xlat15 = max(abs(u_xlat1.x), abs(u_xlat8.x));
        u_xlat1.x = abs(u_xlat1.x) + abs(u_xlat8.x);
        u_xlat1.x = max(u_xlat1.x, u_xlat15);
        u_xlat8.x = float(0.0);
        u_xlat8.y = float(0.0);
        u_xlat8.z = float(-1.0);
    } else {
        u_xlat2.x = dot(u_xlat6.xyw, u_xlat6.xyw);
        u_xlat2.x = rsqrt(u_xlat2.x);
        u_xlat8.xyz = u_xlat2.xxx * u_xlat6.xyw;
        u_xlat2.x = (-u_xlat6.w) / g_ShadowNearClipPlane;
        u_xlat2.xy = u_xlat0.xy * u_xlat2.xx;
        u_xlat16 = u_xlat2.x * g_dxCamera.x;
        u_xlat23 = u_xlat2.y * g_dyCamera.y;
        u_xlat0.x = dot(u_xlat6.xyw, u_xlat0.xyz);
        u_xlat7 = u_xlat16 * u_xlat6.w;
        u_xlat14 = u_xlat2.x * g_dxCamera.x + u_xlat0.x;
        u_xlat14 = max(abs(u_xlat14), 0.200000003);
        u_xlat7 = abs(u_xlat7) / u_xlat14;
        u_xlat14 = u_xlat23 * u_xlat6.w;
        u_xlat0.x = u_xlat2.y * g_dyCamera.y + u_xlat0.x;
        u_xlat0.x = max(abs(u_xlat0.x), 0.200000003);
        u_xlat0.x = abs(u_xlat14) / u_xlat0.x;
        u_xlat14 = max(u_xlat0.x, u_xlat7);
        u_xlat0.x = u_xlat0.x + u_xlat7;
        u_xlat1.x = max(u_xlat0.x, u_xlat14);
    }
    u_xlat0.x = abs(u_xlat6.w) * g_DepthBias.y;
    u_xlat0.x = u_xlat0.x / g_ShadowNearClipPlane;
    u_xlat0.x = (u_xlatb21) ? g_DepthBias.y : u_xlat0.x;
    u_xlat7 = dot(g_DepthBias.xx, u_xlat1.xx);
    u_xlat0.x = min(u_xlat0.x, u_xlat7);
    u_xlat0.xyz = u_xlat0.xxx * u_xlat8.xyz + u_xlat6.xyw;
    u_xlat1 = u_xlat0.yyyy * _RMCaptured_hlslcc_mtx4x4glstate_matrix_projection[1];
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4glstate_matrix_projection[0] * u_xlat0.xxxx + u_xlat1;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4glstate_matrix_projection[2] * u_xlat0.zzzz + u_xlat1;
    o.position = u_xlat0 + _RMCaptured_hlslcc_mtx4x4glstate_matrix_projection[3];
    o.vs_NORMAL0.xyz = u_xlat4.xyz;
    o.vs_NORMAL0.w = 1.0;
    u_xlat5.w = v.in_TANGENT0.w;
    o.vs_TANGENT0 = u_xlat5;
    o.vs_POSITIONWS0.xyz = u_xlat3.xyz;
    o.vs_POSITIONWS0.w = 1.0;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy;
    o.vs_TEXCOORD0.zw = v.in_TEXCOORD1.xy;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;



    o.SV_Target0 = float4(1.0, 1.0, 1.0, 1.0);
    return o;

}
ENDHLSL
} } }
