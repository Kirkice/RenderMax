// Associated EIDs: 1656
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E1656_Off_Opaque" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_PostExposure ("_PostExposure", Float) = 0
_ExposureScale ("_ExposureScale", Float) = 0
_AutoExposureEnable ("_AutoExposureEnable", Float) = 0
_ExposureTex ("_ExposureTex", 2D) = "white" {}
g_ExposureScale ("g_ExposureScale", Vector) = (0,0,0,0)
_Bloom_Params ("_Bloom_Params", Vector) = (0,0,0,0)
_Vignette_Params1 ("_Vignette_Params1", Vector) = (0,0,0,0)
_Vignette_Params2 ("_Vignette_Params2", Vector) = (0,0,0,0)
_DistortionIntensity ("_DistortionIntensity", Float) = 0
_DisabledAlpha ("_DisabledAlpha", Float) = 0
_CanyonDistortion ("_CanyonDistortion", Float) = 0
_DistortTex ("_DistortTex", 2D) = "white" {}
_BlitTex ("_BlitTex", 2D) = "white" {}
_BloomTex ("_BloomTex", 2D) = "white" {}
_UE42DLutTex ("_UE42DLutTex", 2D) = "white" {} }
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
float _PostExposure;
float _ExposureScale;
float _AutoExposureEnable;
Texture2D _ExposureTex; SamplerState sampler_ExposureTex;
float4 g_ExposureScale;
float4 _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[4];
float4 _Bloom_Params;
float4 _Vignette_Params1;
float4 _Vignette_Params2;
float _DistortionIntensity;
float _DisabledAlpha;
int _CanyonDistortion;
Texture2D _DistortTex; SamplerState sampler_DistortTex;
Texture2D _BlitTex; SamplerState sampler_BlitTex;
Texture2D _BloomTex; SamplerState sampler_BloomTex;
Texture2D _UE42DLutTex; SamplerState sampler_UE42DLutTex;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float2 in_TEXCOORD0 : TEXCOORD0; };
struct RMVaryings { float4 position : SV_POSITION;
float2 vs_TEXCOORD0 : TEXCOORD0;
float vs_TEXCOORD1 : TEXCOORD1; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;








float u_xlat0;
bool u_xlatb0;
float u_xlat16_1;
float u_xlat16_3;


    o.position.xyw = v.in_POSITION0.xyw;
    o.position.z = 1.0;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy;
    u_xlat0 = _ExposureTex.Load(int3(int2(0, 0), int(0))).x;
    u_xlat16_1 = u_xlat0 + -1.0;
    u_xlat16_1 = _AutoExposureEnable * u_xlat16_1 + 1.0;
    u_xlatb0 = 0.0<_PostExposure;
    u_xlat16_3 = (u_xlatb0) ? _ExposureScale : 1.0;
    o.vs_TEXCOORD1 = u_xlat16_1 * u_xlat16_3;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;















float4 u_xlat0;
float3 u_xlat16_0;
float4 u_xlat16_1;
float3 u_xlat16_2;
float3 u_xlat16_3;
float3 u_xlat16_4;
float u_xlat5;
float3 u_xlat16_5;
float u_xlat6;
bool u_xlatb12;
float2 u_xlat16_13;
float u_xlat18;
bool u_xlatb18;
float u_xlat16_20;


    u_xlat16_0.xyz = _DistortTex.Sample(sampler_DistortTex, v.vs_TEXCOORD0.xy).xyz;
    u_xlat16_1.xy = u_xlat16_0.xy + float2(-0.5, -0.5);
    u_xlatb18 = _CanyonDistortion<1;
    if(u_xlatb18){
        u_xlat0.x = dot(u_xlat16_0.xyz, float3(1.0, 1.0, 1.0));
        u_xlat0.xy = u_xlat0.xx * u_xlat16_1.xy;
        u_xlat16_13.xy = u_xlat0.xy * float2(0.0199999996, 0.0199999996);
        u_xlat0.xy = u_xlat16_13.xy * float2(float2(_DistortionIntensity, _DistortionIntensity)) + v.vs_TEXCOORD0.xy;
    } else {
        u_xlat16_1.xy = u_xlat16_0.zz * u_xlat16_1.xy;
        u_xlat0.xy = u_xlat16_1.xy * float2(0.200000003, 0.200000003) + v.vs_TEXCOORD0.xy;
    }
    u_xlat16_1 = _BlitTex.Sample(sampler_BlitTex, u_xlat0.xy);
    u_xlatb12 = 0.0<_RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].w;
    u_xlat16_2.xyz = u_xlat16_1.xyz + u_xlat16_1.xyz;
    u_xlatb18 = 1.0>=g_ExposureScale.w;
    u_xlat16_2.xyz = (bool(u_xlatb18)) ? u_xlat16_1.xyz : u_xlat16_2.xyz;
    u_xlat16_3.xyz = u_xlat16_1.xyz;
    u_xlat16_3.xyz = clamp(u_xlat16_3.xyz, 0.0, 1.0);
    u_xlat16_4.xyz = (-u_xlat16_3.xyz) + float3(1.01900005, 1.01900005, 1.01900005);
    u_xlat16_3.xyz = u_xlat16_3.xyz / u_xlat16_4.xyz;
    u_xlat16_3.xyz = u_xlat16_3.xyz * float3(0.155000001, 0.155000001, 0.155000001);
    u_xlat16_2.xyz = (bool(u_xlatb12)) ? u_xlat16_2.xyz : u_xlat16_3.xyz;
    u_xlat16_5.xyz = _BloomTex.Sample(sampler_BloomTex, u_xlat0.xy).xyz;
    u_xlat16_3.xyz = u_xlat16_5.xyz * _Bloom_Params.xxx;
    u_xlat16_2.xyz = u_xlat16_3.xyz * _Bloom_Params.yzw + u_xlat16_2.xyz;
    u_xlat16_2.xyz = u_xlat16_2.xyz * ((float3)(v.vs_TEXCOORD1));
    u_xlat16_3.xy = u_xlat0.xy + (-_Vignette_Params2.xy);
    u_xlat16_3.yz = abs(u_xlat16_3.xy) * _Vignette_Params2.zz;
    u_xlat16_3.x = u_xlat16_3.y * _Vignette_Params1.w;
    u_xlat16_20 = dot(u_xlat16_3.xz, u_xlat16_3.xz);
    u_xlat16_20 = (-u_xlat16_20) + 1.0;
    u_xlat16_20 = max(u_xlat16_20, 0.0);
    u_xlat16_20 = log2(u_xlat16_20);
    u_xlat16_20 = u_xlat16_20 * _Vignette_Params2.w;
    u_xlat16_20 = exp2(u_xlat16_20);
    u_xlat16_3.xyz = (-_Vignette_Params1.xyz) + float3(1.0, 1.0, 1.0);
    u_xlat16_3.xyz = ((float3)(u_xlat16_20)) * u_xlat16_3.xyz + _Vignette_Params1.xyz;
    u_xlat16_2.xyz = u_xlat16_2.xyz * u_xlat16_3.xyz;
    u_xlat16_2.xyz = min(u_xlat16_2.xyz, float3(16.0, 16.0, 16.0));
    u_xlat16_2.xyz = u_xlat16_2.xyz + float3(0.00266771927, 0.00266771927, 0.00266771927);
    u_xlat16_2.xyz = log2(u_xlat16_2.xyz);
    u_xlat16_2.xyz = u_xlat16_2.xyz * float3(0.0714285746, 0.0714285746, 0.0714285746) + float3(0.610726953, 0.610726953, 0.610726953);
    u_xlat16_2.xyz = max(u_xlat16_2.xyz, float3(0.0, 0.0, 0.0));
    u_xlat0.yzw = u_xlat16_2.xyz * float3(0.9375, 0.9375, 0.9375) + float3(0.03125, 0.03125, 0.03125);
    u_xlat18 = u_xlat0.w * 16.0 + -0.5;
    u_xlat5 = floor(u_xlat18);
    u_xlat18 = u_xlat18 + (-u_xlat5);
    u_xlat6 = u_xlat0.y + u_xlat5;
    u_xlat0.x = u_xlat6 * 0.0625;
    u_xlat16_5.xyz = _UE42DLutTex.SampleLevel(sampler_UE42DLutTex, u_xlat0.xz, 0.0).xyz;
    u_xlat0.xy = u_xlat0.xz + float2(0.0625, 0.0);
    u_xlat16_0.xyz = _UE42DLutTex.SampleLevel(sampler_UE42DLutTex, u_xlat0.xy, 0.0).xyz;
    u_xlat16_2.xyz = (-u_xlat16_5.xyz) + u_xlat16_0.xyz;
    u_xlat16_2.xyz = ((float3)(u_xlat18)) * u_xlat16_2.xyz + u_xlat16_5.xyz;
    o.SV_Target0.xyz = u_xlat16_2.xyz * float3(1.04999995, 1.04999995, 1.04999995);
    u_xlat16_2.x = dot(u_xlat16_1.xyz, float3(1000.0, 1000.0, 1000.0));
    u_xlat16_2.x = u_xlat16_2.x * _DisabledAlpha + u_xlat16_1.w;
    o.SV_Target0.w = min(u_xlat16_2.x, 1.0);
    return o;

}
ENDHLSL
} } }
