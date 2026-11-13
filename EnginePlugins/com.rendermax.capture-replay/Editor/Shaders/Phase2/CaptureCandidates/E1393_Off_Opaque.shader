// Associated EIDs: 1393
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E1393_Off_Opaque" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
g_ExposureScale ("g_ExposureScale", Vector) = (0,0,0,0)
_BloomSrcTex_TexelSize ("_BloomSrcTex_TexelSize", Vector) = (0,0,0,0)
_Threshold ("_Threshold", Vector) = (0,0,0,0)
_Params ("_Params", Vector) = (0,0,0,0)
_ExposureParams ("_ExposureParams", Vector) = (0,0,0,0)
_BloomSrcUvScale ("_BloomSrcUvScale", Vector) = (0,0,0,0)
_PostExposure ("_PostExposure", Float) = 0
_BloomSrcTex ("_BloomSrcTex", 2D) = "white" {} }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Off
ZTest Always
ZWrite Off
Blend Off
ColorMask RGBA
HLSLPROGRAM
#pragma target 3.5
#pragma prefer_hlslcc gles
#pragma vertex RMVertex
#pragma fragment RMFragment
#include "UnityCG.cginc"
float4 g_ExposureScale;
float4 _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[4];
float4 _BloomSrcTex_TexelSize;
float4 _Threshold;
float4 _Params;
float4 _ExposureParams;
float2 _BloomSrcUvScale;
float _PostExposure;
Texture2D _BloomSrcTex; SamplerState sampler_BloomSrcTex;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float2 in_TEXCOORD0 : TEXCOORD0; };
struct RMVaryings { float4 position : SV_POSITION;
float2 vs_TEXCOORD0 : TEXCOORD0; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;





    o.position.xyw = v.in_POSITION0.xyw;
    o.position.z = 1.0;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;











float4 u_xlat16_0;
float4 u_xlat16_1;
float4 u_xlat16_2;
float3 u_xlat16_3;
float4 u_xlat16_4;
bool u_xlatb4;
float3 u_xlat16_5;
float u_xlat16_9;
float u_xlat16_21;


    u_xlat16_0 = _BloomSrcTex_TexelSize.xyxy * _BloomSrcUvScale.xyxy;
    u_xlat16_1 = u_xlat16_0.zwzw * float4(-2.0, -1.0, 0.0, -1.0) + v.vs_TEXCOORD0.xyxy;
    u_xlat16_2 = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_1.zw, 0.0);
    u_xlat16_1 = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_1.xy, 0.0);
    u_xlat16_3.x = dot(u_xlat16_2.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_3.x = u_xlat16_3.x + _Params.y;
    u_xlat16_3.x = float(1.0) / u_xlat16_3.x;
    u_xlat16_2 = u_xlat16_2 * u_xlat16_3.xxxx;
    u_xlat16_9 = dot(u_xlat16_1.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_9 = u_xlat16_9 + _Params.y;
    u_xlat16_9 = float(1.0) / u_xlat16_9;
    u_xlat16_1 = u_xlat16_1 * ((float4)(u_xlat16_9)) + u_xlat16_2;
    u_xlat16_3.x = u_xlat16_3.x + u_xlat16_9;
    u_xlat16_2 = u_xlat16_0.zwzw * float4(2.0, -1.0, -2.0, 1.0) + v.vs_TEXCOORD0.xyxy;
    u_xlat16_0 = u_xlat16_0 * float4(0.0, 1.0, 2.0, 1.0) + v.vs_TEXCOORD0.xyxy;
    u_xlat16_4 = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_2.xy, 0.0);
    u_xlat16_2 = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_2.zw, 0.0);
    u_xlat16_9 = dot(u_xlat16_4.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_9 = u_xlat16_9 + _Params.y;
    u_xlat16_9 = float(1.0) / u_xlat16_9;
    u_xlat16_1 = u_xlat16_4 * ((float4)(u_xlat16_9)) + u_xlat16_1;
    u_xlat16_3.x = u_xlat16_9 + u_xlat16_3.x;
    u_xlat16_9 = dot(u_xlat16_2.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_9 = u_xlat16_9 + _Params.y;
    u_xlat16_9 = float(1.0) / u_xlat16_9;
    u_xlat16_1 = u_xlat16_2 * ((float4)(u_xlat16_9)) + u_xlat16_1;
    u_xlat16_3.x = u_xlat16_9 + u_xlat16_3.x;
    u_xlat16_2 = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_0.xy, 0.0);
    u_xlat16_0 = _BloomSrcTex.SampleLevel(sampler_BloomSrcTex, u_xlat16_0.zw, 0.0);
    u_xlat16_9 = dot(u_xlat16_2.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_9 = u_xlat16_9 + _Params.y;
    u_xlat16_9 = float(1.0) / u_xlat16_9;
    u_xlat16_1 = u_xlat16_2 * ((float4)(u_xlat16_9)) + u_xlat16_1;
    u_xlat16_3.x = u_xlat16_9 + u_xlat16_3.x;
    u_xlat16_9 = dot(u_xlat16_0.xyz, float3(0.212500006, 0.715399981, 0.0720999986));
    u_xlat16_9 = u_xlat16_9 + _Params.y;
    u_xlat16_9 = float(1.0) / u_xlat16_9;
    u_xlat16_0 = u_xlat16_0 * ((float4)(u_xlat16_9)) + u_xlat16_1;
    u_xlat16_3.x = u_xlat16_9 + u_xlat16_3.x;
    u_xlat16_0 = u_xlat16_0 / u_xlat16_3.xxxx;
    u_xlat16_3.xyz = u_xlat16_0.xyz;
    u_xlat16_3.xyz = clamp(u_xlat16_3.xyz, 0.0, 1.0);
    u_xlat16_5.xyz = (-u_xlat16_3.xyz) + float3(1.01900005, 1.01900005, 1.01900005);
    u_xlat16_3.xyz = u_xlat16_3.xyz / u_xlat16_5.xyz;
    u_xlat16_3.xyz = u_xlat16_3.xyz * float3(0.155000001, 0.155000001, 0.155000001);
    u_xlat16_21 = dot(u_xlat16_3.xyz, float3(0.333299994, 0.333299994, 0.333299994));
    u_xlat16_21 = max(u_xlat16_21, 0.00999999978);
    u_xlat16_21 = u_xlat16_0.w / u_xlat16_21;
    u_xlat16_21 = clamp(u_xlat16_21, 0.0, 1.0);
    u_xlat16_3.xyz = ((float3)(u_xlat16_21)) * u_xlat16_3.xyz;
    u_xlat16_5.xyz = u_xlat16_0.xyz + u_xlat16_0.xyz;
    u_xlatb4 = 1.0>=g_ExposureScale.w;
    u_xlat16_5.xyz = (bool(u_xlatb4)) ? u_xlat16_0.xyz : u_xlat16_5.xyz;
    u_xlatb4 = 0.0<_RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].w;
    u_xlat16_3.xyz = (bool(u_xlatb4)) ? u_xlat16_5.xyz : u_xlat16_3.xyz;
    u_xlat16_5.xyz = u_xlat16_3.xyz * _ExposureParams.xxx;
    u_xlatb4 = 0.0<_PostExposure;
    u_xlat16_3.xyz = (bool(u_xlatb4)) ? u_xlat16_5.xyz : u_xlat16_3.xyz;
    u_xlat16_3.xyz = min(u_xlat16_3.xyz, _Params.xxx);
    u_xlat16_21 = max(u_xlat16_3.z, u_xlat16_3.y);
    u_xlat16_21 = max(u_xlat16_21, u_xlat16_3.x);
    u_xlat16_5.xy = ((float2)(u_xlat16_21)) + (-_Threshold.yx);
    u_xlat16_21 = max(u_xlat16_21, 9.99999975e-05);
    u_xlat16_5.x = max(u_xlat16_5.x, 0.0);
    u_xlat16_5.x = min(u_xlat16_5.x, _Threshold.z);
    u_xlat16_5.x = u_xlat16_5.x * u_xlat16_5.x;
    u_xlat16_5.x = u_xlat16_5.x * _Threshold.w;
    u_xlat16_5.x = max(u_xlat16_5.y, u_xlat16_5.x);
    u_xlat16_21 = u_xlat16_5.x / u_xlat16_21;
    u_xlat16_3.xyz = ((float3)(u_xlat16_21)) * u_xlat16_3.xyz;
    u_xlat16_3.xyz = min((-u_xlat16_3.xyz), float3(0.0, 0.0, 0.0));
    o.SV_Target0.xyz = (-u_xlat16_3.xyz);
    o.SV_Target0.w = 1.0;
    return o;

}
ENDHLSL
} } }
