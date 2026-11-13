// Associated EIDs: 1299
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E1299_Back_Blended" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_RMCaptured_WorldSpaceCameraPos ("_RMCaptured_WorldSpaceCameraPos", Vector) = (0,0,0,0)
g_FogParam ("g_FogParam", Vector) = (0,0,0,0)
g_FogColor ("g_FogColor", Vector) = (0,0,0,0)
_ShadowStrength ("_ShadowStrength", Float) = 0
_RMCaptured_unity_WorldTransformParams ("_RMCaptured_unity_WorldTransformParams", Vector) = (0,0,0,0)
_AlbedoColor ("_AlbedoColor", Vector) = (0,0,0,0)
_EmissiveColor ("_EmissiveColor", Vector) = (0,0,0,0)
_MetallicMultiplier ("_MetallicMultiplier", Float) = 0
_RoughnessMultiplier ("_RoughnessMultiplier", Float) = 0
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
_SelfGI ("_SelfGI", Vector) = (0,0,0,0)
_SSSColor1 ("_SSSColor1", Vector) = (0,0,0,0)
_SSSColor2 ("_SSSColor2", Vector) = (0,0,0,0)
_SSSColor3 ("_SSSColor3", Vector) = (0,0,0,0)
_SSSIntensity ("_SSSIntensity", Float) = 0
_ShadowCol ("_ShadowCol", Vector) = (0,0,0,0)
_SpOpacity ("_SpOpacity", Float) = 0
_AlbedoMap ("_AlbedoMap", 2D) = "white" {}
_SkinSSSMap ("_SkinSSSMap", 2D) = "white" {}
_MaterialParamsMap ("_MaterialParamsMap", 2D) = "white" {}
_EmissiveMap ("_EmissiveMap", 2D) = "white" {}
_NormalMap ("_NormalMap", 2D) = "white" {}
g_EnvmapFilterWithGGX ("g_EnvmapFilterWithGGX", Cube) = "white" {}
g_CSMShadowTexture ("g_CSMShadowTexture", 2D) = "white" {}
hlslcc_zcmpg_CSMShadowTexture ("hlslcc_zcmpg_CSMShadowTexture", 2D) = "white" {}
s_DFGTexture ("s_DFGTexture", 2D) = "white" {} }
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
float3 _RMCaptured_WorldSpaceCameraPos;
float4 _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float4 g_FogParam;
float4 g_FogColor;
float _ShadowStrength;
float4 _RMCaptured_unity_WorldTransformParams;
float4 _AlbedoColor;
float4 _EmissiveColor;
float _MetallicMultiplier;
float _RoughnessMultiplier;
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
float4 _SelfGI;
float4 _SSSColor1;
float4 _SSSColor2;
float4 _SSSColor3;
float _SSSIntensity;
float3 _ShadowCol;
float _SpOpacity;
Texture2D _AlbedoMap; SamplerState sampler_AlbedoMap;
Texture2D _SkinSSSMap; SamplerState sampler_SkinSSSMap;
Texture2D _MaterialParamsMap; SamplerState sampler_MaterialParamsMap;
Texture2D _EmissiveMap; SamplerState sampler_EmissiveMap;
Texture2D _NormalMap; SamplerState sampler_NormalMap;
TextureCube g_EnvmapFilterWithGGX; SamplerState samplerg_EnvmapFilterWithGGX;
Texture2D g_CSMShadowTexture; SamplerState samplerg_CSMShadowTexture;
Texture2D hlslcc_zcmpg_CSMShadowTexture; SamplerState samplerhlslcc_zcmpg_CSMShadowTexture; // Captured D16 stored as RFloat; explicit <= comparison.
Texture2D s_DFGTexture; SamplerState samplers_DFGTexture;
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
float u_xlat16_2;
float u_xlat9;


    u_xlat0.x = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[0].xyz);
    u_xlat0.y = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[1].xyz);
    u_xlat0.z = dot(v.in_NORMAL0.xyz, _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[2].xyz);
    u_xlat9 = dot(u_xlat0.xyz, u_xlat0.xyz);
    u_xlat9 = rsqrt(u_xlat9);
    u_xlat0.xyz = ((float3)(u_xlat9)) * u_xlat0.xyz;
    o.vs_NORMAL0.xyz = u_xlat0.xyz;
    o.vs_NORMAL0.w = 1.0;
    u_xlat0.xyz = v.in_TANGENT0.yyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1].xyz;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0].xyz * v.in_TANGENT0.xxx + u_xlat0.xyz;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2].xyz * v.in_TANGENT0.zzz + u_xlat0.xyz;
    u_xlat9 = dot(u_xlat0.xyz, u_xlat0.xyz);
    u_xlat9 = rsqrt(u_xlat9);
    u_xlat0.xyz = ((float3)(u_xlat9)) * u_xlat0.xyz;
    u_xlat0.w = v.in_TANGENT0.w;
    o.vs_TANGENT0 = u_xlat0;
    u_xlat0.xyz = v.in_POSITION0.yyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1].xyz;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0].xyz * v.in_POSITION0.xxx + u_xlat0.xyz;
    u_xlat0.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2].xyz * v.in_POSITION0.zzz + u_xlat0.xyz;
    u_xlat0.xyz = u_xlat0.xyz + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3].xyz;
    u_xlat1.xyz = (-u_xlat0.xyz) + _RMCaptured_WorldSpaceCameraPos.xyz;
    u_xlat9 = dot(u_xlat1.xyz, u_xlat1.xyz);
    u_xlat9 = sqrt(u_xlat9);
    u_xlat9 = u_xlat9 + (-g_FogParam.x);
    u_xlat9 = u_xlat9 / g_FogParam.y;
    u_xlat9 = clamp(u_xlat9, 0.0, 1.0);
    u_xlat1.x = u_xlat0.y + (-g_FogParam.z);
    u_xlat1.x = u_xlat1.x / g_FogParam.w;
    u_xlat1.x = clamp(u_xlat1.x, 0.0, 1.0);
    u_xlat16_2 = u_xlat9 + u_xlat1.x;
    u_xlat16_2 = min(u_xlat16_2, 1.0);
    u_xlat16_2 = u_xlat16_2 * g_FogColor.w;
    o.vs_POSITIONWS0.w = u_xlat16_2;
    o.vs_POSITIONWS0.xyz = u_xlat0.xyz;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy;
    o.vs_TEXCOORD0.zw = v.in_TEXCOORD1.xy;
    u_xlat1 = u_xlat0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1];
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0] * u_xlat0.xxxx + u_xlat1;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2] * u_xlat0.zzzz + u_xlat1;
    o.position = u_xlat0 + _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3];
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;














































float3 u_xlat0;
float4 u_xlat16_0;
bool3 u_xlatb0;
float4 u_xlat16_1;
float4 u_xlat2;
float4 u_xlat16_2;
float4 u_xlat16_3;
float3 u_xlat16_4;
float4 u_xlat16_5;
float u_xlat6;
float4 u_xlat16_6;
bool u_xlatb6;
float3 u_xlat16_7;
float3 u_xlat16_8;
float4 u_xlat9;
float3 u_xlat16_9;
float2 u_xlat16_10;
float3 u_xlat16_11;
float3 u_xlat16_12;
float3 u_xlat16_13;
float3 u_xlat16_14;
float3 u_xlat15;
bool u_xlatb15;
float3 u_xlat16_16;
float3 u_xlat17;
bool2 u_xlatb17;
float3 u_xlat18;
float4 u_xlat19;
float3 u_xlat16_20;
float3 u_xlat16_21;
float3 u_xlat16_22;
float3 u_xlat16_23;
float3 u_xlat16_24;
float3 u_xlat16_25;
float3 u_xlat16_26;
bool u_xlatb27;
float3 u_xlat16_28;
float3 u_xlat33;
float3 u_xlat42;
bool u_xlatb42;
bool u_xlatb54;
float u_xlat16_55;
float u_xlat16_64;
float u_xlat69;
float u_xlat16_69;
uint u_xlatu69;
float2 u_xlat71;
float2 u_xlat72;
float u_xlat16_72;
float u_xlat16_82;
float u_xlat16_85;
float u_xlat16_88;
float u_xlat16_89;
float u_xlat16_90;
float u_xlat16_91;
float u_xlat16_92;
float u_xlat16_93;
float u_xlat16_94;
float u_xlat16_95;
float u_xlat96;
float u_xlat16_96;
bool u_xlatb96;
float u_xlat16_97;


float4 hlslcc_FragCoord = float4(v.position.xyz, 1.0/v.position.w);
    u_xlat16_0 = _AlbedoMap.Sample(sampler_AlbedoMap, v.vs_TEXCOORD0.xy);
    u_xlat16_1.xyz = u_xlat16_0.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_1.xyz = u_xlat16_0.xyz * u_xlat16_1.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_1.xyz = u_xlat16_0.xyz * u_xlat16_1.xyz;
    u_xlat16_0.x = _SkinSSSMap.Sample(sampler_SkinSSSMap, v.vs_TEXCOORD0.xy).x;
    u_xlat16_2 = _MaterialParamsMap.Sample(sampler_MaterialParamsMap, v.vs_TEXCOORD0.xy);
    u_xlat16_3 = _AlbedoColor + float4(-1.0, -1.0, -1.0, -1.0);
    u_xlat16_3 = u_xlat16_2.wwww * u_xlat16_3 + float4(1.0, 1.0, 1.0, 1.0);
    u_xlat16_4.xyz = u_xlat16_1.xyz * u_xlat16_3.xyz;
    u_xlat16_5.w = u_xlat16_0.w * u_xlat16_3.w;
    u_xlat16_6.xyz = _EmissiveMap.Sample(sampler_EmissiveMap, v.vs_TEXCOORD0.xy).xyz;
    u_xlat16_7.xyz = u_xlat16_6.xyz * _EmissiveColor.xyz;
    u_xlat16_8.xyz = u_xlat16_7.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_8.xyz = u_xlat16_7.xyz * u_xlat16_8.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_9.xy = u_xlat16_2.xy * float2(_RoughnessMultiplier, _MetallicMultiplier);
    u_xlat16_10.y = max(u_xlat16_9.x, 0.0890000015);
    u_xlat16_82 = u_xlat16_10.y * u_xlat16_10.y;
    u_xlat16_85 = (-u_xlat16_2.y) * _MetallicMultiplier + 1.0;
    u_xlat16_4.xyz = ((float3)(u_xlat16_85)) * u_xlat16_4.xyz;
    u_xlat2.xyw = u_xlat16_1.xyz * u_xlat16_3.xyz + float3(-0.0399999991, -0.0399999991, -0.0399999991);
    u_xlat2.xyw = u_xlat16_9.yyy * u_xlat2.xyw + float3(0.0399999991, 0.0399999991, 0.0399999991);
    u_xlat16_1.x = u_xlat16_0.x * u_xlat16_85;
    u_xlat16_0.xyz = _NormalMap.Sample(sampler_NormalMap, v.vs_TEXCOORD0.xy).xyz;
    u_xlat16_3.xyz = u_xlat16_0.xyz * float3(2.0, 2.0, 2.0) + float3(-1.0, -1.0, -1.0);
    u_xlat16_28.x = dot(u_xlat16_3.xyz, u_xlat16_3.xyz);
    u_xlat16_28.x = rsqrt(u_xlat16_28.x);
    u_xlat16_3.xyz = u_xlat16_28.xxx * u_xlat16_3.xyz;
    u_xlat16_28.x = _SSSIntensity * _SSSIntensity;
    u_xlat16_1.x = u_xlat16_28.x * u_xlat16_1.x;
    u_xlat16_1.x = clamp(u_xlat16_1.x, 0.0, 1.0);
    u_xlat16_28.x = sqrt(u_xlat16_1.x);
    u_xlatb0.x = _RMCaptured_unity_WorldTransformParams.w>=0.0;
    u_xlat0.x = (u_xlatb0.x) ? 1.0 : -1.0;
    u_xlat16_55 = u_xlat0.x * v.vs_TANGENT0.w;
    u_xlat16_85 = dot(v.vs_NORMAL0.xyz, v.vs_NORMAL0.xyz);
    u_xlat16_85 = rsqrt(u_xlat16_85);
    u_xlat16_9.xyz = ((float3)(u_xlat16_85)) * v.vs_NORMAL0.xyz;
    u_xlat16_88 = dot(v.vs_TANGENT0.xyz, u_xlat16_9.xyz);
    u_xlat16_11.xyz = (-u_xlat16_9.xyz) * ((float3)(u_xlat16_88)) + v.vs_TANGENT0.xyz;
    u_xlat16_88 = dot(u_xlat16_11.xyz, u_xlat16_11.xyz);
    u_xlat16_88 = rsqrt(u_xlat16_88);
    u_xlat16_11.xyz = ((float3)(u_xlat16_88)) * u_xlat16_11.xyz;
    u_xlat16_12.xyz = u_xlat16_9.zxy * u_xlat16_11.yzx;
    u_xlat16_12.xyz = u_xlat16_9.yzx * u_xlat16_11.zxy + (-u_xlat16_12.xyz);
    u_xlat16_12.xyz = ((float3)(u_xlat16_55)) * u_xlat16_12.xyz;
    u_xlat16_12.xyz = u_xlat16_3.yyy * u_xlat16_12.xyz;
    u_xlat16_11.xyz = u_xlat16_3.xxx * u_xlat16_11.xyz + u_xlat16_12.xyz;
    u_xlat16_3.xyz = u_xlat16_3.zzz * u_xlat16_9.xyz + u_xlat16_11.xyz;
    u_xlat0.xyz = (-v.vs_POSITIONWS0.xyz) + _RMCaptured_WorldSpaceCameraPos.xyz;
    u_xlat6 = dot(u_xlat0.xyz, u_xlat0.xyz);
    u_xlat6 = sqrt(u_xlat6);
    u_xlat6 = float(1.0) / u_xlat6;
    u_xlat33.xyz = u_xlat0.xyz * ((float3)(u_xlat6));
    u_xlat16_10.x = dot(u_xlat16_3.xyz, u_xlat33.xyz);
    u_xlat16_10.x = clamp(u_xlat16_10.x, 0.0, 1.0);
    u_xlat16_55 = dot(u_xlat16_3.xyz, u_xlat16_3.xyz);
    u_xlat16_55 = min(u_xlat16_55, 1.0);
    u_xlat16_55 = u_xlat16_55 * 0.5 + 0.5;
    u_xlat16_11.xyz = v.vs_NORMAL0.xyz * ((float3)(u_xlat16_85)) + u_xlat16_3.xyz;
    u_xlat16_85 = dot(u_xlat16_11.xyz, u_xlat16_11.xyz);
    u_xlat16_85 = rsqrt(u_xlat16_85);
    u_xlat16_11.xyz = ((float3)(u_xlat16_85)) * u_xlat16_11.xyz;
    u_xlat16_11.xyz = u_xlat16_28.xxx * u_xlat16_11.xyz;
    u_xlat16_11.xyz = u_xlat16_11.xyz * float3(16.0, 16.0, 16.0) + u_xlat16_3.xyz;
    u_xlat16_85 = dot(u_xlat16_11.xyz, u_xlat16_11.xyz);
    u_xlat16_85 = rsqrt(u_xlat16_85);
    u_xlat16_11.xyz = ((float3)(u_xlat16_85)) * u_xlat16_11.xyz;
    u_xlat16_85 = (-u_xlat16_28.x) + 1.0;
    u_xlat16_12.xyz = _SSSColor1.xyz * u_xlat16_28.xxx + ((float3)(u_xlat16_85));
    u_xlat16_13.xyz = _SSSColor2.xyz * u_xlat16_28.xxx + ((float3)(u_xlat16_85));
    u_xlat16_14.xyz = _SSSColor3.xyz * u_xlat16_28.xxx + ((float3)(u_xlat16_85));
    u_xlatb15 = 0.0<g_LightPositionAndFalloff[0].w;
    if(u_xlatb15){
        u_xlat15.xyz = (-v.vs_POSITIONWS0.xyz) + g_LightPositionAndFalloff[0].xyz;
        u_xlat16_85 = dot(u_xlat15.xyz, u_xlat15.xyz);
        u_xlat16_88 = rsqrt(u_xlat16_85);
        u_xlat16_16.xyz = ((float3)(u_xlat16_88)) * u_xlat15.xyz;
        u_xlat16_88 = dot(u_xlat16_16.xyz, g_LightDirectionAndAngleOffset[0].xyz);
        u_xlat16_89 = max(u_xlat16_85, 9.99999975e-05);
        u_xlat16_89 = float(1.0) / u_xlat16_89;
        u_xlat16_85 = u_xlat16_85 * g_LightPositionAndFalloff[0].w;
        u_xlat15.x = (-u_xlat16_85) * u_xlat16_85 + 1.0;
        u_xlat15.x = max(u_xlat15.x, 0.0);
        u_xlat16_85 = u_xlat16_88 * abs(g_LightIntensityAndAngleScale[0].w) + g_LightDirectionAndAngleOffset[0].w;
        u_xlat16_85 = clamp(u_xlat16_85, 0.0, 1.0);
        u_xlat16_85 = u_xlat16_85 * u_xlat15.x;
        u_xlat16_85 = u_xlat16_85 * u_xlat16_85;
        u_xlat16_85 = u_xlat16_89 * u_xlat16_85;
    } else {
        u_xlat16_16.xyz = g_LightDirectionAndAngleOffset[0].xyz;
        u_xlat16_85 = 1.0;
    }
    u_xlat16_88 = dot(u_xlat16_3.xyz, u_xlat16_16.xyz);
    u_xlat16_89 = u_xlat16_88;
    u_xlat16_89 = clamp(u_xlat16_89, 0.0, 1.0);
    u_xlatb15 = g_CSMNormalBias.x<0.0;
    u_xlat42.x = v.vs_POSITIONWS0.y * _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[1].w;
    u_xlat42.x = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[0].w * v.vs_POSITIONWS0.x + u_xlat42.x;
    u_xlat42.x = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[2].w * v.vs_POSITIONWS0.z + u_xlat42.x;
    u_xlat42.x = u_xlat42.x + _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[3].w;
    u_xlat42.x = u_xlat42.x * g_CSMNormalBias.x;
    u_xlat16_90 = (u_xlatb15) ? abs(u_xlat42.x) : g_CSMNormalBias.x;
    u_xlat16_90 = u_xlat16_90 + u_xlat16_90;
    u_xlat16_64 = (-u_xlat16_89) + 2.0;
    u_xlat16_90 = u_xlat16_90 * u_xlat16_64;
    u_xlat15.xyz = ((float3)(u_xlat16_90)) * u_xlat16_9.xyz + v.vs_POSITIONWS0.xyz;
    u_xlat9 = u_xlat15.yyyy * _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[1];
    u_xlat9 = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[0] * u_xlat15.xxxx + u_xlat9;
    u_xlat9 = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[2] * u_xlat15.zzzz + u_xlat9;
    u_xlat9 = u_xlat9 + _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[3];
    u_xlat15.x = u_xlat9.z * g_CSMDistanceFade.x + g_CSMDistanceFade.y;
    u_xlat15.x = clamp(u_xlat15.x, 0.0, 1.0);
    u_xlatb42 = u_xlat15.x<1.0;
    if(u_xlatb42){
        u_xlat42.xyz = u_xlat9.xyz / u_xlat9.www;
        u_xlatb17.xy = ((abs(u_xlat42.xyxx)) < ( float4(0.9921875, 0.9921875, 0.0, 0.0))).xy;
        u_xlatb17.x = u_xlatb17.y && u_xlatb17.x;
        if(u_xlatb17.x){
            u_xlat42.xy = u_xlat42.xy * g_CSMUvScaleOffset[0].xy + g_CSMUvScaleOffset[0].zw;
            u_xlat96 = min(u_xlat42.z, 0.999899983);
            u_xlat42.xy = u_xlat42.xy * g_CSMShadowTexture_TexelSize.zw + float2(0.5, 0.5);
            u_xlat17.xy = floor(u_xlat42.xy);
            u_xlat42.xy = u_xlat42.xy + (-u_xlat17.xy);
            u_xlat17.xy = u_xlat17.xy + float2(-0.5, -0.5);
            u_xlat17.xy = u_xlat17.xy * g_CSMShadowTexture_TexelSize.xy;
            u_xlat71.xy = (-u_xlat42.yx) * float2(2.0, 2.0) + float2(3.0, 3.0);
            u_xlat18.xy = u_xlat42.xy * float2(2.0, 2.0) + float2(1.0, 1.0);
            u_xlat72.xy = (-u_xlat42.xy) + float2(2.0, 2.0);
            u_xlat72.xy = u_xlat72.xy / u_xlat71.yx;
            u_xlat42.xy = u_xlat42.xy / u_xlat18.xy;
            u_xlat9.zw = u_xlat72.xy + float2(-1.0, -1.0);
            u_xlat9.xy = u_xlat42.xy + float2(1.0, 1.0);
            u_xlat42.x = u_xlat71.x * u_xlat71.y;
            u_xlat19 = u_xlat9.zwxw * g_CSMShadowTexture_TexelSize.xyxy + u_xlat17.xyxy;
            float3 txVec0 = float3(u_xlat19.xy,u_xlat96);
            u_xlat16_69 = step((txVec0).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec0).xy, 0).r);
            u_xlat71.xy = u_xlat71.xy * u_xlat18.xy;
            float3 txVec1 = float3(u_xlat19.zw,u_xlat96);
            u_xlat16_72 = step((txVec1).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec1).xy, 0).r);
            u_xlat71.x = u_xlat71.x * u_xlat16_72;
            u_xlat42.x = u_xlat42.x * u_xlat16_69 + u_xlat71.x;
            u_xlat9 = u_xlat9.zyxy * g_CSMShadowTexture_TexelSize.xyxy + u_xlat17.xyxy;
            float3 txVec2 = float3(u_xlat9.xy,u_xlat96);
            u_xlat16_69 = step((txVec2).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec2).xy, 0).r);
            u_xlat42.x = u_xlat71.y * u_xlat16_69 + u_xlat42.x;
            u_xlat69 = u_xlat18.y * u_xlat18.x;
            float3 txVec3 = float3(u_xlat9.zw,u_xlat96);
            u_xlat16_96 = step((txVec3).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec3).xy, 0).r);
            u_xlat42.x = u_xlat69 * u_xlat16_96 + u_xlat42.x;
            u_xlat69 = u_xlat42.x * 0.0625;
            u_xlat16_64 = (-u_xlat42.x) * 0.0625 + 1.0;
            u_xlat16_64 = u_xlat15.x * u_xlat16_64 + u_xlat69;
        } else {
            u_xlat16_64 = 1.0;
        }
    } else {
        u_xlat16_64 = 1.0;
    }
    u_xlatb15 = 0.0<_SpOpacity;
    u_xlatb42 = u_xlat16_5.w==0.0;
    u_xlatb42 = u_xlatb42 && u_xlatb15;
    if(!u_xlatb42){
        u_xlat16_20.xyz = ((float3)(u_xlat16_85)) * g_LightIntensityAndAngleScale[0].xyz;
        u_xlat16_21.xyz = u_xlat0.xyz * ((float3)(u_xlat6)) + u_xlat16_16.xyz;
        u_xlat16_85 = dot(u_xlat16_21.xyz, u_xlat16_21.xyz);
        u_xlat16_85 = rsqrt(u_xlat16_85);
        u_xlat16_21.xyz = ((float3)(u_xlat16_85)) * u_xlat16_21.xyz;
        u_xlat16_85 = dot(u_xlat16_3.xyz, u_xlat16_21.xyz);
        u_xlat16_85 = max(u_xlat16_85, 0.0);
        u_xlat16_91 = dot(u_xlat16_16.xyz, u_xlat16_21.xyz);
        u_xlat16_91 = clamp(u_xlat16_91, 0.0, 1.0);
        u_xlat16_64 = u_xlat16_64 + -1.0;
        u_xlat16_64 = _ShadowStrength * u_xlat16_64 + 1.0;
        u_xlat16_92 = dot(u_xlat16_11.xyz, u_xlat16_16.xyz);
        u_xlat16_88 = u_xlat16_88 + (-u_xlat16_92);
        u_xlat16_93 = u_xlat16_28.x + 1.0;
        u_xlat16_92 = u_xlat16_92 / u_xlat16_93;
        u_xlat16_92 = u_xlat16_28.x * 0.5 + u_xlat16_92;
        u_xlat16_92 = clamp(u_xlat16_92, 0.0, 1.0);
        u_xlat16_16.xyz = (-float3(_ShadowCol.x, _ShadowCol.y, _ShadowCol.z)) + float3(1.0, 1.0, 1.0);
        u_xlat16_16.xyz = ((float3)(u_xlat16_64)) * u_xlat16_16.xyz + float3(_ShadowCol.x, _ShadowCol.y, _ShadowCol.z);
        u_xlat16_21.xyz = ((float3)(u_xlat16_92)) * u_xlat16_16.xyz;
        u_xlat16_22.xyz = u_xlat16_13.xyz + (-u_xlat16_14.xyz);
        u_xlat16_22.xyz = u_xlat16_21.xyz * u_xlat16_22.xyz + u_xlat16_14.xyz;
        u_xlat16_23.xyz = u_xlat16_21.xyz * u_xlat16_22.xyz;
        u_xlat16_22.xyz = (-u_xlat16_21.xyz) * u_xlat16_22.xyz + u_xlat16_12.xyz;
        u_xlat16_21.xyz = u_xlat16_21.xyz * u_xlat16_22.xyz + u_xlat16_23.xyz;
        u_xlat16_88 = u_xlat16_88 * 2.0 + 1.0;
        u_xlat16_64 = sqrt(u_xlat16_55);
        u_xlat16_88 = u_xlat16_88 * u_xlat16_64;
        u_xlat16_88 = clamp(u_xlat16_88, 0.0, 1.0);
        u_xlat16_22.xyz = (-u_xlat16_13.xyz) + float3(1.0, 1.0, 1.0);
        u_xlat16_22.xyz = ((float3)(u_xlat16_88)) * u_xlat16_22.xyz + u_xlat16_13.xyz;
        u_xlat16_21.xyz = u_xlat16_21.xyz * u_xlat16_22.xyz;
        u_xlat16_85 = min(u_xlat16_85, 0.999511719);
        u_xlat16_88 = u_xlat16_82 * u_xlat16_82;
        u_xlat16_64 = u_xlat16_85 * u_xlat16_88 + (-u_xlat16_85);
        u_xlat16_85 = u_xlat16_64 * u_xlat16_85 + 1.0;
        u_xlat16_85 = u_xlat16_82 / u_xlat16_85;
        u_xlat16_85 = u_xlat16_85 * u_xlat16_85;
        u_xlat69 = u_xlat16_85 * 0.318309873;
        u_xlat16_85 = (-u_xlat16_10.x) * u_xlat16_88 + u_xlat16_10.x;
        u_xlat16_85 = u_xlat16_85 * u_xlat16_10.x + u_xlat16_88;
        u_xlat16_85 = sqrt(u_xlat16_85);
        u_xlat16_85 = u_xlat16_85 + u_xlat16_10.x;
        u_xlat16_64 = (-u_xlat16_89) * u_xlat16_88 + u_xlat16_89;
        u_xlat16_88 = u_xlat16_64 * u_xlat16_89 + u_xlat16_88;
        u_xlat16_88 = sqrt(u_xlat16_88);
        u_xlat16_88 = u_xlat16_88 + u_xlat16_89;
        u_xlat16_85 = u_xlat16_85 * u_xlat16_88;
        u_xlat16_85 = float(1.0) / u_xlat16_85;
        u_xlat16_88 = (-u_xlat16_91) + 1.0;
        u_xlat16_64 = u_xlat16_88 * u_xlat16_88;
        u_xlat16_64 = u_xlat16_64 * u_xlat16_64;
        u_xlat16_91 = u_xlat16_88 * u_xlat16_64;
        u_xlat16_92 = u_xlat2.y * 50.0;
        u_xlat16_92 = clamp(u_xlat16_92, 0.0, 1.0);
        u_xlat16_88 = (-u_xlat16_88) * u_xlat16_64 + 1.0;
        u_xlat16_22.xyz = u_xlat2.xyw * ((float3)(u_xlat16_88));
        u_xlat16_22.xyz = ((float3)(u_xlat16_92)) * ((float3)(u_xlat16_91)) + u_xlat16_22.xyz;
        u_xlat16_85 = u_xlat16_85 * u_xlat69;
        u_xlat16_22.xyz = ((float3)(u_xlat16_85)) * u_xlat16_22.xyz;
        u_xlat17.xyz = u_xlat16_4.xyz * float3(0.318309873, 0.318309873, 0.318309873);
        u_xlat16_21.xyz = u_xlat17.xyz * u_xlat16_21.xyz;
        u_xlat16_22.xyz = u_xlat16_20.xyz * u_xlat16_22.xyz;
        u_xlat16_16.xyz = ((float3)(u_xlat16_89)) * u_xlat16_16.xyz;
        u_xlat16_16.xyz = u_xlat16_16.xyz * u_xlat16_22.xyz;
        u_xlat16_20.xyz = u_xlat16_21.xyz * u_xlat16_20.xyz + u_xlat16_16.xyz;
    } else {
        u_xlat16_16.x = float(0.0);
        u_xlat16_16.y = float(0.0);
        u_xlat16_16.z = float(0.0);
        u_xlat16_20.x = float(0.0);
        u_xlat16_20.y = float(0.0);
        u_xlat16_20.z = float(0.0);
    }
    u_xlat16_7.xyz = u_xlat16_7.xyz * u_xlat16_8.xyz + u_xlat16_20.xyz;
    u_xlat16_85 = u_xlat16_28.x + 1.0;
    u_xlat16_8.xyz = u_xlat16_13.xyz + (-u_xlat16_14.xyz);
    u_xlat16_20.xyz = (-u_xlat16_13.xyz) + float3(1.0, 1.0, 1.0);
    u_xlat16_88 = u_xlat16_82 * u_xlat16_82;
    u_xlat16_89 = (-u_xlat16_10.x) * u_xlat16_88 + u_xlat16_10.x;
    u_xlat16_89 = u_xlat16_89 * u_xlat16_10.x + u_xlat16_88;
    u_xlat16_89 = sqrt(u_xlat16_89);
    u_xlat16_89 = u_xlat16_89 + u_xlat16_10.x;
    u_xlat16_64 = u_xlat2.y * 50.0;
    u_xlat16_64 = clamp(u_xlat16_64, 0.0, 1.0);
    u_xlat17.xyz = u_xlat16_4.xyz * float3(0.318309873, 0.318309873, 0.318309873);
    u_xlat16_21.xyz = u_xlat16_16.xyz;
    u_xlat16_22.xyz = u_xlat16_7.xyz;
    u_xlat16_91 = u_xlat16_55;
    for(uint u_xlatu_loop_1 = 1u ; u_xlatu_loop_1<3u ; u_xlatu_loop_1++)
    {
        u_xlatb96 = 0.0<g_LightPositionAndFalloff[int(u_xlatu_loop_1)].w;
        if(u_xlatb96){
            u_xlat18.xyz = (-v.vs_POSITIONWS0.xyz) + g_LightPositionAndFalloff[int(u_xlatu_loop_1)].xyz;
            u_xlat16_92 = dot(u_xlat18.xyz, u_xlat18.xyz);
            u_xlat16_93 = rsqrt(u_xlat16_92);
            u_xlat16_23.xyz = ((float3)(u_xlat16_93)) * u_xlat18.xyz;
            u_xlat16_93 = dot(u_xlat16_23.xyz, g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].xyz);
            u_xlat16_94 = max(u_xlat16_92, 9.99999975e-05);
            u_xlat16_94 = float(1.0) / u_xlat16_94;
            u_xlat16_92 = u_xlat16_92 * g_LightPositionAndFalloff[int(u_xlatu_loop_1)].w;
            u_xlat96 = (-u_xlat16_92) * u_xlat16_92 + 1.0;
            u_xlat96 = max(u_xlat96, 0.0);
            u_xlat16_92 = u_xlat16_93 * g_LightIntensityAndAngleScale[int(u_xlatu_loop_1)].w + g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].w;
            u_xlat16_92 = clamp(u_xlat16_92, 0.0, 1.0);
            u_xlat16_92 = u_xlat16_92 * u_xlat96;
            u_xlat16_92 = u_xlat16_92 * u_xlat16_92;
            u_xlat16_92 = u_xlat16_94 * u_xlat16_92;
        } else {
            u_xlat16_23.xyz = g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].xyz;
            u_xlat16_92 = 1.0;
        }
        if(!u_xlatb42){
            u_xlat16_24.xyz = ((float3)(u_xlat16_92)) * g_LightIntensityAndAngleScale[int(u_xlatu_loop_1)].xyz;
            u_xlat16_25.xyz = u_xlat0.xyz * ((float3)(u_xlat6)) + u_xlat16_23.xyz;
            u_xlat16_92 = dot(u_xlat16_25.xyz, u_xlat16_25.xyz);
            u_xlat16_92 = rsqrt(u_xlat16_92);
            u_xlat16_25.xyz = ((float3)(u_xlat16_92)) * u_xlat16_25.xyz;
            u_xlat16_92 = dot(u_xlat16_3.xyz, u_xlat16_23.xyz);
            u_xlat16_93 = u_xlat16_92;
            u_xlat16_93 = clamp(u_xlat16_93, 0.0, 1.0);
            u_xlat16_94 = dot(u_xlat16_3.xyz, u_xlat16_25.xyz);
            u_xlat16_94 = max(u_xlat16_94, 0.0);
            u_xlat16_95 = dot(u_xlat16_23.xyz, u_xlat16_25.xyz);
            u_xlat16_95 = clamp(u_xlat16_95, 0.0, 1.0);
            u_xlat16_97 = dot(u_xlat16_11.xyz, u_xlat16_23.xyz);
            u_xlat16_92 = u_xlat16_92 + (-u_xlat16_97);
            u_xlat16_97 = u_xlat16_97 / u_xlat16_85;
            u_xlat16_97 = u_xlat16_28.x * 0.5 + u_xlat16_97;
            u_xlat16_97 = clamp(u_xlat16_97, 0.0, 1.0);
            u_xlat16_23.xyz = ((float3)(u_xlat16_97)) * u_xlat16_8.xyz + u_xlat16_14.xyz;
            u_xlat16_25.xyz = ((float3)(u_xlat16_97)) * u_xlat16_23.xyz;
            u_xlat16_23.xyz = (-((float3)(u_xlat16_97))) * u_xlat16_23.xyz + u_xlat16_12.xyz;
            u_xlat16_23.xyz = ((float3)(u_xlat16_97)) * u_xlat16_23.xyz + u_xlat16_25.xyz;
            u_xlat16_92 = u_xlat16_92 * 2.0 + 1.0;
            u_xlat16_97 = sqrt(u_xlat16_91);
            u_xlat16_92 = u_xlat16_92 * u_xlat16_97;
            u_xlat16_92 = clamp(u_xlat16_92, 0.0, 1.0);
            u_xlat16_25.xyz = ((float3)(u_xlat16_92)) * u_xlat16_20.xyz + u_xlat16_13.xyz;
            u_xlat16_23.xyz = u_xlat16_23.xyz * u_xlat16_25.xyz;
            u_xlat16_92 = min(u_xlat16_94, 0.999511719);
            u_xlat16_94 = u_xlat16_92 * u_xlat16_88 + (-u_xlat16_92);
            u_xlat16_92 = u_xlat16_94 * u_xlat16_92 + 1.0;
            u_xlat16_92 = u_xlat16_82 / u_xlat16_92;
            u_xlat16_92 = u_xlat16_92 * u_xlat16_92;
            u_xlat96 = u_xlat16_92 * 0.318309873;
            u_xlat16_92 = (-u_xlat16_93) * u_xlat16_88 + u_xlat16_93;
            u_xlat16_92 = u_xlat16_92 * u_xlat16_93 + u_xlat16_88;
            u_xlat16_92 = sqrt(u_xlat16_92);
            u_xlat16_92 = u_xlat16_92 + u_xlat16_93;
            u_xlat16_92 = u_xlat16_89 * u_xlat16_92;
            u_xlat16_92 = float(1.0) / u_xlat16_92;
            u_xlat16_94 = (-u_xlat16_95) + 1.0;
            u_xlat16_95 = u_xlat16_94 * u_xlat16_94;
            u_xlat16_95 = u_xlat16_95 * u_xlat16_95;
            u_xlat16_97 = u_xlat16_94 * u_xlat16_95;
            u_xlat16_94 = (-u_xlat16_94) * u_xlat16_95 + 1.0;
            u_xlat16_25.xyz = u_xlat2.xyw * ((float3)(u_xlat16_94));
            u_xlat16_25.xyz = ((float3)(u_xlat16_64)) * ((float3)(u_xlat16_97)) + u_xlat16_25.xyz;
            u_xlat16_92 = u_xlat16_92 * u_xlat96;
            u_xlat16_25.xyz = ((float3)(u_xlat16_92)) * u_xlat16_25.xyz;
            u_xlat16_23.xyz = u_xlat17.xyz * u_xlat16_23.xyz;
            u_xlat16_25.xyz = u_xlat16_24.xyz * u_xlat16_25.xyz;
            u_xlat16_26.xyz = ((float3)(u_xlat16_93)) * u_xlat16_25.xyz;
            u_xlat16_21.xyz = u_xlat16_25.xyz * ((float3)(u_xlat16_93)) + u_xlat16_21.xyz;
            u_xlat16_23.xyz = u_xlat16_23.xyz * u_xlat16_24.xyz + u_xlat16_26.xyz;
        } else {
            u_xlat16_23.x = float(0.0);
            u_xlat16_23.y = float(0.0);
            u_xlat16_23.z = float(0.0);
        }
        u_xlat16_22.xyz = u_xlat16_22.xyz + u_xlat16_23.xyz;
    }
    if(!u_xlatb42){
        u_xlat16_7.x = dot(g_EnvmapRotationParam.xy, u_xlat16_3.xz);
        u_xlat16_7.z = dot(g_EnvmapRotationParam.zw, u_xlat16_3.xz);
        u_xlat16_7.y = u_xlat16_3.y;
        u_xlat16_28.xyz = u_xlat16_7.xyz * u_xlat16_7.xyz;
        u_xlat16_8.xyz = min(u_xlat16_28.xyz, float3(1.0, 1.0, 1.0));
        u_xlat16_85 = dot(u_xlat16_7.xyz, u_xlat16_7.xyz);
        u_xlat16_89 = (-u_xlat16_85) + 1.0;
        u_xlat16_89 = max(u_xlat16_89, 0.0);
        u_xlat16_91 = u_xlat16_89 * 0.400000006;
        u_xlat16_85 = u_xlat16_85 * 0.5 + 0.5;
        u_xlat16_11.x = (-u_xlat16_85) + 1.0;
        u_xlat16_11.xyz = u_xlat16_11.xxx * u_xlat16_13.xyz;
        u_xlat16_11.xyz = u_xlat16_12.xyz * ((float3)(u_xlat16_85)) + u_xlat16_11.xyz;
        u_xlat16_85 = (-u_xlat16_1.x) + 1.0;
        u_xlat16_89 = u_xlat16_85 * u_xlat16_89;
        u_xlat16_11.xyz = u_xlat16_1.xxx * u_xlat16_11.xyz;
        u_xlat16_11.xyz = ((float3)(u_xlat16_89)) * float3(0.5, 0.5, 0.5) + u_xlat16_11.xyz;
        u_xlat16_11.xyz = ((float3)(u_xlat16_91)) * _SelfGI.xyz + u_xlat16_11.xyz;
        u_xlatb0.xyz = ((u_xlat16_7.xyzx) >= ( float4(0.0, 0.0, 0.0, 0.0))).xyz;
        u_xlat16_7.x = (u_xlatb0.x) ? float(1.0) : float(0.0);
        u_xlat16_7.y = (u_xlatb0.y) ? float(1.0) : float(0.0);
        u_xlat16_7.z = (u_xlatb0.z) ? float(1.0) : float(0.0);
        u_xlat16_12.x = (u_xlatb0.x) ? float(0.0) : float(1.0);
        u_xlat16_12.y = (u_xlatb0.y) ? float(0.0) : float(1.0);
        u_xlat16_12.z = (u_xlatb0.z) ? float(0.0) : float(1.0);
        u_xlat16_8.xyz = u_xlat16_8.xyz * ((float3)(u_xlat16_85));
        u_xlat16_13.xyz = u_xlat16_11.xyz * u_xlat16_28.xxx + u_xlat16_8.xxx;
        u_xlat16_1.xyz = u_xlat16_11.xyz * u_xlat16_28.yyy + u_xlat16_8.yyy;
        u_xlat16_8.xyz = u_xlat16_11.xyz * u_xlat16_28.zzz + u_xlat16_8.zzz;
        u_xlat16_11.xyz = u_xlat16_13.xyz * g_IrradianceACCoeffs[0].xyz;
        u_xlat16_13.xyz = u_xlat16_13.xyz * g_IrradianceACCoeffs[1].xyz;
        u_xlat16_13.xyz = u_xlat16_12.xxx * u_xlat16_13.xyz;
        u_xlat16_11.xyz = u_xlat16_11.xyz * u_xlat16_7.xxx + u_xlat16_13.xyz;
        u_xlat16_13.xyz = u_xlat16_1.xyz * g_IrradianceACCoeffs[2].xyz;
        u_xlat16_11.xyz = u_xlat16_13.xyz * u_xlat16_7.yyy + u_xlat16_11.xyz;
        u_xlat16_1.xyz = u_xlat16_1.xyz * g_IrradianceACCoeffs[3].xyz;
        u_xlat16_1.xyz = u_xlat16_1.xyz * u_xlat16_12.yyy + u_xlat16_11.xyz;
        u_xlat16_11.xyz = u_xlat16_8.xyz * g_IrradianceACCoeffs[4].xyz;
        u_xlat16_1.xyz = u_xlat16_11.xyz * u_xlat16_7.zzz + u_xlat16_1.xyz;
        u_xlat16_7.xyz = u_xlat16_8.xyz * g_IrradianceACCoeffs[5].xyz;
        u_xlat16_1.xyz = u_xlat16_7.xyz * u_xlat16_12.zzz + u_xlat16_1.xyz;
        u_xlat16_1.xyz = u_xlat16_1.xyz * g_EnvmapIntensity.xyz;
        u_xlat16_1.xyz = u_xlat16_2.zzz * u_xlat16_1.xyz;
    } else {
        u_xlat16_1.x = float(0.0);
        u_xlat16_1.y = float(0.0);
        u_xlat16_1.z = float(0.0);
    }
    u_xlat16_4.xyz = u_xlat16_1.xyz * u_xlat16_4.xyz + u_xlat16_22.xyz;
    if(!u_xlatb42){
        u_xlat16_1.x = dot(u_xlat16_1.xyz, float3(0.333299994, 0.333299994, 0.333299994));
        u_xlat16_28.x = u_xlat16_10.y * g_EnvmapMipLevelsUsed + (-u_xlat16_10.y);
        u_xlat16_55 = dot((-u_xlat33.xyz), u_xlat16_3.xyz);
        u_xlat16_55 = u_xlat16_55 + u_xlat16_55;
        u_xlat16_7.xyz = u_xlat16_3.xyz * (-((float3)(u_xlat16_55))) + (-u_xlat33.xyz);
        u_xlat16_3.xyz = u_xlat16_3.xyz + (-u_xlat16_7.xyz);
        u_xlat16_3.xyz = ((float3)(u_xlat16_88)) * u_xlat16_3.xyz + u_xlat16_7.xyz;
        u_xlat16_7.x = dot(g_EnvmapRotationParam.xy, u_xlat16_3.xz);
        u_xlat16_7.z = dot(g_EnvmapRotationParam.zw, u_xlat16_3.xz);
        u_xlat16_7.y = u_xlat16_3.y;
        u_xlat16_6 = g_EnvmapFilterWithGGX.SampleLevel(samplerg_EnvmapFilterWithGGX, u_xlat16_7.xyz, u_xlat16_28.x);
        u_xlat16_28.x = u_xlat16_6.w * 6.0;
        u_xlat16_28.xyz = u_xlat16_28.xxx * u_xlat16_6.xyz;
        u_xlat16_28.xyz = u_xlat16_28.xyz * u_xlat16_28.xyz;
        u_xlat16_28.xyz = u_xlat16_28.xyz * g_EnvmapIntensity.xyz;
        u_xlat16_28.xyz = u_xlat16_2.zzz * u_xlat16_28.xyz;
        u_xlatb0.x = 0.0<g_EnvmapIntensity.w;
        u_xlat0.x = (u_xlatb0.x) ? u_xlat16_1.x : 1.0;
        u_xlat0.xyz = u_xlat0.xxx * u_xlat16_28.xyz;
        u_xlat16_6.xy = s_DFGTexture.Sample(samplers_DFGTexture, u_xlat16_10.xy).xy;
        u_xlat16_1.x = u_xlat16_6.y * u_xlat16_64;
        u_xlat16_1.xyz = u_xlat2.xyw * u_xlat16_6.xxx + u_xlat16_1.xxx;
        u_xlat16_3.xyz = u_xlat0.xyz * u_xlat16_1.xyz;
        u_xlat16_1.xyz = u_xlat0.xyz * u_xlat16_1.xyz + u_xlat16_21.xyz;
        u_xlat16_1.x = dot(u_xlat16_1.xyz, float3(0.219999999, 0.707000017, 0.0710000023));
        u_xlat16_1.x = clamp(u_xlat16_1.x, 0.0, 1.0);
        u_xlat16_1.x = u_xlat16_0.w * u_xlat16_3.w + u_xlat16_1.x;
        u_xlat16_5.w = (u_xlatb15) ? u_xlat16_1.x : u_xlat16_5.w;
    } else {
        u_xlat16_3.x = float(0.0);
        u_xlat16_3.y = float(0.0);
        u_xlat16_3.z = float(0.0);
    }
    u_xlat16_1.xyz = u_xlat16_3.xyz + u_xlat16_4.xyz;
    u_xlat16_3.xyz = (-u_xlat16_1.xyz) + g_FogColor.xyz;
    u_xlat16_1.xyz = v.vs_POSITIONWS0.www * u_xlat16_3.xyz + u_xlat16_1.xyz;
    u_xlat16_3.xyz = u_xlat16_1.yyy * _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[1].xyz;
    u_xlat16_1.xyw = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[0].xyz * u_xlat16_1.xxx + u_xlat16_3.xyz;
    u_xlat16_1.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[2].xyz * u_xlat16_1.zzz + u_xlat16_1.xyw;
    u_xlat16_1.xyz = u_xlat16_1.xyz + _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].xyz;
    u_xlatb0.x = 0.0>=_RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].w;
    u_xlat16_3.xyz = u_xlat16_1.xyz + float3(0.155000001, 0.155000001, 0.155000001);
    u_xlat16_3.xyz = u_xlat16_1.xyz / u_xlat16_3.xyz;
    u_xlat16_2.xyz = u_xlat16_3.xyz * float3(1.01900005, 1.01900005, 1.01900005);
    u_xlatb27 = 0.0>=_BlendMode;
    u_xlatb54 = g_AlphaOutput<1.0;
    u_xlatb27 = u_xlatb54 && u_xlatb27;
    u_xlat16_82 = dot(u_xlat16_2.xyz, float3(0.333299994, 0.333299994, 0.333299994));
    u_xlat16_2.w = (u_xlatb27) ? u_xlat16_82 : u_xlat16_5.w;
    u_xlat16_5.xyz = min(u_xlat16_1.xyz, float3(65504.0, 65504.0, 65504.0));
    u_xlat16_0 = (u_xlatb0.x) ? u_xlat16_2 : u_xlat16_5;
    u_xlatb6 = 2.0<g_ExposureScale.w;
    u_xlat15.xy = hlslcc_FragCoord.xy;
    u_xlat15.z = g_FrameIndexModX.x;
    u_xlat33.x = dot(u_xlat15.xyz, float3(0.117647059, 0.411764711, 1.35294116));
    u_xlat33.x = frac(u_xlat33.x);
    u_xlat16_1.xyz = u_xlat33.xxx * float3(0.00392156886, 0.00392156886, 0.00392156886) + u_xlat16_0.xyz;
    o.SV_Target0.xyz = (bool(u_xlatb6)) ? u_xlat16_1.xyz : u_xlat16_0.xyz;
    o.SV_Target0.w = u_xlat16_0.w;
    return o;

}
ENDHLSL
} } }
