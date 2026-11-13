// Associated EIDs: 977
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0977_Back_Opaque" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_RMCaptured_WorldSpaceCameraPos ("_RMCaptured_WorldSpaceCameraPos", Vector) = (0,0,0,0)
_RMCaptured_unity_WorldTransformParams ("_RMCaptured_unity_WorldTransformParams", Vector) = (0,0,0,0)
_AlbedoMap_ST ("_AlbedoMap_ST", Vector) = (0,0,0,0)
_tSpecShiftMap_ST ("_tSpecShiftMap_ST", Vector) = (0,0,0,0)
_AlbedoColor ("_AlbedoColor", Vector) = (0,0,0,0)
_EmissiveColor ("_EmissiveColor", Vector) = (0,0,0,0)
_MetallicMultiplier ("_MetallicMultiplier", Float) = 0
_RoughnessMultiplier ("_RoughnessMultiplier", Float) = 0
_OcclusionScale ("_OcclusionScale", Float) = 0
_BlendMode ("_BlendMode", Float) = 0
g_EnvmapMipLevelsUsed ("g_EnvmapMipLevelsUsed", Float) = 0
g_EnvmapIntensity ("g_EnvmapIntensity", Vector) = (0,0,0,0)
g_EnvmapRotationParam ("g_EnvmapRotationParam", Vector) = (0,0,0,0)
g_CSMShadowTexture_TexelSize ("g_CSMShadowTexture_TexelSize", Vector) = (0,0,0,0)
g_CSMNormalBias ("g_CSMNormalBias", Vector) = (0,0,0,0)
g_CSMDistanceFade ("g_CSMDistanceFade", Vector) = (0,0,0,0)
g_ExposureScale ("g_ExposureScale", Vector) = (0,0,0,0)
g_FrameIndexModX ("g_FrameIndexModX", Vector) = (0,0,0,0)
g_AlphaOutput ("g_AlphaOutput", Float) = 0
_SwitchSpecular ("_SwitchSpecular", Float) = 0
_SelfGI ("_SelfGI", Vector) = (0,0,0,0)
_SunShiftOffuse ("_SunShiftOffuse", Float) = 0
_SunShift ("_SunShift", Float) = 0
_AnisotropicMultiplier ("_AnisotropicMultiplier", Float) = 0
_SpecularReflectance ("_SpecularReflectance", Vector) = (0,0,0,0)
_AnisotropicMultiplier2 ("_AnisotropicMultiplier2", Float) = 0
_SunShift2 ("_SunShift2", Float) = 0
_SpecularReflectance2 ("_SpecularReflectance2", Vector) = (0,0,0,0)
_AlbedoUVMask ("_AlbedoUVMask", Vector) = (0,0,0,0)
_EmissiveMap_ST ("_EmissiveMap_ST", Vector) = (0,0,0,0)
_EmissiveMapUVMask ("_EmissiveMapUVMask", Vector) = (0,0,0,0)
_NoBlendEmissive ("_NoBlendEmissive", Float) = 0
_NoiseAddAlpha ("_NoiseAddAlpha", Float) = 0
_EmissiveNoiseMap_ST ("_EmissiveNoiseMap_ST", Vector) = (0,0,0,0)
_EmissiveNoiseUVMask ("_EmissiveNoiseUVMask", Vector) = (0,0,0,0)
_EmissiveFlowMap_ST ("_EmissiveFlowMap_ST", Vector) = (0,0,0,0)
_NoiseColor ("_NoiseColor", Vector) = (0,0,0,0)
_EmissiveUVXLerp ("_EmissiveUVXLerp", Float) = 0
_EmissiveUVYLerp ("_EmissiveUVYLerp", Float) = 0
_SpOpacity ("_SpOpacity", Float) = 0
_AlbedoMap ("_AlbedoMap", 2D) = "white" {}
_MaterialParamsMap ("_MaterialParamsMap", 2D) = "white" {}
_tSpecShiftMap ("_tSpecShiftMap", 2D) = "white" {}
_EmissiveFlowMap ("_EmissiveFlowMap", 2D) = "white" {}
_EmissiveNoiseMap ("_EmissiveNoiseMap", 2D) = "white" {}
_EmissiveMap ("_EmissiveMap", 2D) = "white" {}
_NormalMap ("_NormalMap", 2D) = "white" {}
g_EnvmapFilterWithGGX ("g_EnvmapFilterWithGGX", Cube) = "white" {}
g_CSMShadowTexture ("g_CSMShadowTexture", 2D) = "white" {}
hlslcc_zcmpg_CSMShadowTexture ("hlslcc_zcmpg_CSMShadowTexture", 2D) = "white" {}
s_SpecularOcclusionLutBRDFCone ("s_SpecularOcclusionLutBRDFCone", 2D) = "white" {}
s_DFGTexture ("s_DFGTexture", 2D) = "white" {} }
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
float4 _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float3 _RMCaptured_WorldSpaceCameraPos;
float4 _RMCaptured_unity_WorldTransformParams;
float4 _AlbedoMap_ST;
float4 _tSpecShiftMap_ST;
float4 _AlbedoColor;
float4 _EmissiveColor;
float _MetallicMultiplier;
float _RoughnessMultiplier;
float _OcclusionScale;
float _BlendMode;
float g_EnvmapMipLevelsUsed;
float4 g_IrradianceACCoeffs[6];
float4 g_EnvmapIntensity;
float4 g_EnvmapRotationParam;
float4 g_CSMShadowTexture_TexelSize;
float4 _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[8];
float4 g_CSMUvScaleOffset[2];
float4 g_CSMNormalBias;
float2 g_CSMDistanceFade;
float4 g_LightPositionAndFalloff[3];
float4 g_LightIntensityAndAngleScale[3];
float4 g_LightDirectionAndAngleOffset[3];
float4 g_ExposureScale;
float4 g_FrameIndexModX;
float g_AlphaOutput;
float4 _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[4];
int _SwitchSpecular;
float4 _SelfGI;
float _SunShiftOffuse;
float _SunShift;
float _AnisotropicMultiplier;
float3 _SpecularReflectance;
float _AnisotropicMultiplier2;
float _SunShift2;
float3 _SpecularReflectance2;
float4 _AlbedoUVMask;
float4 _EmissiveMap_ST;
float4 _EmissiveMapUVMask;
float _NoBlendEmissive;
float _NoiseAddAlpha;
float4 _EmissiveNoiseMap_ST;
float4 _EmissiveNoiseUVMask;
float4 _EmissiveFlowMap_ST;
float4 _NoiseColor;
float _EmissiveUVXLerp;
float _EmissiveUVYLerp;
float _SpOpacity;
Texture2D _AlbedoMap; SamplerState sampler_AlbedoMap;
Texture2D _MaterialParamsMap; SamplerState sampler_MaterialParamsMap;
Texture2D _tSpecShiftMap; SamplerState sampler_tSpecShiftMap;
Texture2D _EmissiveFlowMap; SamplerState sampler_EmissiveFlowMap;
Texture2D _EmissiveNoiseMap; SamplerState sampler_EmissiveNoiseMap;
Texture2D _EmissiveMap; SamplerState sampler_EmissiveMap;
Texture2D _NormalMap; SamplerState sampler_NormalMap;
TextureCube g_EnvmapFilterWithGGX; SamplerState samplerg_EnvmapFilterWithGGX;
Texture2D g_CSMShadowTexture; SamplerState samplerg_CSMShadowTexture;
Texture2D hlslcc_zcmpg_CSMShadowTexture; SamplerState samplerhlslcc_zcmpg_CSMShadowTexture; // Captured D16 stored as RFloat; explicit <= comparison.
Texture2D s_SpecularOcclusionLutBRDFCone; SamplerState samplers_SpecularOcclusionLutBRDFCone;
Texture2D s_DFGTexture; SamplerState samplers_DFGTexture;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float3 in_NORMAL0 : NORMAL0;
float4 in_TANGENT0 : TANGENT0;
float2 in_TEXCOORD0 : TEXCOORD0;
float4 in_TEXCOORD1 : TEXCOORD1;
float4 in_TEXCOORD2 : TEXCOORD2; };
struct RMVaryings { float4 position : SV_POSITION;
float4 vs_NORMAL0 : TEXCOORD0;
float4 vs_TANGENT0 : TEXCOORD1;
float4 vs_POSITIONWS0 : TEXCOORD2;
float4 vs_TEXCOORD0 : TEXCOORD3;
float4 vs_TEXCOORD1 : TEXCOORD4; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;














float4 u_xlat0;
float4 u_xlat1;
float3 u_xlat2;
float3 u_xlat16_3;
float u_xlat4;
float u_xlat5;
float u_xlat18;
float u_xlat19;


    u_xlat0.w = v.in_TEXCOORD2.w;
    u_xlat1.x = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[0].xyz);
    u_xlat1.y = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[1].xyz);
    u_xlat1.z = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[2].xyz);
    u_xlat19 = dot(u_xlat1.xyz, u_xlat1.xyz);
    u_xlat19 = rsqrt(u_xlat19);
    u_xlat0.xyz = ((float3)(u_xlat19)) * u_xlat1.xyz;
    o.vs_NORMAL0 = u_xlat0;
    u_xlat1.xyz = v.in_TANGENT0.yyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1].xyz;
    u_xlat1.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0].xyz * v.in_TANGENT0.xxx + u_xlat1.xyz;
    u_xlat1.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2].xyz * v.in_TANGENT0.zzz + u_xlat1.xyz;
    u_xlat18 = dot(u_xlat1.xyz, u_xlat1.xyz);
    u_xlat18 = rsqrt(u_xlat18);
    u_xlat1.xyz = ((float3)(u_xlat18)) * u_xlat1.xyz;
    u_xlat1.w = v.in_TANGENT0.w;
    o.vs_TANGENT0 = u_xlat1;
    o.vs_POSITIONWS0.w = 0.0;
    u_xlat2.xyz = v.in_POSITION0.yyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1].xyz;
    u_xlat2.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0].xyz * v.in_POSITION0.xxx + u_xlat2.xyz;
    u_xlat2.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2].xyz * v.in_POSITION0.zzz + u_xlat2.xyz;
    u_xlat2.xyz = u_xlat2.xyz + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3].xyz;
    o.vs_POSITIONWS0.xyz = u_xlat2.xyz;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy;
    o.vs_TEXCOORD0.zw = v.in_TEXCOORD1.xy;
    u_xlat16_3.xyz = u_xlat0.zxy * u_xlat1.yzx;
    u_xlat16_3.xyz = u_xlat0.yzx * u_xlat1.zxy + (-u_xlat16_3.xyz);
    u_xlat16_3.xyz = u_xlat16_3.xyz * v.in_TANGENT0.www;
    u_xlat1.xyz = u_xlat0.yzx * u_xlat16_3.zxy;
    u_xlat1.xyz = u_xlat16_3.yzx * u_xlat0.zxy + (-u_xlat1.xyz);
    u_xlat4 = sin(v.in_TEXCOORD2.y);
    u_xlat5 = cos(v.in_TEXCOORD2.y);
    u_xlat1.xyz = u_xlat1.xyz * ((float3)(u_xlat4));
    u_xlat0.xyz = ((float3)(u_xlat5)) * u_xlat0.xyz + u_xlat1.xyz;
    u_xlat18 = (-v.in_TEXCOORD2.x) * v.in_TEXCOORD2.x + 1.0;
    u_xlat18 = max(u_xlat18, 0.0);
    u_xlat18 = sqrt(u_xlat18);
    u_xlat0.xyz = ((float3)(u_xlat18)) * u_xlat0.xyz;
    u_xlat0.xyz = v.in_TEXCOORD2.xxx * u_xlat16_3.xyz + u_xlat0.xyz;
    u_xlat0.w = v.in_TEXCOORD2.z * 0.636619747;
    u_xlat0.w = clamp(u_xlat0.w, 0.0, 1.0);
    o.vs_TEXCOORD1 = u_xlat0;
    u_xlat0 = u_xlat2.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1];
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0] * u_xlat2.xxxx + u_xlat0;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2] * u_xlat2.zzzz + u_xlat0;
    o.position = u_xlat0 + _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3];
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;

































































float4 u_xlat16_0;
float4 u_xlat16_1;
float4 u_xlat2;
float4 u_xlat16_2;
bool u_xlatb2;
float4 u_xlat16_3;
float3 u_xlat16_4;
float3 u_xlat16_5;
float2 u_xlat6;
float4 u_xlat16_6;
float4 u_xlat7;
float3 u_xlat16_7;
bool3 u_xlatb7;
float3 u_xlat16_8;
float3 u_xlat16_9;
float3 u_xlat16_10;
float4 u_xlat16_11;
float3 u_xlat12;
float2 u_xlat16_12;
float4 u_xlat16_13;
float4 u_xlat14;
float3 u_xlat16_14;
float3 u_xlat15;
float3 u_xlat16;
bool2 u_xlatb16;
float3 u_xlat17;
float u_xlat16_17;
uint u_xlatu17;
float4 u_xlat18;
float4 u_xlat16_19;
float4 u_xlat16_20;
float4 u_xlat16_21;
float3 u_xlat16_22;
float3 u_xlat16_23;
float3 u_xlat16_24;
float3 u_xlat16_25;
float3 u_xlat16_26;
float3 u_xlat16_27;
float3 u_xlat16_28;
float3 u_xlat16_29;
float u_xlat16_31;
float u_xlat32;
bool u_xlatb32;
float u_xlat16_43;
float u_xlat46;
float3 u_xlat47;
float2 u_xlat16_47;
bool u_xlatb47;
float2 u_xlat16_48;
float u_xlat16_51;
float u_xlat16_52;
float u_xlat16_61;
float u_xlat62;
float u_xlat16_62;
bool u_xlatb62;
float2 u_xlat16_65;
float2 u_xlat66;
float u_xlat16_68;
float2 u_xlat75;
float2 u_xlat76;
float u_xlat77;
float u_xlat16_77;
float u_xlat16_81;
float u_xlat16_82;
float u_xlat16_91;
bool u_xlatb92;
float u_xlat16_93;
float u_xlat16_94;
float u_xlat16_95;
float u_xlat97;
bool u_xlatb97;
float u_xlat16_98;
float u_xlat16_99;
float u_xlat16_100;
float u_xlat16_101;
float u_xlat102;
float u_xlat16_102;
bool u_xlatb102;
float u_xlat16_103;
float u_xlat106;
bool u_xlatb106;
float u_xlat16_109;
float u_xlat16_110;
float u_xlat16_111;
float u_xlat16_112;
float u_xlat16_113;


float4 hlslcc_FragCoord = float4(v.position.xyz, 1.0/v.position.w);
    u_xlat16_0 = v.vs_TEXCOORD0 * _AlbedoUVMask.xxyy;
    u_xlat16_0.xy = u_xlat16_0.zw + u_xlat16_0.xy;
    u_xlat16_0.xy = u_xlat16_0.xy * _AlbedoMap_ST.xy + _AlbedoMap_ST.zw;
    u_xlat16_0 = _AlbedoMap.Sample(sampler_AlbedoMap, u_xlat16_0.xy);
    u_xlat16_1.xyz = u_xlat16_0.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_1.xyz = u_xlat16_0.xyz * u_xlat16_1.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_1.xyz = u_xlat16_0.xyz * u_xlat16_1.xyz;
    u_xlat16_2 = _MaterialParamsMap.Sample(sampler_MaterialParamsMap, v.vs_TEXCOORD0.xy);
    u_xlat16_3 = _AlbedoColor + float4(-1.0, -1.0, -1.0, -1.0);
    u_xlat16_3 = u_xlat16_2.wwww * u_xlat16_3 + float4(1.0, 1.0, 1.0, 1.0);
    u_xlat16_4.xyz = u_xlat16_1.xyz * u_xlat16_3.xyz;
    u_xlat16_5.xy = v.vs_TEXCOORD0.xy * _tSpecShiftMap_ST.xy + _tSpecShiftMap_ST.zw;
    u_xlat16_6.xy = _tSpecShiftMap.Sample(sampler_tSpecShiftMap, u_xlat16_5.xy).xy;
    u_xlat16_5.xy = u_xlat16_6.xy * float2(2.0, 2.0) + float2(-1.0, -1.0);
    u_xlat6.xy = v.vs_TEXCOORD0.xy * _EmissiveFlowMap_ST.xy + _EmissiveFlowMap_ST.zw;
    u_xlat16_6.xy = _EmissiveFlowMap.Sample(sampler_EmissiveFlowMap, u_xlat6.xy).xy;
    u_xlat7 = v.vs_TEXCOORD0 * _EmissiveNoiseUVMask.xxyy;
    u_xlat66.xy = u_xlat7.zw + u_xlat7.xy;
    u_xlat16_65.xy = (-float2(_EmissiveUVXLerp, _EmissiveUVYLerp)) + float2(1.0, 1.0);
    u_xlat16_65.xy = u_xlat16_65.xy * u_xlat66.xy;
    u_xlat16_65.xy = u_xlat16_6.xy * float2(_EmissiveUVXLerp, _EmissiveUVYLerp) + u_xlat16_65.xy;
    u_xlat16_65.xy = u_xlat16_65.xy * _EmissiveNoiseMap_ST.xy + _EmissiveNoiseMap_ST.zw;
    u_xlat16_6.xyz = _EmissiveNoiseMap.Sample(sampler_EmissiveNoiseMap, u_xlat16_65.xy).xyz;
    u_xlat16_8.xyz = u_xlat16_6.xyz * _NoiseColor.xyz;
    u_xlat16_9.xyz = u_xlat16_8.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_9.xyz = u_xlat16_8.xyz * u_xlat16_9.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_8.xyz = u_xlat16_8.xyz * u_xlat16_9.xyz;
    u_xlat16_6 = v.vs_TEXCOORD0 * _EmissiveMapUVMask.xxyy;
    u_xlat16_65.xy = u_xlat16_6.zw + u_xlat16_6.xy;
    u_xlat16_65.xy = u_xlat16_65.xy * _EmissiveMap_ST.xy + _EmissiveMap_ST.zw;
    u_xlat16_7.xyz = _EmissiveMap.Sample(sampler_EmissiveMap, u_xlat16_65.xy).xyz;
    u_xlat16_9.xyz = u_xlat16_7.xyz * _EmissiveColor.xyz;
    u_xlat16_10.xyz = u_xlat16_9.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_10.xyz = u_xlat16_9.xyz * u_xlat16_10.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_11.xyz = u_xlat16_9.xyz * u_xlat16_10.xyz;
    u_xlatb92 = _NoBlendEmissive==0.0;
    u_xlat16_11.xyz = u_xlat16_8.xyz * u_xlat16_11.xyz;
    u_xlat16_8.xyz = u_xlat16_9.xyz * u_xlat16_10.xyz + u_xlat16_8.xyz;
    u_xlat16_8.xyz = (bool(u_xlatb92)) ? u_xlat16_11.xyz : u_xlat16_8.xyz;
    u_xlat16_91 = u_xlat16_5.x * _SunShift + _SunShiftOffuse;
    u_xlat16_94 = u_xlat16_5.y * _SunShift2;
    u_xlat16_5.x = u_xlat16_8.y + u_xlat16_8.x;
    u_xlat16_5.x = u_xlat16_8.z + u_xlat16_5.x;
    u_xlat16_5.x = u_xlat16_5.x * _NoiseAddAlpha;
    u_xlat16_5.x = u_xlat16_5.x * 0.330000013;
    u_xlat16_5.x = clamp(u_xlat16_5.x, 0.0, 1.0);
    u_xlat16_0.w = u_xlat16_0.w * u_xlat16_3.w + u_xlat16_5.x;
    u_xlat16_5.xy = u_xlat16_2.xy * float2(_RoughnessMultiplier, _MetallicMultiplier);
    u_xlat16_6.y = max(u_xlat16_5.x, 0.0890000015);
    u_xlat16_93 = u_xlat16_6.y * u_xlat16_6.y;
    u_xlat16_5.x = (-u_xlat16_2.y) * _MetallicMultiplier + 1.0;
    u_xlat16_4.xyz = u_xlat16_4.xyz * u_xlat16_5.xxx;
    u_xlat2.xyw = u_xlat16_1.xyz * u_xlat16_3.xyz + float3(-0.0399999991, -0.0399999991, -0.0399999991);
    u_xlat2.xyw = u_xlat16_5.yyy * u_xlat2.xyw + float3(0.0399999991, 0.0399999991, 0.0399999991);
    u_xlat16_1.x = u_xlat16_2.z * _AnisotropicMultiplier;
    u_xlat16_1.y = u_xlat16_2.z * _AnisotropicMultiplier2;
    u_xlat16_1.xy = u_xlat16_1.xy * float2(2.0, 2.0) + float2(-1.0, -1.0);
    u_xlat16_7.xyz = _NormalMap.Sample(sampler_NormalMap, v.vs_TEXCOORD0.xy).xyz;
    u_xlat16_3.xyz = u_xlat16_7.xyz * float3(2.0, 2.0, 2.0) + float3(-1.0, -1.0, -1.0);
    u_xlat16_61 = dot(u_xlat16_3.xyz, u_xlat16_3.xyz);
    u_xlat16_61 = rsqrt(u_xlat16_61);
    u_xlat16_3.xyz = ((float3)(u_xlat16_61)) * u_xlat16_3.xyz;
    u_xlatb62 = _RMCaptured_unity_WorldTransformParams.w>=0.0;
    u_xlat62 = (u_xlatb62) ? 1.0 : -1.0;
    u_xlat16_61 = u_xlat62 * v.vs_TANGENT0.w;
    u_xlat16_5.x = dot(v.vs_NORMAL0.xyz, v.vs_NORMAL0.xyz);
    u_xlat16_5.x = rsqrt(u_xlat16_5.x);
    u_xlat16_5.xyz = u_xlat16_5.xxx * v.vs_NORMAL0.yzx;
    u_xlat16_95 = dot(v.vs_TANGENT0.yzx, u_xlat16_5.xyz);
    u_xlat16_9.xyz = (-u_xlat16_5.xyz) * ((float3)(u_xlat16_95)) + v.vs_TANGENT0.yzx;
    u_xlat16_95 = dot(u_xlat16_9.xyz, u_xlat16_9.xyz);
    u_xlat16_95 = rsqrt(u_xlat16_95);
    u_xlat16_9.xyz = ((float3)(u_xlat16_95)) * u_xlat16_9.xyz;
    u_xlat16_10.xyz = u_xlat16_5.yzx * u_xlat16_9.xyz;
    u_xlat16_10.xyz = u_xlat16_5.xyz * u_xlat16_9.yzx + (-u_xlat16_10.xyz);
    u_xlat16_10.xyz = ((float3)(u_xlat16_61)) * u_xlat16_10.xyz;
    u_xlat16_10.xyz = u_xlat16_3.yyy * u_xlat16_10.xyz;
    u_xlat16_10.xyz = u_xlat16_3.xxx * u_xlat16_9.zxy + u_xlat16_10.xyz;
    u_xlat16_3.xyz = u_xlat16_3.zzz * u_xlat16_5.zxy + u_xlat16_10.xyz;
    u_xlat16_95 = dot(u_xlat16_9.zxy, u_xlat16_3.xyz);
    u_xlat16_9.xyz = (-u_xlat16_3.yzx) * ((float3)(u_xlat16_95)) + u_xlat16_9.xyz;
    u_xlat16_95 = dot(u_xlat16_9.xyz, u_xlat16_9.xyz);
    u_xlat16_95 = rsqrt(u_xlat16_95);
    u_xlat16_9.xyz = ((float3)(u_xlat16_95)) * u_xlat16_9.xyz;
    u_xlat16_10.xyz = u_xlat16_3.xyz * u_xlat16_9.yzx;
    u_xlat16_10.xyz = u_xlat16_3.zxy * u_xlat16_9.zxy + (-u_xlat16_10.xyz);
    u_xlat16_10.xyz = ((float3)(u_xlat16_61)) * u_xlat16_10.xyz;
    u_xlat7.xyz = (-v.vs_POSITIONWS0.xyz) + _RMCaptured_WorldSpaceCameraPos.xyz;
    u_xlat62 = dot(u_xlat7.xyz, u_xlat7.xyz);
    u_xlat62 = sqrt(u_xlat62);
    u_xlat62 = float(1.0) / u_xlat62;
    u_xlat12.xyz = ((float3)(u_xlat62)) * u_xlat7.xyz;
    u_xlat16_6.x = dot(u_xlat16_3.xyz, u_xlat12.xyz);
    u_xlat16_6.x = clamp(u_xlat16_6.x, 0.0, 1.0);
    u_xlat16_61 = dot(v.vs_TEXCOORD1.xyz, v.vs_TEXCOORD1.xyz);
    u_xlat16_61 = rsqrt(u_xlat16_61);
    u_xlat16_11.xyz = ((float3)(u_xlat16_61)) * v.vs_TEXCOORD1.xyz;
    u_xlat16_61 = v.vs_TEXCOORD1.w;
    u_xlat16_61 = clamp(u_xlat16_61, 0.0, 1.0);
    u_xlat16_95 = v.vs_NORMAL0.w;
    u_xlat16_95 = clamp(u_xlat16_95, 0.0, 1.0);
    u_xlat16_98 = u_xlat16_61 * u_xlat16_61;
    u_xlat16_13.xyz = u_xlat16_3.xyz * ((float3)(u_xlat16_98)) + u_xlat16_11.xyz;
    u_xlat16_98 = dot(u_xlat16_13.xyz, u_xlat16_13.xyz);
    u_xlat16_98 = rsqrt(u_xlat16_98);
    u_xlat16_14.xyz = ((float3)(u_xlat16_98)) * u_xlat16_13.xyz;
    u_xlat16_99 = dot(u_xlat16_11.xyz, u_xlat16_14.xyz);
    u_xlat16_99 = clamp(u_xlat16_99, 0.0, 1.0);
    u_xlat16_11.xyz = u_xlat16_13.xyz * ((float3)(u_xlat16_98)) + (-u_xlat16_3.xyz);
    u_xlat16_11.xyz = float3(float3(_OcclusionScale, _OcclusionScale, _OcclusionScale)) * u_xlat16_11.xyz + u_xlat16_3.xyz;
    u_xlat16_98 = dot(u_xlat16_11.xyz, u_xlat16_11.xyz);
    u_xlat16_98 = rsqrt(u_xlat16_98);
    u_xlat16_11.xyz = ((float3)(u_xlat16_98)) * u_xlat16_11.xyz;
    u_xlat16_61 = u_xlat16_61 * u_xlat16_99 + -1.0;
    u_xlat16_6.w = _OcclusionScale * u_xlat16_61 + 1.0;
    u_xlat16_61 = u_xlat16_95 + -1.0;
    u_xlat16_61 = _OcclusionScale * u_xlat16_61 + 1.0;
    u_xlatb97 = 0.0<g_LightPositionAndFalloff[0].w;
    if(u_xlatb97){
        u_xlat15.xyz = (-v.vs_POSITIONWS0.xyz) + g_LightPositionAndFalloff[0].xyz;
        u_xlat16_95 = dot(u_xlat15.xyz, u_xlat15.xyz);
        u_xlat16_98 = rsqrt(u_xlat16_95);
        u_xlat16_13.xyz = ((float3)(u_xlat16_98)) * u_xlat15.xyz;
        u_xlat16_98 = dot(u_xlat16_13.xyz, g_LightDirectionAndAngleOffset[0].xyz);
        u_xlat16_99 = max(u_xlat16_95, 9.99999975e-05);
        u_xlat16_99 = float(1.0) / u_xlat16_99;
        u_xlat16_95 = u_xlat16_95 * g_LightPositionAndFalloff[0].w;
        u_xlat97 = (-u_xlat16_95) * u_xlat16_95 + 1.0;
        u_xlat97 = max(u_xlat97, 0.0);
        u_xlat16_95 = u_xlat16_98 * abs(g_LightIntensityAndAngleScale[0].w) + g_LightDirectionAndAngleOffset[0].w;
        u_xlat16_95 = clamp(u_xlat16_95, 0.0, 1.0);
        u_xlat16_95 = u_xlat16_95 * u_xlat97;
        u_xlat16_95 = u_xlat16_95 * u_xlat16_95;
        u_xlat16_95 = u_xlat16_99 * u_xlat16_95;
    } else {
        u_xlat16_13.xyz = g_LightDirectionAndAngleOffset[0].xyz;
        u_xlat16_95 = 1.0;
    }
    u_xlat16_98 = dot(u_xlat16_3.xyz, u_xlat16_13.xyz);
    u_xlat16_98 = clamp(u_xlat16_98, 0.0, 1.0);
    u_xlatb97 = g_CSMNormalBias.x<0.0;
    u_xlat102 = v.vs_POSITIONWS0.y * _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[1].w;
    u_xlat102 = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[0].w * v.vs_POSITIONWS0.x + u_xlat102;
    u_xlat102 = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[2].w * v.vs_POSITIONWS0.z + u_xlat102;
    u_xlat102 = u_xlat102 + _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[3].w;
    u_xlat102 = u_xlat102 * g_CSMNormalBias.x;
    u_xlat16_99 = (u_xlatb97) ? abs(u_xlat102) : g_CSMNormalBias.x;
    u_xlat16_99 = u_xlat16_99 + u_xlat16_99;
    u_xlat16_100 = (-u_xlat16_98) + 2.0;
    u_xlat16_99 = u_xlat16_99 * u_xlat16_100;
    u_xlat15.xyz = ((float3)(u_xlat16_99)) * u_xlat16_5.zxy + v.vs_POSITIONWS0.xyz;
    u_xlat14 = u_xlat15.yyyy * _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[1];
    u_xlat14 = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[0] * u_xlat15.xxxx + u_xlat14;
    u_xlat14 = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[2] * u_xlat15.zzzz + u_xlat14;
    u_xlat14 = u_xlat14 + _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[3];
    u_xlat97 = u_xlat14.z * g_CSMDistanceFade.x + g_CSMDistanceFade.y;
    u_xlat97 = clamp(u_xlat97, 0.0, 1.0);
    u_xlatb102 = u_xlat97<1.0;
    if(u_xlatb102){
        u_xlat15.xyz = u_xlat14.xyz / u_xlat14.www;
        u_xlatb16.xy = ((abs(u_xlat15.xyxx)) < ( float4(0.9921875, 0.9921875, 0.0, 0.0))).xy;
        u_xlatb102 = u_xlatb16.y && u_xlatb16.x;
        if(u_xlatb102){
            u_xlat15.xy = u_xlat15.xy * g_CSMUvScaleOffset[0].xy + g_CSMUvScaleOffset[0].zw;
            u_xlat102 = min(u_xlat15.z, 0.999899983);
            u_xlat15.xy = u_xlat15.xy * g_CSMShadowTexture_TexelSize.zw + float2(0.5, 0.5);
            u_xlat75.xy = floor(u_xlat15.xy);
            u_xlat15.xy = (-u_xlat75.xy) + u_xlat15.xy;
            u_xlat75.xy = u_xlat75.xy + float2(-0.5, -0.5);
            u_xlat75.xy = u_xlat75.xy * g_CSMShadowTexture_TexelSize.xy;
            u_xlat16.xy = (-u_xlat15.yx) * float2(2.0, 2.0) + float2(3.0, 3.0);
            u_xlat76.xy = u_xlat15.xy * float2(2.0, 2.0) + float2(1.0, 1.0);
            u_xlat17.xy = (-u_xlat15.xy) + float2(2.0, 2.0);
            u_xlat17.xy = u_xlat17.xy / u_xlat16.yx;
            u_xlat15.xy = u_xlat15.xy / u_xlat76.xy;
            u_xlat14.zw = u_xlat17.xy + float2(-1.0, -1.0);
            u_xlat14.xy = u_xlat15.xy + float2(1.0, 1.0);
            u_xlat17.x = u_xlat16.x * u_xlat16.y;
            u_xlat18 = u_xlat14.zwxw * g_CSMShadowTexture_TexelSize.xyxy + u_xlat75.xyxy;
            float3 txVec0 = float3(u_xlat18.xy,u_xlat102);
            u_xlat16_47.x = step((txVec0).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec0).xy, 0).r);
            u_xlat16.xy = u_xlat16.xy * u_xlat76.xy;
            float3 txVec1 = float3(u_xlat18.zw,u_xlat102);
            u_xlat16_77 = step((txVec1).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec1).xy, 0).r);
            u_xlat16.x = u_xlat16.x * u_xlat16_77;
            u_xlat16.x = u_xlat17.x * u_xlat16_47.x + u_xlat16.x;
            u_xlat14 = u_xlat14.zyxy * g_CSMShadowTexture_TexelSize.xyxy + u_xlat75.xyxy;
            float3 txVec2 = float3(u_xlat14.xy,u_xlat102);
            u_xlat16_17 = step((txVec2).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec2).xy, 0).r);
            u_xlat16.x = u_xlat16.y * u_xlat16_17 + u_xlat16.x;
            u_xlat46 = u_xlat76.y * u_xlat76.x;
            float3 txVec3 = float3(u_xlat14.zw,u_xlat102);
            u_xlat16_102 = step((txVec3).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec3).xy, 0).r);
            u_xlat102 = u_xlat46 * u_xlat16_102 + u_xlat16.x;
            u_xlat16.x = u_xlat102 * 0.0625;
            u_xlat16_100 = (-u_xlat102) * 0.0625 + 1.0;
            u_xlat16_100 = u_xlat97 * u_xlat16_100 + u_xlat16.x;
        } else {
            u_xlat16_100 = 1.0;
        }
    } else {
        u_xlat16_100 = 1.0;
    }
    u_xlatb97 = 0.0<_SpOpacity;
    u_xlatb102 = u_xlat16_0.w==0.0;
    u_xlatb102 = u_xlatb97 && u_xlatb102;
    if(!u_xlatb102){
        u_xlatb16.x = 0.0<u_xlat16_98;
        if(u_xlatb16.x){
            u_xlat16_19.xyz = ((float3)(u_xlat16_95)) * g_LightIntensityAndAngleScale[0].xyz;
            u_xlat16_20.xyz = u_xlat7.xyz * ((float3)(u_xlat62)) + u_xlat16_13.xyz;
            u_xlat16_95 = dot(u_xlat16_20.xyz, u_xlat16_20.xyz);
            u_xlat16_95 = rsqrt(u_xlat16_95);
            u_xlat16_20.xyz = ((float3)(u_xlat16_95)) * u_xlat16_20.xyz;
            u_xlat16_95 = dot(u_xlat16_3.xyz, u_xlat16_20.xyz);
            u_xlat16_95 = clamp(u_xlat16_95, 0.0, 1.0);
            u_xlat16_101 = dot(u_xlat16_13.xyz, u_xlat16_20.xyz);
            u_xlat16_101 = clamp(u_xlat16_101, 0.0, 1.0);
            u_xlat16.xyz = u_xlat16_4.xyz * float3(0.318309873, 0.318309873, 0.318309873);
            u_xlat16_21.xyz = ((float3)(u_xlat16_91)) * u_xlat16_3.xyz + u_xlat16_10.zxy;
            u_xlat16_103 = dot(u_xlat16_21.xyz, u_xlat16_21.xyz);
            u_xlat16_103 = rsqrt(u_xlat16_103);
            u_xlat16_21.xyz = ((float3)(u_xlat16_103)) * u_xlat16_21.xyz;
            u_xlat16_103 = u_xlat16_1.x * u_xlat16_93;
            u_xlat16_109 = u_xlat16_6.y * u_xlat16_6.y + u_xlat16_103;
            u_xlat16_109 = max(u_xlat16_109, 0.00792100001);
            u_xlat16_103 = u_xlat16_6.y * u_xlat16_6.y + (-u_xlat16_103);
            u_xlat16_103 = max(u_xlat16_103, 0.00792100001);
            u_xlat16_110 = dot(u_xlat16_9.zxy, u_xlat16_20.xyz);
            u_xlat16_111 = dot(u_xlat16_21.xyz, u_xlat16_20.xyz);
            u_xlat16_22.x = dot(u_xlat16_9.zxy, u_xlat12.xyz);
            u_xlat16_52 = dot(u_xlat16_21.xyz, u_xlat12.xyz);
            u_xlat16_82 = dot(u_xlat16_9.zxy, u_xlat16_13.xyz);
            u_xlat16_21.x = dot(u_xlat16_21.xyz, u_xlat16_13.xyz);
            u_xlat16_51 = u_xlat16_103 * u_xlat16_109;
            u_xlat17.x = u_xlat16_95 * u_xlat16_51;
            u_xlat16_47.x = u_xlat16_103 * u_xlat16_110;
            u_xlat16_47.y = u_xlat16_109 * u_xlat16_111;
            u_xlat17.yz = u_xlat16_47.xy;
            u_xlat106 = dot(u_xlat17.xyz, u_xlat17.xyz);
            u_xlat106 = u_xlat16_51 / u_xlat106;
            u_xlat17.x = u_xlat106 * u_xlat16_51;
            u_xlat106 = u_xlat106 * u_xlat17.x;
            u_xlat106 = u_xlat106 * 0.318309873;
            u_xlat16_51 = u_xlat16_109 * u_xlat16_22.x;
            u_xlat16_81 = u_xlat16_103 * u_xlat16_52;
            u_xlat16_81 = u_xlat16_81 * u_xlat16_81;
            u_xlat16_51 = u_xlat16_51 * u_xlat16_51 + u_xlat16_81;
            u_xlat16_51 = u_xlat16_6.x * u_xlat16_6.x + u_xlat16_51;
            u_xlat16_51 = sqrt(u_xlat16_51);
            u_xlat16_51 = u_xlat16_6.x + u_xlat16_51;
            u_xlat16_109 = u_xlat16_109 * u_xlat16_82;
            u_xlat16_103 = u_xlat16_103 * u_xlat16_21.x;
            u_xlat16_103 = u_xlat16_103 * u_xlat16_103;
            u_xlat16_103 = u_xlat16_109 * u_xlat16_109 + u_xlat16_103;
            u_xlat16_103 = u_xlat16_98 * u_xlat16_98 + u_xlat16_103;
            u_xlat16_103 = sqrt(u_xlat16_103);
            u_xlat16_103 = u_xlat16_98 + u_xlat16_103;
            u_xlat16_103 = u_xlat16_103 * u_xlat16_51;
            u_xlat16_103 = float(1.0) / u_xlat16_103;
            u_xlat16_101 = (-u_xlat16_101) + 1.0;
            u_xlat16_109 = u_xlat16_101 * u_xlat16_101;
            u_xlat16_109 = u_xlat16_109 * u_xlat16_109;
            u_xlat16_21.x = u_xlat16_101 * u_xlat16_109;
            u_xlat16_51 = u_xlat2.y * 50.0;
            u_xlat16_51 = clamp(u_xlat16_51, 0.0, 1.0);
            u_xlat16_101 = (-u_xlat16_101) * u_xlat16_109 + 1.0;
            u_xlat16_23.xyz = u_xlat2.xyw * ((float3)(u_xlat16_101));
            u_xlat16_21.xyz = ((float3)(u_xlat16_51)) * u_xlat16_21.xxx + u_xlat16_23.xyz;
            u_xlat16_101 = u_xlat16_103 * u_xlat106;
            u_xlat16_23.xyz = ((float3)(u_xlat16_101)) * u_xlat16_21.xyz;
            u_xlat16_23.xyz = u_xlat16_23.xyz * _SpecularReflectance.xyz;
            u_xlatb106 = 0<_SwitchSpecular;
            if(u_xlatb106){
                u_xlat16_24.xyz = ((float3)(u_xlat16_94)) * u_xlat16_3.xyz + u_xlat16_10.zxy;
                u_xlat16_101 = dot(u_xlat16_24.xyz, u_xlat16_24.xyz);
                u_xlat16_101 = rsqrt(u_xlat16_101);
                u_xlat16_24.xyz = ((float3)(u_xlat16_101)) * u_xlat16_24.xyz;
                u_xlat16_101 = u_xlat16_1.y * u_xlat16_93;
                u_xlat16_103 = u_xlat16_6.y * u_xlat16_6.y + u_xlat16_101;
                u_xlat16_103 = max(u_xlat16_103, 0.00792100001);
                u_xlat16_101 = u_xlat16_6.y * u_xlat16_6.y + (-u_xlat16_101);
                u_xlat16_101 = max(u_xlat16_101, 0.00792100001);
                u_xlat16_109 = dot(u_xlat16_24.xyz, u_xlat16_20.xyz);
                u_xlat16_20.x = dot(u_xlat16_24.xyz, u_xlat12.xyz);
                u_xlat16_13.x = dot(u_xlat16_24.xyz, u_xlat16_13.xyz);
                u_xlat16_43 = u_xlat16_101 * u_xlat16_103;
                u_xlat17.x = u_xlat16_95 * u_xlat16_43;
                u_xlat16_47.x = u_xlat16_110 * u_xlat16_101;
                u_xlat16_47.y = u_xlat16_103 * u_xlat16_109;
                u_xlat17.yz = u_xlat16_47.xy;
                u_xlat106 = dot(u_xlat17.xyz, u_xlat17.xyz);
                u_xlat106 = u_xlat16_43 / u_xlat106;
                u_xlat17.x = u_xlat16_43 * u_xlat106;
                u_xlat106 = u_xlat106 * u_xlat17.x;
                u_xlat106 = u_xlat106 * 0.318309873;
                u_xlat16_95 = u_xlat16_103 * u_xlat16_22.x;
                u_xlat16_43 = u_xlat16_101 * u_xlat16_20.x;
                u_xlat16_43 = u_xlat16_43 * u_xlat16_43;
                u_xlat16_95 = u_xlat16_95 * u_xlat16_95 + u_xlat16_43;
                u_xlat16_95 = u_xlat16_6.x * u_xlat16_6.x + u_xlat16_95;
                u_xlat16_95 = sqrt(u_xlat16_95);
                u_xlat16_95 = u_xlat16_95 + u_xlat16_6.x;
                u_xlat16_43 = u_xlat16_103 * u_xlat16_82;
                u_xlat16_101 = u_xlat16_101 * u_xlat16_13.x;
                u_xlat16_101 = u_xlat16_101 * u_xlat16_101;
                u_xlat16_101 = u_xlat16_43 * u_xlat16_43 + u_xlat16_101;
                u_xlat16_101 = u_xlat16_98 * u_xlat16_98 + u_xlat16_101;
                u_xlat16_101 = sqrt(u_xlat16_101);
                u_xlat16_101 = u_xlat16_98 + u_xlat16_101;
                u_xlat16_95 = u_xlat16_95 * u_xlat16_101;
                u_xlat16_95 = float(1.0) / u_xlat16_95;
                u_xlat16_95 = u_xlat16_95 * u_xlat106;
                u_xlat16_13.xyz = ((float3)(u_xlat16_95)) * u_xlat16_21.xyz;
                u_xlat16_23.xyz = u_xlat16_13.xyz * float3(_SpecularReflectance2.x, _SpecularReflectance2.y, _SpecularReflectance2.z) + u_xlat16_23.xyz;
            }
            u_xlat16_95 = u_xlat16_98 * u_xlat16_100;
            u_xlat16_13.xyz = ((float3)(u_xlat16_95)) * u_xlat16_19.xyz;
            u_xlat16_19.xyz = u_xlat16_13.xyz * u_xlat16_23.xyz;
            u_xlat16_13.xyz = u_xlat16.xyz * u_xlat16_13.xyz + u_xlat16_19.xyz;
        } else {
            u_xlat16_19.x = float(0.0);
            u_xlat16_19.y = float(0.0);
            u_xlat16_19.z = float(0.0);
            u_xlat16_13.x = float(0.0);
            u_xlat16_13.y = float(0.0);
            u_xlat16_13.z = float(0.0);
        }
    } else {
        u_xlat16_19.x = float(0.0);
        u_xlat16_19.y = float(0.0);
        u_xlat16_19.z = float(0.0);
        u_xlat16_13.x = float(0.0);
        u_xlat16_13.y = float(0.0);
        u_xlat16_13.z = float(0.0);
    }
    u_xlat16_8.xyz = u_xlat16_8.xyz + u_xlat16_13.xyz;
    u_xlat16.xyz = u_xlat16_4.xyz * float3(0.318309873, 0.318309873, 0.318309873);
    u_xlat16_13.xyz = ((float3)(u_xlat16_91)) * u_xlat16_3.xyz + u_xlat16_10.zxy;
    u_xlat16_95 = dot(u_xlat16_13.xyz, u_xlat16_13.xyz);
    u_xlat16_95 = rsqrt(u_xlat16_95);
    u_xlat16_13.xyz = ((float3)(u_xlat16_95)) * u_xlat16_13.xyz;
    u_xlat16_20.xy = u_xlat16_1.xy * ((float2)(u_xlat16_93));
    u_xlat16_20.zw = u_xlat16_6.yy * u_xlat16_6.yy + u_xlat16_20.xy;
    u_xlat16_20.xy = u_xlat16_6.yy * u_xlat16_6.yy + (-u_xlat16_20.xy);
    u_xlat16_20 = max(u_xlat16_20, float4(0.00792100001, 0.00792100001, 0.00792100001, 0.00792100001));
    u_xlat16_31 = dot(u_xlat16_9.zxy, u_xlat12.xyz);
    u_xlat16_95 = dot(u_xlat16_13.xyz, u_xlat12.xyz);
    u_xlat16_21.xy = u_xlat16_20.zw * ((float2)(u_xlat16_31));
    u_xlat16_21.xy = u_xlat16_21.xy * u_xlat16_21.xy;
    u_xlat16_31 = u_xlat16_20.x * u_xlat16_95;
    u_xlat16_31 = u_xlat16_31 * u_xlat16_31 + u_xlat16_21.x;
    u_xlat16_95 = u_xlat16_6.w * u_xlat16_6.w;
    u_xlat16_31 = u_xlat16_6.x * u_xlat16_6.x + u_xlat16_31;
    u_xlat16_31 = sqrt(u_xlat16_31);
    u_xlat16_31 = u_xlat16_31 + u_xlat16_6.x;
    u_xlat16_98 = u_xlat2.y * 50.0;
    u_xlat16_98 = clamp(u_xlat16_98, 0.0, 1.0);
    u_xlatb106 = 0<_SwitchSpecular;
    u_xlat16_21.xzw = ((float3)(u_xlat16_94)) * u_xlat16_3.xyz + u_xlat16_10.zxy;
    u_xlat16_94 = dot(u_xlat16_21.xzw, u_xlat16_21.xzw);
    u_xlat16_94 = rsqrt(u_xlat16_94);
    u_xlat16_21.xzw = ((float3)(u_xlat16_94)) * u_xlat16_21.xzw;
    u_xlat16_94 = dot(u_xlat16_21.xzw, u_xlat12.xyz);
    u_xlat16_22.xy = u_xlat16_20.xy * u_xlat16_20.zw;
    u_xlat16_94 = u_xlat16_20.y * u_xlat16_94;
    u_xlat16_94 = u_xlat16_94 * u_xlat16_94 + u_xlat16_21.y;
    u_xlat16_94 = u_xlat16_6.x * u_xlat16_6.x + u_xlat16_94;
    u_xlat16_94 = sqrt(u_xlat16_94);
    u_xlat16_94 = u_xlat16_94 + u_xlat16_6.x;
    u_xlat16_23.xyz = u_xlat16_19.xyz;
    u_xlat16_24.xyz = u_xlat16_8.xyz;
    for(uint u_xlatu_loop_1 = 1u ; u_xlatu_loop_1<3u ; u_xlatu_loop_1++)
    {
        u_xlatb47 = 0.0<g_LightPositionAndFalloff[int(u_xlatu_loop_1)].w;
        if(u_xlatb47){
            u_xlat47.xyz = (-v.vs_POSITIONWS0.xyz) + g_LightPositionAndFalloff[int(u_xlatu_loop_1)].xyz;
            u_xlat16_100 = dot(u_xlat47.xyz, u_xlat47.xyz);
            u_xlat16_101 = rsqrt(u_xlat16_100);
            u_xlat16_25.xyz = ((float3)(u_xlat16_101)) * u_xlat47.xyz;
            u_xlat16_101 = dot(u_xlat16_25.xyz, g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].xyz);
            u_xlat16_103 = max(u_xlat16_100, 9.99999975e-05);
            u_xlat16_103 = float(1.0) / u_xlat16_103;
            u_xlat16_100 = u_xlat16_100 * g_LightPositionAndFalloff[int(u_xlatu_loop_1)].w;
            u_xlat47.x = (-u_xlat16_100) * u_xlat16_100 + 1.0;
            u_xlat47.x = max(u_xlat47.x, 0.0);
            u_xlat16_100 = u_xlat16_101 * g_LightIntensityAndAngleScale[int(u_xlatu_loop_1)].w + g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].w;
            u_xlat16_100 = clamp(u_xlat16_100, 0.0, 1.0);
            u_xlat16_100 = u_xlat16_100 * u_xlat47.x;
            u_xlat16_100 = u_xlat16_100 * u_xlat16_100;
            u_xlat16_100 = u_xlat16_103 * u_xlat16_100;
        } else {
            u_xlat16_25.xyz = g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].xyz;
            u_xlat16_100 = 1.0;
        }
        if(!u_xlatb102){
            u_xlat16_101 = dot(u_xlat16_3.xyz, u_xlat16_25.xyz);
            u_xlat16_101 = clamp(u_xlat16_101, 0.0, 1.0);
            u_xlatb47 = 0.0<u_xlat16_101;
            if(u_xlatb47){
                u_xlat16_26.xyz = ((float3)(u_xlat16_100)) * g_LightIntensityAndAngleScale[int(u_xlatu_loop_1)].xyz;
                u_xlat16_27.xyz = u_xlat7.xyz * ((float3)(u_xlat62)) + u_xlat16_25.xyz;
                u_xlat16_100 = dot(u_xlat16_27.xyz, u_xlat16_27.xyz);
                u_xlat16_100 = rsqrt(u_xlat16_100);
                u_xlat16_27.xyz = ((float3)(u_xlat16_100)) * u_xlat16_27.xyz;
                u_xlat16_100 = dot(u_xlat16_3.xyz, u_xlat16_27.xyz);
                u_xlat16_100 = clamp(u_xlat16_100, 0.0, 1.0);
                u_xlat16_103 = dot(u_xlat16_25.xyz, u_xlat16_27.xyz);
                u_xlat16_103 = clamp(u_xlat16_103, 0.0, 1.0);
                u_xlat16_109 = dot(u_xlat16_9.zxy, u_xlat16_27.xyz);
                u_xlat16_51 = dot(u_xlat16_13.xyz, u_xlat16_27.xyz);
                u_xlat16_82 = dot(u_xlat16_9.zxy, u_xlat16_25.xyz);
                u_xlat16_112 = dot(u_xlat16_13.xyz, u_xlat16_25.xyz);
                u_xlat18.x = u_xlat16_100 * u_xlat16_22.x;
                u_xlat16_48.x = u_xlat16_109 * u_xlat16_20.x;
                u_xlat16_48.y = u_xlat16_20.z * u_xlat16_51;
                u_xlat18.yz = u_xlat16_48.xy;
                u_xlat47.x = dot(u_xlat18.xyz, u_xlat18.xyz);
                u_xlat47.x = u_xlat16_22.x / u_xlat47.x;
                u_xlat77 = u_xlat47.x * u_xlat16_22.x;
                u_xlat47.x = u_xlat47.x * u_xlat77;
                u_xlat47.x = u_xlat47.x * 0.318309873;
                u_xlat16_51 = u_xlat16_20.z * u_xlat16_82;
                u_xlat16_112 = u_xlat16_20.x * u_xlat16_112;
                u_xlat16_112 = u_xlat16_112 * u_xlat16_112;
                u_xlat16_51 = u_xlat16_51 * u_xlat16_51 + u_xlat16_112;
                u_xlat16_51 = u_xlat16_101 * u_xlat16_101 + u_xlat16_51;
                u_xlat16_51 = sqrt(u_xlat16_51);
                u_xlat16_51 = u_xlat16_101 + u_xlat16_51;
                u_xlat16_51 = u_xlat16_31 * u_xlat16_51;
                u_xlat16_51 = float(1.0) / u_xlat16_51;
                u_xlat16_103 = (-u_xlat16_103) + 1.0;
                u_xlat16_112 = u_xlat16_103 * u_xlat16_103;
                u_xlat16_112 = u_xlat16_112 * u_xlat16_112;
                u_xlat16_113 = u_xlat16_103 * u_xlat16_112;
                u_xlat16_103 = (-u_xlat16_103) * u_xlat16_112 + 1.0;
                u_xlat16_28.xyz = u_xlat2.xyw * ((float3)(u_xlat16_103));
                u_xlat16_28.xyz = ((float3)(u_xlat16_98)) * ((float3)(u_xlat16_113)) + u_xlat16_28.xyz;
                u_xlat16_103 = u_xlat47.x * u_xlat16_51;
                u_xlat16_29.xyz = ((float3)(u_xlat16_103)) * u_xlat16_28.xyz;
                u_xlat16_29.xyz = u_xlat16_29.xyz * _SpecularReflectance.xyz;
                if(u_xlatb106){
                    u_xlat16_103 = dot(u_xlat16_21.xzw, u_xlat16_27.xyz);
                    u_xlat16_51 = dot(u_xlat16_21.xzw, u_xlat16_25.xyz);
                    u_xlat18.x = u_xlat16_100 * u_xlat16_22.y;
                    u_xlat16_48.x = u_xlat16_109 * u_xlat16_20.y;
                    u_xlat16_48.y = u_xlat16_103 * u_xlat16_20.w;
                    u_xlat18.yz = u_xlat16_48.xy;
                    u_xlat47.x = dot(u_xlat18.xyz, u_xlat18.xyz);
                    u_xlat47.x = u_xlat16_22.y / u_xlat47.x;
                    u_xlat77 = u_xlat47.x * u_xlat16_22.y;
                    u_xlat47.x = u_xlat47.x * u_xlat77;
                    u_xlat47.x = u_xlat47.x * 0.318309873;
                    u_xlat16_100 = u_xlat16_20.w * u_xlat16_82;
                    u_xlat16_103 = u_xlat16_20.y * u_xlat16_51;
                    u_xlat16_103 = u_xlat16_103 * u_xlat16_103;
                    u_xlat16_100 = u_xlat16_100 * u_xlat16_100 + u_xlat16_103;
                    u_xlat16_100 = u_xlat16_101 * u_xlat16_101 + u_xlat16_100;
                    u_xlat16_100 = sqrt(u_xlat16_100);
                    u_xlat16_100 = u_xlat16_100 + u_xlat16_101;
                    u_xlat16_100 = u_xlat16_94 * u_xlat16_100;
                    u_xlat16_100 = float(1.0) / u_xlat16_100;
                    u_xlat16_100 = u_xlat16_100 * u_xlat47.x;
                    u_xlat16_25.xyz = ((float3)(u_xlat16_100)) * u_xlat16_28.xyz;
                    u_xlat16_29.xyz = u_xlat16_25.xyz * float3(_SpecularReflectance2.x, _SpecularReflectance2.y, _SpecularReflectance2.z) + u_xlat16_29.xyz;
                }
                u_xlat16_25.xyz = ((float3)(u_xlat16_101)) * u_xlat16_26.xyz;
                u_xlat16_26.xyz = u_xlat16_25.xyz * u_xlat16_29.xyz;
                u_xlat16_23.xyz = u_xlat16_29.xyz * u_xlat16_25.xyz + u_xlat16_23.xyz;
                u_xlat16_25.xyz = u_xlat16.xyz * u_xlat16_25.xyz + u_xlat16_26.xyz;
            } else {
                u_xlat16_25.x = float(0.0);
                u_xlat16_25.y = float(0.0);
                u_xlat16_25.z = float(0.0);
            }
        } else {
            u_xlat16_25.x = float(0.0);
            u_xlat16_25.y = float(0.0);
            u_xlat16_25.z = float(0.0);
        }
        u_xlat16_24.xyz = u_xlat16_24.xyz + u_xlat16_25.xyz;
    }
    u_xlat16_8.x = dot(g_EnvmapRotationParam.xy, u_xlat16_3.xz);
    u_xlat16_8.z = dot(g_EnvmapRotationParam.zw, u_xlat16_3.xz);
    u_xlat16_13.x = dot(g_EnvmapRotationParam.xy, u_xlat16_11.xz);
    u_xlat16_13.z = dot(g_EnvmapRotationParam.zw, u_xlat16_11.xz);
    u_xlat16_8.y = u_xlat16_3.y;
    u_xlat16_13.y = u_xlat16_11.y;
    u_xlat16_19.xyz = u_xlat16_8.xyz * u_xlat16_13.xyz;
    u_xlat16_19.xyz = clamp(u_xlat16_19.xyz, 0.0, 1.0);
    u_xlat16_20.xyz = u_xlat16_13.xyz * u_xlat16_13.xyz;
    u_xlat16_31 = dot(u_xlat16_13.xyz, u_xlat16_8.xyz);
    u_xlat16_94 = (-u_xlat16_31) + 1.0;
    u_xlat16_94 = clamp(u_xlat16_94, 0.0, 1.0);
    u_xlat16_31 = (-u_xlat16_31) * u_xlat16_95 + 1.0;
    u_xlat16_31 = clamp(u_xlat16_31, 0.0, 1.0);
    u_xlat16_31 = u_xlat16_31 * 0.400000006;
    u_xlat16_94 = u_xlat16_6.w * u_xlat16_94;
    u_xlat16_8.xyz = ((float3)(u_xlat16_31)) * _SelfGI.xyz;
    u_xlat16_8.xyz = ((float3)(u_xlat16_94)) * float3(0.5, 0.5, 0.5) + u_xlat16_8.xyz;
    u_xlat16_19.xyz = ((float3)(u_xlat16_95)) * u_xlat16_19.xyz;
    u_xlat16_21.xyz = u_xlat16_8.xyz * u_xlat16_20.xxx + u_xlat16_19.xxx;
    u_xlat16_19.xyw = u_xlat16_8.xyz * u_xlat16_20.yyy + u_xlat16_19.yyy;
    u_xlat16_8.xyz = u_xlat16_8.xyz * u_xlat16_20.zzz + u_xlat16_19.zzz;
    u_xlatb7.xyz = ((u_xlat16_13.xyzx) >= ( float4(0.0, 0.0, 0.0, 0.0))).xyz;
    u_xlat16_13.x = (u_xlatb7.x) ? float(1.0) : float(0.0);
    u_xlat16_13.y = (u_xlatb7.y) ? float(1.0) : float(0.0);
    u_xlat16_13.z = (u_xlatb7.z) ? float(1.0) : float(0.0);
    u_xlat16_20.x = (u_xlatb7.x) ? float(0.0) : float(1.0);
    u_xlat16_20.y = (u_xlatb7.y) ? float(0.0) : float(1.0);
    u_xlat16_20.z = (u_xlatb7.z) ? float(0.0) : float(1.0);
    u_xlat16_22.xyz = u_xlat16_21.xyz * g_IrradianceACCoeffs[0].xyz;
    u_xlat16_21.xyz = u_xlat16_21.xyz * g_IrradianceACCoeffs[1].xyz;
    u_xlat16_21.xyz = u_xlat16_20.xxx * u_xlat16_21.xyz;
    u_xlat16_21.xyz = u_xlat16_22.xyz * u_xlat16_13.xxx + u_xlat16_21.xyz;
    u_xlat16_22.xyz = u_xlat16_19.xyw * g_IrradianceACCoeffs[2].xyz;
    u_xlat16_13.xyw = u_xlat16_22.xyz * u_xlat16_13.yyy + u_xlat16_21.xyz;
    u_xlat16_19.xyz = u_xlat16_19.xyw * g_IrradianceACCoeffs[3].xyz;
    u_xlat16_13.xyw = u_xlat16_19.xyz * u_xlat16_20.yyy + u_xlat16_13.xyw;
    u_xlat16_19.xyz = u_xlat16_8.xyz * g_IrradianceACCoeffs[4].xyz;
    u_xlat16_13.xyz = u_xlat16_19.xyz * u_xlat16_13.zzz + u_xlat16_13.xyw;
    u_xlat16_8.xyz = u_xlat16_8.xyz * g_IrradianceACCoeffs[5].xyz;
    u_xlat16_8.xyz = u_xlat16_8.xyz * u_xlat16_20.zzz + u_xlat16_13.xyz;
    u_xlat16_8.xyz = ((float3)(u_xlat16_61)) * u_xlat16_8.xyz;
    u_xlat16_8.xyz = u_xlat16_8.xyz * g_EnvmapIntensity.xyz;
    u_xlat16_8.xyz = (bool(u_xlatb102)) ? float3(0.0, 0.0, 0.0) : u_xlat16_8.xyz;
    u_xlat16_4.xyz = u_xlat16_8.xyz * u_xlat16_4.xyz + u_xlat16_24.xyz;
    if(!u_xlatb102){
        u_xlat16_31 = dot(u_xlat16_8.xyz, float3(0.333299994, 0.333299994, 0.333299994));
        u_xlat16_94 = dot(u_xlat16_11.xyz, u_xlat16_3.xyz);
        u_xlat16_94 = clamp(u_xlat16_94, 0.0, 1.0);
        u_xlat16_8.x = u_xlat16_94 * 0.5 + 0.5;
        u_xlat16_8.x = (-u_xlat16_94) + u_xlat16_8.x;
        u_xlat16_94 = u_xlat16_6.w * u_xlat16_8.x + u_xlat16_94;
        u_xlat16_94 = u_xlat16_95 * u_xlat16_94;
        u_xlat16_94 = u_xlat16_61 * u_xlat16_94;
        u_xlat16_95 = dot((-u_xlat12.xyz), u_xlat16_3.xyz);
        u_xlat16_95 = u_xlat16_95 + u_xlat16_95;
        u_xlat16_8.xyz = u_xlat16_3.xyz * (-((float3)(u_xlat16_95))) + (-u_xlat12.xyz);
        u_xlat16_6.z = dot(u_xlat16_11.xyz, u_xlat16_8.xyz);
        u_xlat16_8.xyz = u_xlat16_6.yzw * float3(1.09769487, 0.5, 1.0) + float3(-0.097694844, 0.5, -0.0);
        u_xlat16_8.xyz = clamp(u_xlat16_8.xyz, 0.0, 1.0);
        u_xlat16_95 = u_xlat16_8.z * 15.0;
        u_xlat16_95 = floor(u_xlat16_95);
        u_xlat16_100 = u_xlat16_95 + 1.0;
        u_xlat16_100 = min(u_xlat16_100, 15.0);
        u_xlat16_68 = u_xlat16_8.z * 15.0 + (-u_xlat16_95);
        u_xlat16_11.x = u_xlat16_95 * 16.0;
        u_xlat16_11.y = float(0.0);
        u_xlat16_11.w = float(0.0);
        u_xlat16_11.z = u_xlat16_100 * 16.0;
        u_xlat16_11 = u_xlat16_8.xyxy * float4(15.0, 15.0, 15.0, 15.0) + u_xlat16_11;
        u_xlat16_11 = u_xlat16_11 * float4(0.00390625, 0.0625, 0.00390625, 0.0625) + float4(0.001953125, 0.03125, 0.001953125, 0.03125);
        u_xlat16_62 = s_SpecularOcclusionLutBRDFCone.Sample(samplers_SpecularOcclusionLutBRDFCone, u_xlat16_11.xy).x;
        u_xlat16_7.x = s_SpecularOcclusionLutBRDFCone.Sample(samplers_SpecularOcclusionLutBRDFCone, u_xlat16_11.zw).x;
        u_xlat16_95 = (-u_xlat16_62) + u_xlat16_7.x;
        u_xlat16_95 = u_xlat16_68 * u_xlat16_95 + u_xlat16_62;
        u_xlat16_61 = u_xlat16_61 * u_xlat16_95;
        u_xlat16_95 = u_xlat16_94 * 0.5;
        u_xlat16_8.x = (-u_xlat16_94) * 0.5 + 1.0;
        u_xlat16_61 = u_xlat16_61 * u_xlat16_8.x + u_xlat16_95;
        u_xlat16_8.xyz = ((float3)(u_xlat16_61)) * _SelfGI.xyz;
        u_xlat16_11.xyz = u_xlat16_8.xyz + u_xlat16_8.xyz;
        u_xlat16_8.xyz = (-u_xlat16_8.xyz) * float3(2.0, 2.0, 2.0) + float3(1.0, 1.0, 1.0);
        u_xlat16_8.xyz = ((float3)(u_xlat16_61)) * u_xlat16_8.xyz + u_xlat16_11.xyz;
        u_xlat16_8.xyz = ((float3)(u_xlat16_94)) * u_xlat16_8.xyz;
        u_xlat16_5.xyz = ((float3)(u_xlat16_91)) * u_xlat16_5.xyz + u_xlat16_10.xyz;
        u_xlat16_61 = dot(u_xlat16_5.xyz, u_xlat16_5.xyz);
        u_xlat16_61 = rsqrt(u_xlat16_61);
        u_xlat16_5.xyz = ((float3)(u_xlat16_61)) * u_xlat16_5.xyz;
        u_xlat16_61 = -abs(u_xlat16_1.x) * 0.800000012 + 1.0;
        u_xlat16_61 = u_xlat16_61 * u_xlat16_6.y;
        u_xlat16_61 = u_xlat16_61 * g_EnvmapMipLevelsUsed + (-u_xlat16_61);
        u_xlatb62 = u_xlat16_1.x>=0.0;
        u_xlat16_5.xyz = (bool(u_xlatb62)) ? u_xlat16_5.xyz : u_xlat16_9.xyz;
        u_xlat16_10.xyz = u_xlat12.xyz * u_xlat16_5.xyz;
        u_xlat16_10.xyz = u_xlat16_5.zxy * u_xlat12.yzx + (-u_xlat16_10.xyz);
        u_xlat16_11.xyz = u_xlat16_5.xyz * u_xlat16_10.xyz;
        u_xlat16_5.xyz = u_xlat16_10.zxy * u_xlat16_5.yzx + (-u_xlat16_11.xyz);
        u_xlat16_91 = u_xlat16_93 * 8.0;
        u_xlat16_91 = min(u_xlat16_91, 1.0);
        u_xlat16_91 = u_xlat16_91 * abs(u_xlat16_1.x);
        u_xlat16_5.xyz = (-u_xlat16_3.xyz) + u_xlat16_5.xyz;
        u_xlat16_5.xyz = ((float3)(u_xlat16_91)) * u_xlat16_5.xyz + u_xlat16_3.xyz;
        u_xlat16_91 = dot(u_xlat16_5.xyz, u_xlat16_5.xyz);
        u_xlat16_91 = rsqrt(u_xlat16_91);
        u_xlat16_5.xyz = ((float3)(u_xlat16_91)) * u_xlat16_5.xyz;
        u_xlat16_91 = dot((-u_xlat12.xyz), u_xlat16_5.xyz);
        u_xlat16_91 = u_xlat16_91 + u_xlat16_91;
        u_xlat16_5.xyz = u_xlat16_5.xyz * (-((float3)(u_xlat16_91))) + (-u_xlat12.xyz);
        u_xlat16_91 = u_xlat16_93 * u_xlat16_93;
        u_xlat16_3.xyz = u_xlat16_3.xyz + (-u_xlat16_5.xyz);
        u_xlat16_3.xyz = ((float3)(u_xlat16_91)) * u_xlat16_3.xyz + u_xlat16_5.xyz;
        u_xlat16_5.xyz = (-u_xlat16_3.xyz) + u_xlat16_5.xyz;
        u_xlat16_3.xyz = abs(u_xlat16_1.xxx) * u_xlat16_5.xyz + u_xlat16_3.xyz;
        u_xlat16_5.x = dot(g_EnvmapRotationParam.xy, u_xlat16_3.xz);
        u_xlat16_5.z = dot(g_EnvmapRotationParam.zw, u_xlat16_3.xz);
        u_xlat16_5.y = u_xlat16_3.y;
        u_xlat16_3 = g_EnvmapFilterWithGGX.SampleLevel(samplerg_EnvmapFilterWithGGX, u_xlat16_5.xyz, u_xlat16_61);
        u_xlat16_1.x = u_xlat16_3.w * 6.0;
        u_xlat16_1.xzw = u_xlat16_1.xxx * u_xlat16_3.xyz;
        u_xlat16_1.xzw = u_xlat16_1.xzw * u_xlat16_1.xzw;
        u_xlat16_1.xzw = u_xlat16_1.xzw * g_EnvmapIntensity.xyz;
        u_xlat16_1.xzw = u_xlat16_8.xyz * u_xlat16_1.xzw;
        u_xlatb62 = 0.0<g_EnvmapIntensity.w;
        u_xlat62 = (u_xlatb62) ? u_xlat16_31 : 1.0;
        u_xlat7.xyz = u_xlat16_1.xzw * ((float3)(u_xlat62));
        u_xlat16_12.xy = s_DFGTexture.Sample(samplers_DFGTexture, u_xlat16_6.xy).xy;
        u_xlat16_1.x = u_xlat16_98 * u_xlat16_12.y;
        u_xlat16_1.xyz = u_xlat2.xyw * u_xlat16_12.xxx + u_xlat16_1.xxx;
        u_xlat16_5.xyz = u_xlat16_1.xyz * u_xlat7.xyz;
        u_xlat16_1.xyz = u_xlat7.xyz * u_xlat16_1.xyz + u_xlat16_23.xyz;
        u_xlat16_1.x = dot(u_xlat16_1.xyz, float3(0.219999999, 0.707000017, 0.0710000023));
        u_xlat16_1.x = clamp(u_xlat16_1.x, 0.0, 1.0);
        u_xlat16_1.x = u_xlat16_0.w + u_xlat16_1.x;
        u_xlat16_0.w = (u_xlatb97) ? u_xlat16_1.x : u_xlat16_0.w;
    } else {
        u_xlat16_5.x = float(0.0);
        u_xlat16_5.y = float(0.0);
        u_xlat16_5.z = float(0.0);
    }
    u_xlat16_1.xyz = u_xlat16_4.xyz + u_xlat16_5.xyz;
    u_xlat16_4.xyz = u_xlat16_1.yyy * _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[1].xyz;
    u_xlat16_1.xyw = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[0].xyz * u_xlat16_1.xxx + u_xlat16_4.xyz;
    u_xlat16_1.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[2].xyz * u_xlat16_1.zzz + u_xlat16_1.xyw;
    u_xlat16_1.xyz = u_xlat16_1.xyz + _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].xyz;
    u_xlatb2 = 0.0>=_RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].w;
    u_xlat16_4.xyz = u_xlat16_1.xyz + float3(0.155000001, 0.155000001, 0.155000001);
    u_xlat16_4.xyz = u_xlat16_1.xyz / u_xlat16_4.xyz;
    u_xlat16_3.xyz = u_xlat16_4.xyz * float3(1.01900005, 1.01900005, 1.01900005);
    u_xlatb32 = 0.0>=_BlendMode;
    u_xlatb62 = g_AlphaOutput<1.0;
    u_xlatb32 = u_xlatb62 && u_xlatb32;
    u_xlat16_91 = dot(u_xlat16_3.xyz, float3(0.333299994, 0.333299994, 0.333299994));
    u_xlat16_3.w = (u_xlatb32) ? u_xlat16_91 : u_xlat16_0.w;
    u_xlat16_0.xyz = min(u_xlat16_1.xyz, float3(65504.0, 65504.0, 65504.0));
    u_xlat16_0 = (bool(u_xlatb2)) ? u_xlat16_3 : u_xlat16_0;
    u_xlatb2 = 2.0<g_ExposureScale.w;
    u_xlat7.xy = hlslcc_FragCoord.xy;
    u_xlat7.z = g_FrameIndexModX.x;
    u_xlat32 = dot(u_xlat7.xyz, float3(0.117647059, 0.411764711, 1.35294116));
    u_xlat32 = frac(u_xlat32);
    u_xlat16_1.xyz = ((float3)(u_xlat32)) * float3(0.00392156886, 0.00392156886, 0.00392156886) + u_xlat16_0.xyz;
    o.SV_Target0.xyz = (bool(u_xlatb2)) ? u_xlat16_1.xyz : u_xlat16_0.xyz;
    o.SV_Target0.w = u_xlat16_0.w;
    return o;

}
ENDHLSL
} } }
