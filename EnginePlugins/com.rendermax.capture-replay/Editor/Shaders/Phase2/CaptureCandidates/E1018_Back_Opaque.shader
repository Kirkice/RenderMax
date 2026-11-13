// Associated EIDs: 1018, 1029
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E1018_Back_Opaque" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_RMCaptured_WorldSpaceCameraPos ("_RMCaptured_WorldSpaceCameraPos", Vector) = (0,0,0,0)
g_FogParam ("g_FogParam", Vector) = (0,0,0,0)
g_FogColor ("g_FogColor", Vector) = (0,0,0,0)
_OcclusionScale ("_OcclusionScale", Float) = 0
_ShadowStrength ("_ShadowStrength", Float) = 0
_RMCaptured_Time ("_RMCaptured_Time", Vector) = (0,0,0,0)
_RMCaptured_unity_WorldTransformParams ("_RMCaptured_unity_WorldTransformParams", Vector) = (0,0,0,0)
g_EnvmapMipLevelsUsed ("g_EnvmapMipLevelsUsed", Float) = 0
g_EnvmapIntensity ("g_EnvmapIntensity", Vector) = (0,0,0,0)
g_EnvmapRotationParam ("g_EnvmapRotationParam", Vector) = (0,0,0,0)
g_CSMShadowTexture_TexelSize ("g_CSMShadowTexture_TexelSize", Vector) = (0,0,0,0)
g_CSMNormalBias ("g_CSMNormalBias", Vector) = (0,0,0,0)
g_CSMDistanceFade ("g_CSMDistanceFade", Vector) = (0,0,0,0)
g_ExposureScale ("g_ExposureScale", Vector) = (0,0,0,0)
g_FrameIndexModX ("g_FrameIndexModX", Vector) = (0,0,0,0)
g_AlphaOutput ("g_AlphaOutput", Float) = 0
_AlbedoColor ("_AlbedoColor", Vector) = (0,0,0,0)
_EmissiveColor ("_EmissiveColor", Vector) = (0,0,0,0)
_BlendMode ("_BlendMode", Float) = 0
_ReflectEnvColor ("_ReflectEnvColor", Vector) = (0,0,0,0)
_RefractEnvColor ("_RefractEnvColor", Vector) = (0,0,0,0)
_AOMinMaxRemap ("_AOMinMaxRemap", Vector) = (0,0,0,0)
_MetallicMinMaxRemap ("_MetallicMinMaxRemap", Vector) = (0,0,0,0)
_RoughnessMinMaxRemap ("_RoughnessMinMaxRemap", Vector) = (0,0,0,0)
_OpacityMinMaxRemap ("_OpacityMinMaxRemap", Vector) = (0,0,0,0)
_RoughnessChSwizzle ("_RoughnessChSwizzle", Vector) = (0,0,0,0)
_MetallicChSwizzle ("_MetallicChSwizzle", Vector) = (0,0,0,0)
_OcclusionChSwizzle ("_OcclusionChSwizzle", Vector) = (0,0,0,0)
_SelfGI ("_SelfGI", Vector) = (0,0,0,0)
_Scale_Thickness ("_Scale_Thickness", Float) = 0
_Scale_Refraction ("_Scale_Refraction", Float) = 0
_TransColor ("_TransColor", Vector) = (0,0,0,0)
_NormalFlatten ("_NormalFlatten", Float) = 0
_MotionSpeed ("_MotionSpeed", Float) = 0
_ThicknessMap_ST ("_ThicknessMap_ST", Vector) = (0,0,0,0)
_DetailNormalMap_ST ("_DetailNormalMap_ST", Vector) = (0,0,0,0)
_PBRRef ("_PBRRef", Float) = 0
_AlbedoMap ("_AlbedoMap", 2D) = "white" {}
_MaterialParamsMap ("_MaterialParamsMap", 2D) = "white" {}
_EmissiveMap ("_EmissiveMap", 2D) = "white" {}
_NormalMap ("_NormalMap", 2D) = "white" {}
_ThicknessMap ("_ThicknessMap", 2D) = "white" {}
_DetailNormalMap ("_DetailNormalMap", 2D) = "white" {}
g_EnvmapFilterWithGGX ("g_EnvmapFilterWithGGX", Cube) = "white" {}
g_CSMShadowTexture ("g_CSMShadowTexture", 2D) = "white" {}
hlslcc_zcmpg_CSMShadowTexture ("hlslcc_zcmpg_CSMShadowTexture", 2D) = "white" {}
s_SpecularOcclusionLutBRDFCone ("s_SpecularOcclusionLutBRDFCone", 2D) = "white" {}
s_DFGTexture ("s_DFGTexture", 2D) = "white" {}
_ReflectionMap_MatCap ("_ReflectionMap_MatCap", 2D) = "white" {}
_RefractionMap_MatCap ("_RefractionMap_MatCap", 2D) = "white" {} }
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
float4 _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float4 g_FogParam;
float4 g_FogColor;
float _OcclusionScale;
float _ShadowStrength;
float4 _RMCaptured_Time;
float4 _RMCaptured_unity_WorldTransformParams;
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixV[4];
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
float4 _AlbedoColor;
float4 _EmissiveColor;
float _BlendMode;
float4 _ReflectEnvColor;
float4 _RefractEnvColor;
float2 _AOMinMaxRemap;
float2 _MetallicMinMaxRemap;
float2 _RoughnessMinMaxRemap;
float2 _OpacityMinMaxRemap;
float4 _RoughnessChSwizzle;
float4 _MetallicChSwizzle;
float4 _OcclusionChSwizzle;
float3 _SelfGI;
float _Scale_Thickness;
float _Scale_Refraction;
float3 _TransColor;
float _NormalFlatten;
float _MotionSpeed;
float4 _ThicknessMap_ST;
float4 _DetailNormalMap_ST;
int _PBRRef;
Texture2D _AlbedoMap; SamplerState sampler_AlbedoMap;
Texture2D _MaterialParamsMap; SamplerState sampler_MaterialParamsMap;
Texture2D _EmissiveMap; SamplerState sampler_EmissiveMap;
Texture2D _NormalMap; SamplerState sampler_NormalMap;
Texture2D _ThicknessMap; SamplerState sampler_ThicknessMap;
Texture2D _DetailNormalMap; SamplerState sampler_DetailNormalMap;
TextureCube g_EnvmapFilterWithGGX; SamplerState samplerg_EnvmapFilterWithGGX;
Texture2D g_CSMShadowTexture; SamplerState samplerg_CSMShadowTexture;
Texture2D hlslcc_zcmpg_CSMShadowTexture; SamplerState samplerhlslcc_zcmpg_CSMShadowTexture; // Captured D16 stored as RFloat; explicit <= comparison.
Texture2D s_SpecularOcclusionLutBRDFCone; SamplerState samplers_SpecularOcclusionLutBRDFCone;
Texture2D s_DFGTexture; SamplerState samplers_DFGTexture;
Texture2D _ReflectionMap_MatCap; SamplerState sampler_ReflectionMap_MatCap;
Texture2D _RefractionMap_MatCap; SamplerState sampler_RefractionMap_MatCap;
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float3 in_NORMAL0 : NORMAL0;
float4 in_TANGENT0 : TANGENT0;
float2 in_TEXCOORD0 : TEXCOORD0;
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
float3 u_xlat3;
float3 u_xlat16_4;
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
    u_xlat2.xyz = v.in_POSITION0.yyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1].xyz;
    u_xlat2.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0].xyz * v.in_POSITION0.xxx + u_xlat2.xyz;
    u_xlat2.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2].xyz * v.in_POSITION0.zzz + u_xlat2.xyz;
    u_xlat2.xyz = u_xlat2.xyz + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3].xyz;
    u_xlat3.xyz = (-u_xlat2.xyz) + _RMCaptured_WorldSpaceCameraPos.xyz;
    u_xlat18 = dot(u_xlat3.xyz, u_xlat3.xyz);
    u_xlat18 = sqrt(u_xlat18);
    u_xlat18 = u_xlat18 + (-g_FogParam.x);
    u_xlat18 = u_xlat18 / g_FogParam.y;
    u_xlat18 = clamp(u_xlat18, 0.0, 1.0);
    u_xlat19 = u_xlat2.y + (-g_FogParam.z);
    u_xlat19 = u_xlat19 / g_FogParam.w;
    u_xlat19 = clamp(u_xlat19, 0.0, 1.0);
    u_xlat16_4.x = u_xlat18 + u_xlat19;
    u_xlat16_4.x = min(u_xlat16_4.x, 1.0);
    u_xlat16_4.x = u_xlat16_4.x * g_FogColor.w;
    o.vs_POSITIONWS0.w = u_xlat16_4.x;
    o.vs_POSITIONWS0.xyz = u_xlat2.xyz;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy;
    o.vs_TEXCOORD0.zw = float2(0.0, 0.0);
    u_xlat16_4.xyz = u_xlat0.zxy * u_xlat1.yzx;
    u_xlat16_4.xyz = u_xlat0.yzx * u_xlat1.zxy + (-u_xlat16_4.xyz);
    u_xlat16_4.xyz = u_xlat16_4.xyz * v.in_TANGENT0.www;
    u_xlat1.xyz = u_xlat0.yzx * u_xlat16_4.zxy;
    u_xlat1.xyz = u_xlat16_4.yzx * u_xlat0.zxy + (-u_xlat1.xyz);
    u_xlat3.x = sin(v.in_TEXCOORD2.y);
    u_xlat5 = cos(v.in_TEXCOORD2.y);
    u_xlat1.xyz = u_xlat1.xyz * u_xlat3.xxx;
    u_xlat0.xyz = ((float3)(u_xlat5)) * u_xlat0.xyz + u_xlat1.xyz;
    u_xlat18 = (-v.in_TEXCOORD2.x) * v.in_TEXCOORD2.x + 1.0;
    u_xlat18 = max(u_xlat18, 0.0);
    u_xlat18 = sqrt(u_xlat18);
    u_xlat0.xyz = ((float3)(u_xlat18)) * u_xlat0.xyz;
    u_xlat0.xyz = v.in_TEXCOORD2.xxx * u_xlat16_4.xyz + u_xlat0.xyz;
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
float4 u_xlat16_2;
float3 u_xlat16_3;
float4 u_xlat16_4;
float4 u_xlat16_5;
float4 u_xlat6;
float4 u_xlat16_6;
float3 u_xlat7;
bool u_xlatb7;
float3 u_xlat8;
float3 u_xlat16_8;
bool3 u_xlatb8;
float3 u_xlat16_9;
float4 u_xlat10;
float4 u_xlat16_10;
float3 u_xlat16_11;
float3 u_xlat12;
float2 u_xlat16_12;
float3 u_xlat16_13;
float4 u_xlat14;
float3 u_xlat16_14;
float4 u_xlat15;
float3 u_xlat16_15;
bool u_xlatb15;
float3 u_xlat16;
bool2 u_xlatb16;
float2 u_xlat17;
float4 u_xlat18;
float4 u_xlat16_19;
float3 u_xlat16_20;
float3 u_xlat16_21;
float3 u_xlat16_24;
float u_xlat29;
bool u_xlatb29;
float u_xlat37;
float u_xlat16_37;
float u_xlat16_46;
bool u_xlatb51;
float u_xlat59;
float u_xlat16_59;
float2 u_xlat60;
float2 u_xlat61;
float u_xlat16_67;
float u_xlat16_68;
float u_xlat16_69;
float u_xlat16_71;
float u_xlat16_72;
float u_xlat73;
float u_xlat16_73;
bool u_xlatb73;
float u_xlat74;
float u_xlat16_74;
float u_xlat16_75;
float u_xlat16_76;
float u_xlat16_77;
float u_xlat78;
uint u_xlatu78;
bool u_xlatb78;
float u_xlat16_79;
float u_xlat81;
float u_xlat16_81;
bool u_xlatb81;


float4 hlslcc_FragCoord = float4(v.position.xyz, 1.0/v.position.w);
    u_xlat16_0 = _AlbedoMap.Sample(sampler_AlbedoMap, v.vs_TEXCOORD0.xy);
    u_xlat16_1.xyz = u_xlat16_0.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_1.xyz = u_xlat16_0.xyz * u_xlat16_1.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_1.xyz = u_xlat16_0.xyz * u_xlat16_1.xyz;
    u_xlat16_2.xyz = u_xlat16_0.www * _AlbedoColor.xyz;
    u_xlat16_2.xyz = u_xlat16_2.xyz * _AlbedoColor.www;
    u_xlat16_3.xyz = u_xlat16_1.xyz * u_xlat16_2.xyz;
    u_xlat16_4 = _MaterialParamsMap.Sample(sampler_MaterialParamsMap, v.vs_TEXCOORD0.xy);
    u_xlat16_67 = dot(u_xlat16_4, _RoughnessChSwizzle);
    u_xlat16_68 = dot(u_xlat16_4, _MetallicChSwizzle);
    u_xlat16_69 = dot(u_xlat16_4, _OcclusionChSwizzle);
    u_xlat16_67 = u_xlat16_67 * _RoughnessMinMaxRemap.y + _RoughnessMinMaxRemap.x;
    u_xlat16_68 = u_xlat16_68 * _MetallicMinMaxRemap.xxxy.w + _MetallicMinMaxRemap.xxxy.z;
    u_xlat16_69 = u_xlat16_69 * _AOMinMaxRemap.y + _AOMinMaxRemap.x;
    u_xlat16_0.xyz = _EmissiveMap.Sample(sampler_EmissiveMap, v.vs_TEXCOORD0.xy).xyz;
    u_xlat16_5.xyz = u_xlat16_0.xyz * _EmissiveColor.xyz;
    u_xlat16_6.xyz = u_xlat16_5.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_6.xyz = u_xlat16_5.xyz * u_xlat16_6.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_0.w = u_xlat16_0.w * _OpacityMinMaxRemap.xxxy.w + _OpacityMinMaxRemap.xxxy.z;
    u_xlat16_4.y = max(u_xlat16_67, 0.0890000015);
    u_xlat16_67 = u_xlat16_4.y * u_xlat16_4.y;
    u_xlat16_71 = (-u_xlat16_68) + 1.0;
    u_xlat16_3.xyz = u_xlat16_3.xyz * ((float3)(u_xlat16_71));
    u_xlat7.xyz = u_xlat16_1.xyz * u_xlat16_2.xyz + float3(-0.0399999991, -0.0399999991, -0.0399999991);
    u_xlat7.xyz = ((float3)(u_xlat16_68)) * u_xlat7.xyz + float3(0.0399999991, 0.0399999991, 0.0399999991);
    u_xlat16_8.xyz = _NormalMap.Sample(sampler_NormalMap, v.vs_TEXCOORD0.xy).xyz;
    u_xlat16_1.xyz = u_xlat16_8.xyz * float3(2.0, 2.0, 2.0) + float3(-1.0, -1.0, -1.0);
    u_xlat16_2.x = dot(u_xlat16_1.xyz, u_xlat16_1.xyz);
    u_xlat16_2.x = rsqrt(u_xlat16_2.x);
    u_xlat16_1.xyz = u_xlat16_1.xyz * u_xlat16_2.xxx;
    u_xlatb73 = _RMCaptured_unity_WorldTransformParams.w>=0.0;
    u_xlat73 = (u_xlatb73) ? 1.0 : -1.0;
    u_xlat16_2.x = u_xlat73 * v.vs_TANGENT0.w;
    u_xlat16_24.x = dot(v.vs_NORMAL0.xyz, v.vs_NORMAL0.xyz);
    u_xlat16_24.x = rsqrt(u_xlat16_24.x);
    u_xlat16_24.xyz = u_xlat16_24.xxx * v.vs_NORMAL0.xyz;
    u_xlat16_71 = dot(v.vs_TANGENT0.xyz, u_xlat16_24.xyz);
    u_xlat16_9.xyz = (-u_xlat16_24.xyz) * ((float3)(u_xlat16_71)) + v.vs_TANGENT0.xyz;
    u_xlat16_71 = dot(u_xlat16_9.xyz, u_xlat16_9.xyz);
    u_xlat16_71 = rsqrt(u_xlat16_71);
    u_xlat16_9.xyz = ((float3)(u_xlat16_71)) * u_xlat16_9.xyz;
    u_xlat16_10.xyz = u_xlat16_24.zxy * u_xlat16_9.yzx;
    u_xlat16_10.xyz = u_xlat16_24.yzx * u_xlat16_9.zxy + (-u_xlat16_10.xyz);
    u_xlat16_10.xyz = u_xlat16_2.xxx * u_xlat16_10.xyz;
    u_xlat16_11.xyz = u_xlat16_1.yyy * u_xlat16_10.xyz;
    u_xlat16_11.xyz = u_xlat16_1.xxx * u_xlat16_9.xyz + u_xlat16_11.xyz;
    u_xlat16_1.xyz = u_xlat16_1.zzz * u_xlat16_24.xyz + u_xlat16_11.xyz;
    u_xlat8.xyz = (-v.vs_POSITIONWS0.xyz) + _RMCaptured_WorldSpaceCameraPos.xyz;
    u_xlat73 = dot(u_xlat8.xyz, u_xlat8.xyz);
    u_xlat73 = sqrt(u_xlat73);
    u_xlat73 = float(1.0) / u_xlat73;
    u_xlat12.xyz = ((float3)(u_xlat73)) * u_xlat8.xyz;
    u_xlat16_4.x = dot(u_xlat16_1.xyz, u_xlat12.xyz);
    u_xlat16_4.x = clamp(u_xlat16_4.x, 0.0, 1.0);
    u_xlat16_2.x = dot(v.vs_TEXCOORD1.xyz, v.vs_TEXCOORD1.xyz);
    u_xlat16_2.x = rsqrt(u_xlat16_2.x);
    u_xlat16_11.xyz = u_xlat16_2.xxx * v.vs_TEXCOORD1.xyz;
    u_xlat16_2.x = v.vs_TEXCOORD1.w;
    u_xlat16_2.x = clamp(u_xlat16_2.x, 0.0, 1.0);
    u_xlat16_71 = v.vs_NORMAL0.w;
    u_xlat16_71 = clamp(u_xlat16_71, 0.0, 1.0);
    u_xlat16_72 = u_xlat16_2.x * u_xlat16_2.x;
    u_xlat16_13.xyz = u_xlat16_1.xyz * ((float3)(u_xlat16_72)) + u_xlat16_11.xyz;
    u_xlat16_72 = dot(u_xlat16_13.xyz, u_xlat16_13.xyz);
    u_xlat16_72 = rsqrt(u_xlat16_72);
    u_xlat16_14.xyz = ((float3)(u_xlat16_72)) * u_xlat16_13.xyz;
    u_xlat16_75 = dot(u_xlat16_11.xyz, u_xlat16_14.xyz);
    u_xlat16_75 = clamp(u_xlat16_75, 0.0, 1.0);
    u_xlat16_11.xyz = u_xlat16_13.xyz * ((float3)(u_xlat16_72)) + (-u_xlat16_1.xyz);
    u_xlat16_11.xyz = float3(float3(_OcclusionScale, _OcclusionScale, _OcclusionScale)) * u_xlat16_11.xyz + u_xlat16_1.xyz;
    u_xlat16_72 = dot(u_xlat16_11.xyz, u_xlat16_11.xyz);
    u_xlat16_72 = rsqrt(u_xlat16_72);
    u_xlat16_11.xyz = ((float3)(u_xlat16_72)) * u_xlat16_11.xyz;
    u_xlat16_2.x = u_xlat16_2.x * u_xlat16_75 + -1.0;
    u_xlat16_4.w = _OcclusionScale * u_xlat16_2.x + 1.0;
    u_xlat16_2.x = u_xlat16_71 + -1.0;
    u_xlat16_2.x = _OcclusionScale * u_xlat16_2.x + 1.0;
    u_xlat15.xy = v.vs_TEXCOORD0.xy * _ThicknessMap_ST.xy + _ThicknessMap_ST.zw;
    u_xlat16_74 = _ThicknessMap.Sample(sampler_ThicknessMap, u_xlat15.xy).x;
    u_xlat74 = u_xlat16_74 * _Scale_Thickness;
    u_xlat15.xy = v.vs_TEXCOORD0.xy * _DetailNormalMap_ST.xy + _DetailNormalMap_ST.zw;
    u_xlat16_15.xyz = _DetailNormalMap.Sample(sampler_DetailNormalMap, u_xlat15.xy).xyz;
    u_xlat16_13.xyz = u_xlat16_15.xyz * float3(2.0, 2.0, 2.0) + float3(-1.0, -1.0, -1.0);
    u_xlat16_71 = dot(u_xlat16_13.xyz, u_xlat16_13.xyz);
    u_xlat16_71 = rsqrt(u_xlat16_71);
    u_xlat16_14.xyz = ((float3)(u_xlat16_71)) * u_xlat16_13.xyz;
    u_xlat15.xyz = (-u_xlat16_13.xyz) * ((float3)(u_xlat16_71)) + float3(0.0, 0.0, 1.0);
    u_xlat15.xyz = float3(float3(_NormalFlatten, _NormalFlatten, _NormalFlatten)) * u_xlat15.xyz + u_xlat16_14.xyz;
    u_xlat78 = dot(u_xlat15.xyz, u_xlat15.xyz);
    u_xlat78 = rsqrt(u_xlat78);
    u_xlat15.xyz = ((float3)(u_xlat78)) * u_xlat15.xyz;
    u_xlat16_10.xyz = u_xlat16_10.xyz * u_xlat15.yyy;
    u_xlat16_9.xyz = u_xlat15.xxx * u_xlat16_9.xyz + u_xlat16_10.xyz;
    u_xlat16_9.xyz = u_xlat15.zzz * u_xlat16_24.xyz + u_xlat16_9.xyz;
    u_xlatb78 = 0.0<g_LightPositionAndFalloff[0].w;
    if(u_xlatb78){
        u_xlat15.xyz = (-v.vs_POSITIONWS0.xyz) + g_LightPositionAndFalloff[0].xyz;
        u_xlat16_71 = dot(u_xlat15.xyz, u_xlat15.xyz);
        u_xlat16_72 = rsqrt(u_xlat16_71);
        u_xlat16_10.xyz = ((float3)(u_xlat16_72)) * u_xlat15.xyz;
        u_xlat16_72 = dot(u_xlat16_10.xyz, g_LightDirectionAndAngleOffset[0].xyz);
        u_xlat16_75 = max(u_xlat16_71, 9.99999975e-05);
        u_xlat16_75 = float(1.0) / u_xlat16_75;
        u_xlat16_71 = u_xlat16_71 * g_LightPositionAndFalloff[0].w;
        u_xlat78 = (-u_xlat16_71) * u_xlat16_71 + 1.0;
        u_xlat78 = max(u_xlat78, 0.0);
        u_xlat16_71 = u_xlat16_72 * abs(g_LightIntensityAndAngleScale[0].w) + g_LightDirectionAndAngleOffset[0].w;
        u_xlat16_71 = clamp(u_xlat16_71, 0.0, 1.0);
        u_xlat16_71 = u_xlat16_71 * u_xlat78;
        u_xlat16_71 = u_xlat16_71 * u_xlat16_71;
        u_xlat16_71 = u_xlat16_75 * u_xlat16_71;
    } else {
        u_xlat16_10.xyz = g_LightDirectionAndAngleOffset[0].xyz;
        u_xlat16_71 = 1.0;
    }
    u_xlat16_13.xyz = ((float3)(u_xlat16_71)) * g_LightIntensityAndAngleScale[0].xyz;
    u_xlat16_14.xyz = u_xlat8.xyz * ((float3)(u_xlat73)) + u_xlat16_10.xyz;
    u_xlat16_71 = dot(u_xlat16_14.xyz, u_xlat16_14.xyz);
    u_xlat16_71 = rsqrt(u_xlat16_71);
    u_xlat16_14.xyz = ((float3)(u_xlat16_71)) * u_xlat16_14.xyz;
    u_xlat16_71 = dot(u_xlat16_1.xyz, u_xlat16_10.xyz);
    u_xlat16_71 = clamp(u_xlat16_71, 0.0, 1.0);
    u_xlat16_72 = dot(u_xlat16_1.xyz, u_xlat16_14.xyz);
    u_xlat16_72 = max(u_xlat16_72, 0.0);
    u_xlat16_75 = dot(u_xlat16_10.xyz, u_xlat16_14.xyz);
    u_xlat16_75 = clamp(u_xlat16_75, 0.0, 1.0);
    u_xlatb78 = g_CSMNormalBias.x<0.0;
    u_xlat15.x = v.vs_POSITIONWS0.y * _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[1].w;
    u_xlat15.x = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[0].w * v.vs_POSITIONWS0.x + u_xlat15.x;
    u_xlat15.x = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[2].w * v.vs_POSITIONWS0.z + u_xlat15.x;
    u_xlat15.x = u_xlat15.x + _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[3].w;
    u_xlat15.x = u_xlat15.x * g_CSMNormalBias.x;
    u_xlat16_76 = (u_xlatb78) ? abs(u_xlat15.x) : g_CSMNormalBias.x;
    u_xlat16_76 = u_xlat16_76 + u_xlat16_76;
    u_xlat16_77 = (-u_xlat16_71) + 2.0;
    u_xlat16_76 = u_xlat16_76 * u_xlat16_77;
    u_xlat15.xyz = ((float3)(u_xlat16_76)) * u_xlat16_24.xyz + v.vs_POSITIONWS0.xyz;
    u_xlat14 = u_xlat15.yyyy * _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[1];
    u_xlat14 = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[0] * u_xlat15.xxxx + u_xlat14;
    u_xlat14 = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[2] * u_xlat15.zzzz + u_xlat14;
    u_xlat14 = u_xlat14 + _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[3];
    u_xlat78 = u_xlat14.z * g_CSMDistanceFade.x + g_CSMDistanceFade.y;
    u_xlat78 = clamp(u_xlat78, 0.0, 1.0);
    u_xlatb15 = u_xlat78<1.0;
    if(u_xlatb15){
        u_xlat15.xyz = u_xlat14.xyz / u_xlat14.www;
        u_xlatb16.xy = ((abs(u_xlat15.xyxx)) < ( float4(0.9921875, 0.9921875, 0.0, 0.0))).xy;
        u_xlatb81 = u_xlatb16.y && u_xlatb16.x;
        if(u_xlatb81){
            u_xlat15.xy = u_xlat15.xy * g_CSMUvScaleOffset[0].xy + g_CSMUvScaleOffset[0].zw;
            u_xlat59 = min(u_xlat15.z, 0.999899983);
            u_xlat15.xy = u_xlat15.xy * g_CSMShadowTexture_TexelSize.zw + float2(0.5, 0.5);
            u_xlat16.xy = floor(u_xlat15.xy);
            u_xlat15.xy = u_xlat15.xy + (-u_xlat16.xy);
            u_xlat16.xy = u_xlat16.xy + float2(-0.5, -0.5);
            u_xlat16.xy = u_xlat16.xy * g_CSMShadowTexture_TexelSize.xy;
            u_xlat60.xy = (-u_xlat15.yx) * float2(2.0, 2.0) + float2(3.0, 3.0);
            u_xlat17.xy = u_xlat15.xy * float2(2.0, 2.0) + float2(1.0, 1.0);
            u_xlat61.xy = (-u_xlat15.xy) + float2(2.0, 2.0);
            u_xlat61.xy = u_xlat61.xy / u_xlat60.yx;
            u_xlat15.xy = u_xlat15.xy / u_xlat17.xy;
            u_xlat14.zw = u_xlat61.xy + float2(-1.0, -1.0);
            u_xlat14.xy = u_xlat15.xy + float2(1.0, 1.0);
            u_xlat15.x = u_xlat60.x * u_xlat60.y;
            u_xlat18 = u_xlat14.zwxw * g_CSMShadowTexture_TexelSize.xyxy + u_xlat16.xyxy;
            float3 txVec0 = float3(u_xlat18.xy,u_xlat59);
            u_xlat16_37 = step((txVec0).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec0).xy, 0).r);
            u_xlat60.xy = u_xlat60.xy * u_xlat17.xy;
            float3 txVec1 = float3(u_xlat18.zw,u_xlat59);
            u_xlat16_81 = step((txVec1).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec1).xy, 0).r);
            u_xlat81 = u_xlat16_81 * u_xlat60.x;
            u_xlat15.x = u_xlat15.x * u_xlat16_37 + u_xlat81;
            u_xlat14 = u_xlat14.zyxy * g_CSMShadowTexture_TexelSize.xyxy + u_xlat16.xyxy;
            float3 txVec2 = float3(u_xlat14.xy,u_xlat59);
            u_xlat16_37 = step((txVec2).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec2).xy, 0).r);
            u_xlat15.x = u_xlat60.y * u_xlat16_37 + u_xlat15.x;
            u_xlat37 = u_xlat17.y * u_xlat17.x;
            float3 txVec3 = float3(u_xlat14.zw,u_xlat59);
            u_xlat16_59 = step((txVec3).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec3).xy, 0).r);
            u_xlat15.x = u_xlat37 * u_xlat16_59 + u_xlat15.x;
            u_xlat37 = u_xlat15.x * 0.0625;
            u_xlat16_24.x = (-u_xlat15.x) * 0.0625 + 1.0;
            u_xlat16_24.x = u_xlat78 * u_xlat16_24.x + u_xlat37;
        } else {
            u_xlat16_24.x = 1.0;
        }
    } else {
        u_xlat16_24.x = 1.0;
    }
    u_xlat16_24.x = u_xlat16_24.x + -1.0;
    u_xlat16_24.x = _ShadowStrength * u_xlat16_24.x + 1.0;
    u_xlat16_46 = min(u_xlat16_72, 0.999511719);
    u_xlat16_68 = u_xlat16_67 * u_xlat16_67;
    u_xlat16_72 = u_xlat16_46 * u_xlat16_68 + (-u_xlat16_46);
    u_xlat16_46 = u_xlat16_72 * u_xlat16_46 + 1.0;
    u_xlat16_46 = u_xlat16_67 / u_xlat16_46;
    u_xlat16_46 = u_xlat16_46 * u_xlat16_46;
    u_xlat78 = u_xlat16_46 * 0.318309873;
    u_xlat16_46 = (-u_xlat16_4.x) * u_xlat16_68 + u_xlat16_4.x;
    u_xlat16_46 = u_xlat16_46 * u_xlat16_4.x + u_xlat16_68;
    u_xlat16_46 = sqrt(u_xlat16_46);
    u_xlat16_46 = u_xlat16_46 + u_xlat16_4.x;
    u_xlat16_72 = (-u_xlat16_71) * u_xlat16_68 + u_xlat16_71;
    u_xlat16_72 = u_xlat16_72 * u_xlat16_71 + u_xlat16_68;
    u_xlat16_72 = sqrt(u_xlat16_72);
    u_xlat16_72 = u_xlat16_71 + u_xlat16_72;
    u_xlat16_72 = u_xlat16_46 * u_xlat16_72;
    u_xlat16_72 = float(1.0) / u_xlat16_72;
    u_xlat16_75 = (-u_xlat16_75) + 1.0;
    u_xlat16_76 = u_xlat16_75 * u_xlat16_75;
    u_xlat16_76 = u_xlat16_76 * u_xlat16_76;
    u_xlat16_77 = u_xlat16_75 * u_xlat16_76;
    u_xlat16_79 = u_xlat7.y * 50.0;
    u_xlat16_79 = clamp(u_xlat16_79, 0.0, 1.0);
    u_xlat16_75 = (-u_xlat16_75) * u_xlat16_76 + 1.0;
    u_xlat16_19.xyz = u_xlat7.xyz * ((float3)(u_xlat16_75));
    u_xlat16_19.xyz = ((float3)(u_xlat16_79)) * ((float3)(u_xlat16_77)) + u_xlat16_19.xyz;
    u_xlat16_72 = u_xlat16_72 * u_xlat78;
    u_xlat15.xyz = u_xlat16_3.xyz * float3(0.318309873, 0.318309873, 0.318309873);
    u_xlat16_75 = dot((-u_xlat12.xyz), u_xlat16_1.xyz);
    u_xlat16_76 = (-u_xlat16_75) * u_xlat16_75 + 1.0;
    u_xlat16_76 = (-u_xlat16_76) * 0.577600002 + 1.0;
    u_xlat16_77 = sqrt(u_xlat16_76);
    u_xlat16_77 = u_xlat16_75 * 0.75999999 + u_xlat16_77;
    u_xlatb78 = u_xlat16_76>=0.0;
    u_xlat16_20.xyz = u_xlat16_1.xyz * ((float3)(u_xlat16_77));
    u_xlat16_20.xyz = u_xlat12.xyz * float3(-0.75999999, -0.75999999, -0.75999999) + (-u_xlat16_20.xyz);
    u_xlat16_20.xyz = (bool(u_xlatb78)) ? u_xlat16_20.xyz : float3(0.0, 0.0, 0.0);
    u_xlat16_20.xyz = (-u_xlat16_1.xyz) + u_xlat16_20.xyz;
    u_xlat16_76 = dot(u_xlat16_20.xyz, u_xlat16_20.xyz);
    u_xlat16_76 = rsqrt(u_xlat16_76);
    u_xlat16_20.xyz = ((float3)(u_xlat16_76)) * u_xlat16_20.xyz;
    u_xlat16_10.x = dot(u_xlat16_20.xyz, u_xlat16_10.xyz);
    u_xlat16_10.x = u_xlat16_10.x * 0.5 + 0.5;
    u_xlat16_10.x = clamp(u_xlat16_10.x, 0.0, 1.0);
    u_xlat16_10.x = u_xlat16_10.x * u_xlat16_10.x;
    u_xlat16_10.x = u_xlat16_10.x * u_xlat16_10.x;
    u_xlat16_10.x = u_xlat16_10.x * u_xlat16_10.x;
    u_xlat16_10.x = u_xlat16_10.x * u_xlat16_10.x;
    u_xlat16_10.x = u_xlat16_10.x * u_xlat16_10.x;
    u_xlat16_10.x = u_xlat16_10.x * u_xlat16_10.x;
    u_xlat16_10.x = u_xlat16_10.x * u_xlat16_10.x;
    u_xlat16_10.x = u_xlat16_10.x * u_xlat16_10.x;
    u_xlat16_10.xyz = u_xlat16_13.xyz * u_xlat16_10.xxx;
    u_xlat16_10.xyz = u_xlat16_10.xyz * _TransColor.xyz;
    u_xlat16_76 = max(u_xlat16_24.x, 0.617999971);
    u_xlat16_10.xyz = ((float3)(u_xlat16_76)) * u_xlat16_10.xyz;
    u_xlat16_19.xyz = u_xlat16_19.xyz * ((float3)(u_xlat16_72)) + u_xlat15.xyz;
    u_xlat16_24.x = u_xlat16_71 * u_xlat16_24.x;
    u_xlat16_13.xyz = u_xlat16_24.xxx * u_xlat16_13.xyz;
    u_xlat16_10.xyz = u_xlat16_19.xyz * u_xlat16_13.xyz + u_xlat16_10.xyz;
    u_xlat16_5.xyz = u_xlat16_5.xyz * u_xlat16_6.xyz + u_xlat16_10.xyz;
    u_xlat16_6.xyz = u_xlat16_5.xyz;
    for(uint u_xlatu_loop_1 = 1u ; u_xlatu_loop_1<3u ; u_xlatu_loop_1++)
    {
        u_xlatb81 = 0.0<g_LightPositionAndFalloff[int(u_xlatu_loop_1)].w;
        if(u_xlatb81){
            u_xlat16.xyz = (-v.vs_POSITIONWS0.xyz) + g_LightPositionAndFalloff[int(u_xlatu_loop_1)].xyz;
            u_xlat16_24.x = dot(u_xlat16.xyz, u_xlat16.xyz);
            u_xlat16_71 = rsqrt(u_xlat16_24.x);
            u_xlat16_10.xyz = ((float3)(u_xlat16_71)) * u_xlat16.xyz;
            u_xlat16_71 = dot(u_xlat16_10.xyz, g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].xyz);
            u_xlat16_72 = max(u_xlat16_24.x, 9.99999975e-05);
            u_xlat16_72 = float(1.0) / u_xlat16_72;
            u_xlat16_24.x = u_xlat16_24.x * g_LightPositionAndFalloff[int(u_xlatu_loop_1)].w;
            u_xlat81 = (-u_xlat16_24.x) * u_xlat16_24.x + 1.0;
            u_xlat81 = max(u_xlat81, 0.0);
            u_xlat16_24.x = u_xlat16_71 * g_LightIntensityAndAngleScale[int(u_xlatu_loop_1)].w + g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].w;
            u_xlat16_24.x = clamp(u_xlat16_24.x, 0.0, 1.0);
            u_xlat16_24.x = u_xlat16_24.x * u_xlat81;
            u_xlat16_24.x = u_xlat16_24.x * u_xlat16_24.x;
            u_xlat16_24.x = u_xlat16_72 * u_xlat16_24.x;
        } else {
            u_xlat16_10.xyz = g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].xyz;
            u_xlat16_24.x = 1.0;
        }
        u_xlat16_13.xyz = u_xlat16_24.xxx * g_LightIntensityAndAngleScale[int(u_xlatu_loop_1)].xyz;
        u_xlat16_19.xyz = u_xlat8.xyz * ((float3)(u_xlat73)) + u_xlat16_10.xyz;
        u_xlat16_24.x = dot(u_xlat16_19.xyz, u_xlat16_19.xyz);
        u_xlat16_24.x = rsqrt(u_xlat16_24.x);
        u_xlat16_19.xyz = u_xlat16_24.xxx * u_xlat16_19.xyz;
        u_xlat16_24.x = dot(u_xlat16_1.xyz, u_xlat16_10.xyz);
        u_xlat16_24.x = clamp(u_xlat16_24.x, 0.0, 1.0);
        u_xlat16_71 = dot(u_xlat16_1.xyz, u_xlat16_19.xyz);
        u_xlat16_71 = max(u_xlat16_71, 0.0);
        u_xlat16_72 = dot(u_xlat16_10.xyz, u_xlat16_19.xyz);
        u_xlat16_72 = clamp(u_xlat16_72, 0.0, 1.0);
        u_xlat16_71 = min(u_xlat16_71, 0.999511719);
        u_xlat16_76 = u_xlat16_71 * u_xlat16_68 + (-u_xlat16_71);
        u_xlat16_71 = u_xlat16_76 * u_xlat16_71 + 1.0;
        u_xlat16_71 = u_xlat16_67 / u_xlat16_71;
        u_xlat16_71 = u_xlat16_71 * u_xlat16_71;
        u_xlat81 = u_xlat16_71 * 0.318309873;
        u_xlat16_71 = (-u_xlat16_24.x) * u_xlat16_68 + u_xlat16_24.x;
        u_xlat16_71 = u_xlat16_71 * u_xlat16_24.x + u_xlat16_68;
        u_xlat16_71 = sqrt(u_xlat16_71);
        u_xlat16_71 = u_xlat16_24.x + u_xlat16_71;
        u_xlat16_71 = u_xlat16_46 * u_xlat16_71;
        u_xlat16_71 = float(1.0) / u_xlat16_71;
        u_xlat16_72 = (-u_xlat16_72) + 1.0;
        u_xlat16_76 = u_xlat16_72 * u_xlat16_72;
        u_xlat16_76 = u_xlat16_76 * u_xlat16_76;
        u_xlat16_77 = u_xlat16_72 * u_xlat16_76;
        u_xlat16_72 = (-u_xlat16_72) * u_xlat16_76 + 1.0;
        u_xlat16_19.xyz = u_xlat7.xyz * ((float3)(u_xlat16_72));
        u_xlat16_19.xyz = ((float3)(u_xlat16_79)) * ((float3)(u_xlat16_77)) + u_xlat16_19.xyz;
        u_xlat16_71 = u_xlat16_71 * u_xlat81;
        u_xlat16_72 = dot(u_xlat16_20.xyz, u_xlat16_10.xyz);
        u_xlat16_72 = u_xlat16_72 * 0.5 + 0.5;
        u_xlat16_72 = clamp(u_xlat16_72, 0.0, 1.0);
        u_xlat16_72 = u_xlat16_72 * u_xlat16_72;
        u_xlat16_72 = u_xlat16_72 * u_xlat16_72;
        u_xlat16_72 = u_xlat16_72 * u_xlat16_72;
        u_xlat16_72 = u_xlat16_72 * u_xlat16_72;
        u_xlat16_72 = u_xlat16_72 * u_xlat16_72;
        u_xlat16_72 = u_xlat16_72 * u_xlat16_72;
        u_xlat16_72 = u_xlat16_72 * u_xlat16_72;
        u_xlat16_72 = u_xlat16_72 * u_xlat16_72;
        u_xlat16_10.xyz = u_xlat16_13.xyz * ((float3)(u_xlat16_72));
        u_xlat16_10.xyz = u_xlat16_10.xyz * _TransColor.xyz;
        u_xlat16_19.xyz = u_xlat16_19.xyz * ((float3)(u_xlat16_71)) + u_xlat15.xyz;
        u_xlat16_13.xyz = u_xlat16_24.xxx * u_xlat16_13.xyz;
        u_xlat16_10.xyz = u_xlat16_19.xyz * u_xlat16_13.xyz + u_xlat16_10.xyz;
        u_xlat16_6.xyz = u_xlat16_6.xyz + u_xlat16_10.xyz;
    }
    u_xlat16_5.x = dot(g_EnvmapRotationParam.xy, u_xlat16_1.xz);
    u_xlat16_5.z = dot(g_EnvmapRotationParam.zw, u_xlat16_1.xz);
    u_xlat16_10.x = dot(g_EnvmapRotationParam.xy, u_xlat16_11.xz);
    u_xlat16_10.z = dot(g_EnvmapRotationParam.zw, u_xlat16_11.xz);
    u_xlat16_5.y = u_xlat16_1.y;
    u_xlat16_10.y = u_xlat16_11.y;
    u_xlat16_13.xyz = u_xlat16_5.xyz * u_xlat16_10.xyz;
    u_xlat16_13.xyz = clamp(u_xlat16_13.xyz, 0.0, 1.0);
    u_xlat16_19.xyz = u_xlat16_10.xyz * u_xlat16_10.xyz;
    u_xlat16_67 = u_xlat16_4.w * u_xlat16_4.w;
    u_xlat16_24.x = dot(u_xlat16_10.xyz, u_xlat16_5.xyz);
    u_xlat16_46 = (-u_xlat16_24.x) + 1.0;
    u_xlat16_46 = clamp(u_xlat16_46, 0.0, 1.0);
    u_xlat16_24.x = (-u_xlat16_24.x) * u_xlat16_67 + 1.0;
    u_xlat16_24.x = clamp(u_xlat16_24.x, 0.0, 1.0);
    u_xlat16_24.x = u_xlat16_24.x * 0.400000006;
    u_xlat16_46 = u_xlat16_4.w * u_xlat16_46;
    u_xlat16_5.xzw = u_xlat16_24.xxx * _SelfGI.xyz;
    u_xlat16_5.xzw = ((float3)(u_xlat16_46)) * float3(0.5, 0.5, 0.5) + u_xlat16_5.xzw;
    u_xlat16_13.xyz = u_xlat16_13.xyz * ((float3)(u_xlat16_67));
    u_xlat16_20.xyz = u_xlat16_5.xzw * u_xlat16_19.xxx + u_xlat16_13.xxx;
    u_xlat16_19.xyw = u_xlat16_5.xzw * u_xlat16_19.yyy + u_xlat16_13.yyy;
    u_xlat16_5.xzw = u_xlat16_5.xzw * u_xlat16_19.zzz + u_xlat16_13.zzz;
    u_xlatb8.xyz = ((u_xlat16_10.xyzx) >= ( float4(0.0, 0.0, 0.0, 0.0))).xyz;
    u_xlat16_10.x = (u_xlatb8.x) ? float(1.0) : float(0.0);
    u_xlat16_10.y = (u_xlatb8.y) ? float(1.0) : float(0.0);
    u_xlat16_10.z = (u_xlatb8.z) ? float(1.0) : float(0.0);
    u_xlat16_13.x = (u_xlatb8.x) ? float(0.0) : float(1.0);
    u_xlat16_13.y = (u_xlatb8.y) ? float(0.0) : float(1.0);
    u_xlat16_13.z = (u_xlatb8.z) ? float(0.0) : float(1.0);
    u_xlat16_21.xyz = u_xlat16_20.xyz * g_IrradianceACCoeffs[0].xyz;
    u_xlat16_20.xyz = u_xlat16_20.xyz * g_IrradianceACCoeffs[1].xyz;
    u_xlat16_20.xyz = u_xlat16_13.xxx * u_xlat16_20.xyz;
    u_xlat16_20.xyz = u_xlat16_21.xyz * u_xlat16_10.xxx + u_xlat16_20.xyz;
    u_xlat16_21.xyz = u_xlat16_19.xyw * g_IrradianceACCoeffs[2].xyz;
    u_xlat16_10.xyw = u_xlat16_21.xyz * u_xlat16_10.yyy + u_xlat16_20.xyz;
    u_xlat16_19.xyz = u_xlat16_19.xyw * g_IrradianceACCoeffs[3].xyz;
    u_xlat16_10.xyw = u_xlat16_19.xyz * u_xlat16_13.yyy + u_xlat16_10.xyw;
    u_xlat16_19.xyz = u_xlat16_5.xzw * g_IrradianceACCoeffs[4].xyz;
    u_xlat16_10.xyz = u_xlat16_19.xyz * u_xlat16_10.zzz + u_xlat16_10.xyw;
    u_xlat16_5.xzw = u_xlat16_5.xzw * g_IrradianceACCoeffs[5].xyz;
    u_xlat16_5.xzw = u_xlat16_5.xzw * u_xlat16_13.zzz + u_xlat16_10.xyz;
    u_xlat16_5.xzw = u_xlat16_2.xxx * u_xlat16_5.xzw;
    u_xlat16_5.xzw = u_xlat16_5.xzw * g_EnvmapIntensity.xyz;
    u_xlat16_5.xzw = ((float3)(u_xlat16_69)) * u_xlat16_5.xzw;
    u_xlat16_3.xyz = u_xlat16_5.xzw * u_xlat16_3.xyz + u_xlat16_6.xyz;
    u_xlat6 = u_xlat16_5.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixV[1].zxyz;
    u_xlat6 = _RMCaptured_hlslcc_mtx4x4unity_MatrixV[0].zxyz * u_xlat16_1.xxxx + u_xlat6;
    u_xlat6 = _RMCaptured_hlslcc_mtx4x4unity_MatrixV[2].zxyz * u_xlat16_1.zzzz + u_xlat6;
    u_xlat73 = dot(u_xlat6.yzw, u_xlat6.yzw);
    u_xlat73 = rsqrt(u_xlat73);
    u_xlat6 = u_xlat6 * ((float4)(u_xlat73));
    u_xlat10 = v.vs_POSITIONWS0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixV[1].yzzx;
    u_xlat10 = _RMCaptured_hlslcc_mtx4x4unity_MatrixV[0].yzzx * v.vs_POSITIONWS0.xxxx + u_xlat10;
    u_xlat10 = _RMCaptured_hlslcc_mtx4x4unity_MatrixV[2].yzzx * v.vs_POSITIONWS0.zzzz + u_xlat10;
    u_xlat10 = u_xlat10 + _RMCaptured_hlslcc_mtx4x4unity_MatrixV[3].yzzx;
    u_xlat73 = dot(u_xlat10.xzw, u_xlat10.xzw);
    u_xlat73 = rsqrt(u_xlat73);
    u_xlat10 = ((float4)(u_xlat73)) * u_xlat10;
    u_xlat8.xy = u_xlat6.zw * u_xlat10.zw;
    u_xlat8.xy = u_xlat10.yx * u_xlat6.yx + (-u_xlat8.yx);
    u_xlat8.xy = u_xlat8.xy * float2(-0.5, 0.5) + float2(0.5, 0.5);
    u_xlat16_24.x = u_xlat16_4.y * 9.0;
    u_xlat16_6 = _ReflectionMap_MatCap.SampleLevel(sampler_ReflectionMap_MatCap, u_xlat8.xy, u_xlat16_24.x);
    u_xlat16_13.xyz = u_xlat16_6.www * _ReflectEnvColor.xyz;
    u_xlat16_13.xyz = u_xlat16_6.xyz * u_xlat16_13.xyz;
    u_xlat16_19.xyz = u_xlat16_13.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_19.xyz = u_xlat16_13.xyz * u_xlat16_19.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_13.xyz = u_xlat16_13.xyz * u_xlat16_19.xyz;
    u_xlat16_19.xyz = _ReflectEnvColor.xyz * _ReflectEnvColor.xyz;
    u_xlat16_20.xyz = u_xlat16_13.xyz * u_xlat16_19.xyz;
    u_xlat73 = _RMCaptured_Time.x * _MotionSpeed;
    u_xlat73 = cos(u_xlat73);
    u_xlat73 = u_xlat73 * 0.5 + 0.5;
    u_xlat73 = u_xlat73 * 0.200000003 + _NormalFlatten;
    u_xlat73 = clamp(u_xlat73, 0.0, 1.0);
    u_xlat73 = (-u_xlat73) + 1.0;
    u_xlat15.xyz = u_xlat16_9.xyz * ((float3)(u_xlat73)) + u_xlat16_1.xyz;
    u_xlat73 = dot(u_xlat15.xyz, u_xlat15.xyz);
    u_xlat73 = rsqrt(u_xlat73);
    u_xlat15.xyz = ((float3)(u_xlat73)) * u_xlat15.xyz;
    u_xlat16.xyz = u_xlat15.yyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixV[1].xyz;
    u_xlat15.xyw = _RMCaptured_hlslcc_mtx4x4unity_MatrixV[0].xyz * u_xlat15.xxx + u_xlat16.xyz;
    u_xlat15.xyz = _RMCaptured_hlslcc_mtx4x4unity_MatrixV[2].xyz * u_xlat15.zzz + u_xlat15.xyw;
    u_xlat73 = dot(u_xlat15.xyz, u_xlat15.xyz);
    u_xlat73 = rsqrt(u_xlat73);
    u_xlat15.xy = ((float2)(u_xlat73)) * u_xlat15.xy;
    u_xlat15.xy = u_xlat15.xy * float2(float2(_Scale_Refraction, _Scale_Refraction));
    u_xlat8.xy = u_xlat15.xy * ((float2)(u_xlat74)) + u_xlat8.xy;
    u_xlat16_6 = _RefractionMap_MatCap.SampleLevel(sampler_RefractionMap_MatCap, u_xlat8.xy, u_xlat16_4.y);
    u_xlat16_9.xyz = u_xlat16_6.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_9.xyz = u_xlat16_6.xyz * u_xlat16_9.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_9.xyz = u_xlat16_6.xyz * u_xlat16_9.xyz;
    u_xlat16_9.xyz = u_xlat16_6.www * u_xlat16_9.xyz;
    u_xlat16_21.xyz = _RefractEnvColor.xyz * _RefractEnvColor.xyz;
    u_xlat16_9.xyz = u_xlat16_9.xyz * u_xlat16_21.xyz;
    u_xlat73 = float(_PBRRef);
    u_xlatb73 = 0.5<u_xlat73;
    if(u_xlatb73){
        u_xlat16_24.x = dot(u_xlat16_5.xzw, float3(0.333299994, 0.333299994, 0.333299994));
        u_xlat16_46 = dot(u_xlat16_11.xyz, u_xlat16_1.xyz);
        u_xlat16_46 = clamp(u_xlat16_46, 0.0, 1.0);
        u_xlat16_69 = u_xlat16_46 * 0.5 + 0.5;
        u_xlat16_69 = (-u_xlat16_46) + u_xlat16_69;
        u_xlat16_46 = u_xlat16_4.w * u_xlat16_69 + u_xlat16_46;
        u_xlat16_67 = u_xlat16_67 * u_xlat16_46;
        u_xlat16_67 = u_xlat16_2.x * u_xlat16_67;
        u_xlat16_46 = u_xlat16_75 + u_xlat16_75;
        u_xlat16_5.xyz = u_xlat16_1.xyz * (-((float3)(u_xlat16_46))) + (-u_xlat12.xyz);
        u_xlat16_4.z = dot(u_xlat16_11.xyz, u_xlat16_5.xyz);
        u_xlat16_11.xyz = u_xlat16_4.yzw * float3(1.09769487, 0.5, 1.0) + float3(-0.097694844, 0.5, -0.0);
        u_xlat16_11.xyz = clamp(u_xlat16_11.xyz, 0.0, 1.0);
        u_xlat16_46 = u_xlat16_11.z * 15.0;
        u_xlat16_46 = floor(u_xlat16_46);
        u_xlat16_69 = u_xlat16_46 + 1.0;
        u_xlat16_69 = min(u_xlat16_69, 15.0);
        u_xlat16_71 = u_xlat16_11.z * 15.0 + (-u_xlat16_46);
        u_xlat16_6.x = u_xlat16_46 * 16.0;
        u_xlat16_6.y = float(0.0);
        u_xlat16_6.w = float(0.0);
        u_xlat16_6.z = u_xlat16_69 * 16.0;
        u_xlat16_6 = u_xlat16_11.xyxy * float4(15.0, 15.0, 15.0, 15.0) + u_xlat16_6;
        u_xlat16_6 = u_xlat16_6 * float4(0.00390625, 0.0625, 0.00390625, 0.0625) + float4(0.001953125, 0.03125, 0.001953125, 0.03125);
        u_xlat16_73 = s_SpecularOcclusionLutBRDFCone.Sample(samplers_SpecularOcclusionLutBRDFCone, u_xlat16_6.xy).x;
        u_xlat16_8.x = s_SpecularOcclusionLutBRDFCone.Sample(samplers_SpecularOcclusionLutBRDFCone, u_xlat16_6.zw).x;
        u_xlat16_46 = (-u_xlat16_73) + u_xlat16_8.x;
        u_xlat16_46 = u_xlat16_71 * u_xlat16_46 + u_xlat16_73;
        u_xlat16_2.x = u_xlat16_2.x * u_xlat16_46;
        u_xlat16_46 = u_xlat16_67 * 0.5;
        u_xlat16_69 = (-u_xlat16_67) * 0.5 + 1.0;
        u_xlat16_2.x = u_xlat16_2.x * u_xlat16_69 + u_xlat16_46;
        u_xlat16_11.xyz = u_xlat16_2.xxx * _SelfGI.xyz;
        u_xlat16_21.xyz = u_xlat16_11.xyz + u_xlat16_11.xyz;
        u_xlat16_11.xyz = (-u_xlat16_11.xyz) * float3(2.0, 2.0, 2.0) + float3(1.0, 1.0, 1.0);
        u_xlat16_11.xyz = u_xlat16_2.xxx * u_xlat16_11.xyz + u_xlat16_21.xyz;
        u_xlat16_11.xyz = ((float3)(u_xlat16_67)) * u_xlat16_11.xyz;
        u_xlat16_67 = u_xlat16_4.y * g_EnvmapMipLevelsUsed + (-u_xlat16_4.y);
        u_xlat16_1.xyz = u_xlat16_1.xyz + (-u_xlat16_5.xyz);
        u_xlat16_1.xyz = ((float3)(u_xlat16_68)) * u_xlat16_1.xyz + u_xlat16_5.xyz;
        u_xlat16_5.x = dot(g_EnvmapRotationParam.xy, u_xlat16_1.xz);
        u_xlat16_5.z = dot(g_EnvmapRotationParam.zw, u_xlat16_1.xz);
        u_xlat16_5.y = u_xlat16_1.y;
        u_xlat16_1 = g_EnvmapFilterWithGGX.SampleLevel(samplerg_EnvmapFilterWithGGX, u_xlat16_5.xyz, u_xlat16_67);
        u_xlat16_2.x = u_xlat16_1.w * 6.0;
        u_xlat16_2.xzw = u_xlat16_1.xyz * u_xlat16_2.xxx;
        u_xlat16_2.xzw = u_xlat16_2.xzw * u_xlat16_2.xzw;
        u_xlat16_2.xzw = u_xlat16_2.xzw * g_EnvmapIntensity.xyz;
        u_xlat16_2.xzw = u_xlat16_11.xyz * u_xlat16_2.xzw;
        u_xlatb73 = 0.0<g_EnvmapIntensity.w;
        u_xlat73 = (u_xlatb73) ? u_xlat16_24.x : 1.0;
        u_xlat8.xyz = u_xlat16_2.xzw * ((float3)(u_xlat73));
        u_xlat16_12.xy = s_DFGTexture.Sample(samplers_DFGTexture, u_xlat16_4.xy).xy;
        u_xlat16_2.x = u_xlat16_12.y * u_xlat16_79;
        u_xlat16_2.xyz = u_xlat7.xyz * u_xlat16_12.xxx + u_xlat16_2.xxx;
        u_xlat16_5.xyz = u_xlat16_19.xyz * u_xlat16_13.xyz + u_xlat16_9.xyz;
        u_xlat16_2.xyz = u_xlat8.xyz * u_xlat16_2.xyz + (-u_xlat16_5.xyz);
        u_xlat16_2.xyz = u_xlat16_0.www * u_xlat16_2.xyz + u_xlat16_5.xyz;
    } else {
        u_xlat16_2.xyz = u_xlat16_9.xyz * u_xlat16_0.www + u_xlat16_20.xyz;
    }
    u_xlat16_2.xyz = u_xlat16_2.xyz + u_xlat16_3.xyz;
    u_xlat16_3.xyz = (-u_xlat16_2.xyz) + g_FogColor.xyz;
    u_xlat16_2.xyz = v.vs_POSITIONWS0.www * u_xlat16_3.xyz + u_xlat16_2.xyz;
    u_xlat16_3.xyz = u_xlat16_2.yyy * _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[1].xyz;
    u_xlat16_2.xyw = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[0].xyz * u_xlat16_2.xxx + u_xlat16_3.xyz;
    u_xlat16_2.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[2].xyz * u_xlat16_2.zzz + u_xlat16_2.xyw;
    u_xlat16_2.xyz = u_xlat16_2.xyz + _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].xyz;
    u_xlatb7 = 0.0>=_RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].w;
    u_xlat16_3.xyz = u_xlat16_2.xyz + float3(0.155000001, 0.155000001, 0.155000001);
    u_xlat16_3.xyz = u_xlat16_2.xyz / u_xlat16_3.xyz;
    u_xlat16_1.xyz = u_xlat16_3.xyz * float3(1.01900005, 1.01900005, 1.01900005);
    u_xlatb29 = 0.0>=_BlendMode;
    u_xlatb51 = g_AlphaOutput<1.0;
    u_xlatb29 = u_xlatb51 && u_xlatb29;
    u_xlat16_68 = dot(u_xlat16_1.xyz, float3(0.333299994, 0.333299994, 0.333299994));
    u_xlat16_1.w = (u_xlatb29) ? u_xlat16_68 : u_xlat16_0.w;
    u_xlat16_0.xyz = min(u_xlat16_2.xyz, float3(65504.0, 65504.0, 65504.0));
    u_xlat16_0 = (bool(u_xlatb7)) ? u_xlat16_1 : u_xlat16_0;
    u_xlatb7 = 2.0<g_ExposureScale.w;
    u_xlat8.xy = hlslcc_FragCoord.xy;
    u_xlat8.z = g_FrameIndexModX.x;
    u_xlat29 = dot(u_xlat8.xyz, float3(0.117647059, 0.411764711, 1.35294116));
    u_xlat29 = frac(u_xlat29);
    u_xlat16_2.xyz = ((float3)(u_xlat29)) * float3(0.00392156886, 0.00392156886, 0.00392156886) + u_xlat16_0.xyz;
    o.SV_Target0.xyz = (bool(u_xlatb7)) ? u_xlat16_2.xyz : u_xlat16_0.xyz;
    o.SV_Target0.w = u_xlat16_0.w;
    return o;

}
ENDHLSL
} } }
