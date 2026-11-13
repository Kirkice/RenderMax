// Associated EIDs: 891
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0891_Back_Opaque" {
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
_AlbedoMap_ST ("_AlbedoMap_ST", Vector) = (0,0,0,0)
_tSpecShiftMap_ST ("_tSpecShiftMap_ST", Vector) = (0,0,0,0)
_AlbedoColor ("_AlbedoColor", Vector) = (0,0,0,0)
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
_SwitchSpecular ("_SwitchSpecular", Float) = 0
_SelfGI ("_SelfGI", Vector) = (0,0,0,0)
_SunShiftOffuse ("_SunShiftOffuse", Float) = 0
_SunShift ("_SunShift", Float) = 0
_AnisotropicMultiplier ("_AnisotropicMultiplier", Float) = 0
_SpecularReflectance ("_SpecularReflectance", Vector) = (0,0,0,0)
_AnisotropicMultiplier2 ("_AnisotropicMultiplier2", Float) = 0
_SunShift2 ("_SunShift2", Float) = 0
_SpecularReflectance2 ("_SpecularReflectance2", Vector) = (0,0,0,0)
_SpOpacity ("_SpOpacity", Float) = 0
_AlbedoMap ("_AlbedoMap", 2D) = "white" {}
_MaterialParamsMap ("_MaterialParamsMap", 2D) = "white" {}
_tSpecShiftMap ("_tSpecShiftMap", 2D) = "white" {}
_NormalMap ("_NormalMap", 2D) = "white" {}
g_EnvmapFilterWithGGX ("g_EnvmapFilterWithGGX", Cube) = "white" {}
g_CSMShadowTexture ("g_CSMShadowTexture", 2D) = "white" {}
hlslcc_zcmpg_CSMShadowTexture ("hlslcc_zcmpg_CSMShadowTexture", 2D) = "white" {}
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
float3 _RMCaptured_WorldSpaceCameraPos;
float4 _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_WorldToObject[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float4 g_FogParam;
float4 g_FogColor;
float _ShadowStrength;
float4 _RMCaptured_unity_WorldTransformParams;
float4 _AlbedoMap_ST;
float4 _tSpecShiftMap_ST;
float4 _AlbedoColor;
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
int _SwitchSpecular;
float4 _SelfGI;
float _SunShiftOffuse;
float _SunShift;
float _AnisotropicMultiplier;
float3 _SpecularReflectance;
float _AnisotropicMultiplier2;
float _SunShift2;
float3 _SpecularReflectance2;
float _SpOpacity;
Texture2D _AlbedoMap; SamplerState sampler_AlbedoMap;
Texture2D _MaterialParamsMap; SamplerState sampler_MaterialParamsMap;
Texture2D _tSpecShiftMap; SamplerState sampler_tSpecShiftMap;
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

















































float4 u_xlat16_0;
float3 u_xlat1;
float4 u_xlat16_1;
bool u_xlatb1;
float4 u_xlat16_2;
float3 u_xlat3;
float4 u_xlat16_3;
bool3 u_xlatb3;
float4 u_xlat16_4;
float3 u_xlat16_5;
float4 u_xlat16_6;
float3 u_xlat16_7;
float2 u_xlat16_8;
float3 u_xlat16_9;
float3 u_xlat16_10;
float3 u_xlat11;
float2 u_xlat16_11;
float4 u_xlat12;
bool u_xlatb12;
float4 u_xlat16_13;
float4 u_xlat14;
bool2 u_xlatb14;
float3 u_xlat15;
float4 u_xlat16;
float4 u_xlat17;
float4 u_xlat16_18;
float3 u_xlat16_19;
float3 u_xlat16_20;
float3 u_xlat16_21;
float3 u_xlat16_22;
float3 u_xlat16_23;
float3 u_xlat16_24;
float3 u_xlat16_25;
float3 u_xlat16_26;
float3 u_xlat16_27;
float3 u_xlat16_28;
float3 u_xlat16_29;
float u_xlat30;
bool u_xlatb30;
float2 u_xlat16_31;
float3 u_xlat41;
float u_xlat16_41;
bool u_xlatb41;
float u_xlat16_42;
float u_xlat43;
float2 u_xlat16_43;
uint u_xlatu43;
float2 u_xlat16_44;
float2 u_xlat16_58;
bool u_xlatb59;
float u_xlat16_60;
float2 u_xlat16_66;
float u_xlat70;
float u_xlat16_70;
float2 u_xlat72;
bool u_xlatb72;
float2 u_xlat73;
float u_xlat16_87;
float u_xlat16_89;
float u_xlat90;
float u_xlat16_92;
float u_xlat16_94;
float u_xlat16_95;
float u_xlat16_96;
float u_xlat16_97;
float u_xlat98;
bool u_xlatb98;
float u_xlat99;
float u_xlat16_99;
bool u_xlatb99;
float u_xlat16_100;
float u_xlat101;
float u_xlat16_105;
float u_xlat16_106;
float u_xlat16_107;


float4 hlslcc_FragCoord = float4(v.position.xyz, 1.0/v.position.w);
    u_xlat16_0.xy = v.vs_TEXCOORD0.xy * _AlbedoMap_ST.xy + _AlbedoMap_ST.zw;
    u_xlat16_1 = _AlbedoMap.Sample(sampler_AlbedoMap, u_xlat16_0.xy);
    u_xlat16_2.xyz = u_xlat16_1.xyz * float3(0.305306017, 0.305306017, 0.305306017) + float3(0.682171106, 0.682171106, 0.682171106);
    u_xlat16_2.xyz = u_xlat16_1.xyz * u_xlat16_2.xyz + float3(0.0125228781, 0.0125228781, 0.0125228781);
    u_xlat16_2.xyz = u_xlat16_1.xyz * u_xlat16_2.xyz;
    u_xlat16_3 = _MaterialParamsMap.Sample(sampler_MaterialParamsMap, u_xlat16_0.xy);
    u_xlat16_4 = _AlbedoColor + float4(-1.0, -1.0, -1.0, -1.0);
    u_xlat16_4 = u_xlat16_3.wwww * u_xlat16_4 + float4(1.0, 1.0, 1.0, 1.0);
    u_xlat16_5.xyz = u_xlat16_2.xyz * u_xlat16_4.xyz;
    u_xlat16_6.w = u_xlat16_1.w * u_xlat16_4.w;
    u_xlat16_58.xy = v.vs_TEXCOORD0.xy * _tSpecShiftMap_ST.xy + _tSpecShiftMap_ST.zw;
    u_xlat16_1.xy = _tSpecShiftMap.Sample(sampler_tSpecShiftMap, u_xlat16_58.xy).xy;
    u_xlat16_58.xy = u_xlat16_1.xy * float2(2.0, 2.0) + float2(-1.0, -1.0);
    u_xlat16_58.x = u_xlat16_58.x * _SunShift + _SunShiftOffuse;
    u_xlat16_87 = u_xlat16_58.y * _SunShift2;
    u_xlat16_7.xy = u_xlat16_3.xy * float2(_RoughnessMultiplier, _MetallicMultiplier);
    u_xlat16_8.y = max(u_xlat16_7.x, 0.0890000015);
    u_xlat16_89 = u_xlat16_8.y * u_xlat16_8.y;
    u_xlat16_92 = (-u_xlat16_3.y) * _MetallicMultiplier + 1.0;
    u_xlat16_5.xyz = ((float3)(u_xlat16_92)) * u_xlat16_5.xyz;
    u_xlat1.xyz = u_xlat16_2.xyz * u_xlat16_4.xyz + float3(-0.0399999991, -0.0399999991, -0.0399999991);
    u_xlat1.xyz = u_xlat16_7.yyy * u_xlat1.xyz + float3(0.0399999991, 0.0399999991, 0.0399999991);
    u_xlat16_2.x = u_xlat16_3.z * _AnisotropicMultiplier;
    u_xlat16_2.y = u_xlat16_3.z * _AnisotropicMultiplier2;
    u_xlat16_2.xy = u_xlat16_2.xy * float2(2.0, 2.0) + float2(-1.0, -1.0);
    u_xlat16_3.xyz = _NormalMap.Sample(sampler_NormalMap, u_xlat16_0.xy).xyz;
    u_xlat16_4.xyz = u_xlat16_3.xyz * float3(2.0, 2.0, 2.0) + float3(-1.0, -1.0, -1.0);
    u_xlat16_0.x = dot(u_xlat16_4.xyz, u_xlat16_4.xyz);
    u_xlat16_0.x = rsqrt(u_xlat16_0.x);
    u_xlat16_4.xyz = u_xlat16_0.xxx * u_xlat16_4.xyz;
    u_xlatb3.x = _RMCaptured_unity_WorldTransformParams.w>=0.0;
    u_xlat3.x = (u_xlatb3.x) ? 1.0 : -1.0;
    u_xlat16_0.x = u_xlat3.x * v.vs_TANGENT0.w;
    u_xlat16_29.x = dot(v.vs_NORMAL0.xyz, v.vs_NORMAL0.xyz);
    u_xlat16_29.x = rsqrt(u_xlat16_29.x);
    u_xlat16_7.xyz = u_xlat16_29.xxx * v.vs_NORMAL0.yzx;
    u_xlat16_29.x = dot(v.vs_TANGENT0.yzx, u_xlat16_7.xyz);
    u_xlat16_9.xyz = (-u_xlat16_7.xyz) * u_xlat16_29.xxx + v.vs_TANGENT0.yzx;
    u_xlat16_29.x = dot(u_xlat16_9.xyz, u_xlat16_9.xyz);
    u_xlat16_29.x = rsqrt(u_xlat16_29.x);
    u_xlat16_9.xyz = u_xlat16_29.xxx * u_xlat16_9.xyz;
    u_xlat16_10.xyz = u_xlat16_7.yzx * u_xlat16_9.xyz;
    u_xlat16_10.xyz = u_xlat16_7.xyz * u_xlat16_9.yzx + (-u_xlat16_10.xyz);
    u_xlat16_10.xyz = u_xlat16_0.xxx * u_xlat16_10.xyz;
    u_xlat16_10.xyz = u_xlat16_4.yyy * u_xlat16_10.xyz;
    u_xlat16_10.xyz = u_xlat16_4.xxx * u_xlat16_9.zxy + u_xlat16_10.xyz;
    u_xlat16_4.xyz = u_xlat16_4.zzz * u_xlat16_7.zxy + u_xlat16_10.xyz;
    u_xlat16_29.x = dot(u_xlat16_9.zxy, u_xlat16_4.xyz);
    u_xlat16_9.xyz = (-u_xlat16_4.yzx) * u_xlat16_29.xxx + u_xlat16_9.xyz;
    u_xlat16_29.x = dot(u_xlat16_9.xyz, u_xlat16_9.xyz);
    u_xlat16_29.x = rsqrt(u_xlat16_29.x);
    u_xlat16_9.xyz = u_xlat16_29.xxx * u_xlat16_9.xyz;
    u_xlat16_10.xyz = u_xlat16_4.xyz * u_xlat16_9.yzx;
    u_xlat16_10.xyz = u_xlat16_4.zxy * u_xlat16_9.zxy + (-u_xlat16_10.xyz);
    u_xlat16_10.xyz = u_xlat16_0.xxx * u_xlat16_10.xyz;
    u_xlat3.xyz = (-v.vs_POSITIONWS0.xyz) + _RMCaptured_WorldSpaceCameraPos.xyz;
    u_xlat90 = dot(u_xlat3.xyz, u_xlat3.xyz);
    u_xlat90 = sqrt(u_xlat90);
    u_xlat90 = float(1.0) / u_xlat90;
    u_xlat11.xyz = ((float3)(u_xlat90)) * u_xlat3.xyz;
    u_xlat16_8.x = dot(u_xlat16_4.xyz, u_xlat11.xyz);
    u_xlat16_8.x = clamp(u_xlat16_8.x, 0.0, 1.0);
    u_xlatb98 = 0.0<g_LightPositionAndFalloff[0].w;
    if(u_xlatb98){
        u_xlat12.xyz = (-v.vs_POSITIONWS0.xyz) + g_LightPositionAndFalloff[0].xyz;
        u_xlat16_0.x = dot(u_xlat12.xyz, u_xlat12.xyz);
        u_xlat16_29.x = rsqrt(u_xlat16_0.x);
        u_xlat16_13.xyz = u_xlat16_29.xxx * u_xlat12.xyz;
        u_xlat16_29.x = dot(u_xlat16_13.xyz, g_LightDirectionAndAngleOffset[0].xyz);
        u_xlat16_60 = max(u_xlat16_0.x, 9.99999975e-05);
        u_xlat16_60 = float(1.0) / u_xlat16_60;
        u_xlat16_0.x = u_xlat16_0.x * g_LightPositionAndFalloff[0].w;
        u_xlat98 = (-u_xlat16_0.x) * u_xlat16_0.x + 1.0;
        u_xlat98 = max(u_xlat98, 0.0);
        u_xlat16_0.x = u_xlat16_29.x * abs(g_LightIntensityAndAngleScale[0].w) + g_LightDirectionAndAngleOffset[0].w;
        u_xlat16_0.x = clamp(u_xlat16_0.x, 0.0, 1.0);
        u_xlat16_0.x = u_xlat16_0.x * u_xlat98;
        u_xlat16_0.x = u_xlat16_0.x * u_xlat16_0.x;
        u_xlat16_0.x = u_xlat16_60 * u_xlat16_0.x;
    } else {
        u_xlat16_13.xyz = g_LightDirectionAndAngleOffset[0].xyz;
        u_xlat16_0.x = 1.0;
    }
    u_xlat16_29.x = dot(u_xlat16_4.xyz, u_xlat16_13.xyz);
    u_xlat16_29.x = clamp(u_xlat16_29.x, 0.0, 1.0);
    u_xlatb98 = g_CSMNormalBias.x<0.0;
    u_xlat12.x = v.vs_POSITIONWS0.y * _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[1].w;
    u_xlat12.x = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[0].w * v.vs_POSITIONWS0.x + u_xlat12.x;
    u_xlat12.x = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[2].w * v.vs_POSITIONWS0.z + u_xlat12.x;
    u_xlat12.x = u_xlat12.x + _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[3].w;
    u_xlat12.x = u_xlat12.x * g_CSMNormalBias.x;
    u_xlat16_60 = (u_xlatb98) ? abs(u_xlat12.x) : g_CSMNormalBias.x;
    u_xlat16_60 = u_xlat16_60 + u_xlat16_60;
    u_xlat16_92 = (-u_xlat16_29.x) + 2.0;
    u_xlat16_60 = u_xlat16_60 * u_xlat16_92;
    u_xlat12.xyz = ((float3)(u_xlat16_60)) * u_xlat16_7.zxy + v.vs_POSITIONWS0.xyz;
    u_xlat14 = u_xlat12.yyyy * _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[1];
    u_xlat14 = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[0] * u_xlat12.xxxx + u_xlat14;
    u_xlat12 = _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[2] * u_xlat12.zzzz + u_xlat14;
    u_xlat12 = u_xlat12 + _RMCaptured_hlslcc_mtx4x4g_CSMShadowMatrices[3];
    u_xlat98 = u_xlat12.z * g_CSMDistanceFade.x + g_CSMDistanceFade.y;
    u_xlat98 = clamp(u_xlat98, 0.0, 1.0);
    u_xlatb14.x = u_xlat98<1.0;
    if(u_xlatb14.x){
        u_xlat12.xyz = u_xlat12.xyz / u_xlat12.www;
        u_xlatb14.xy = ((abs(u_xlat12.xyxx)) < ( float4(0.9921875, 0.9921875, 0.0, 0.0))).xy;
        u_xlatb99 = u_xlatb14.y && u_xlatb14.x;
        if(u_xlatb99){
            u_xlat12.xy = u_xlat12.xy * g_CSMUvScaleOffset[0].xy + g_CSMUvScaleOffset[0].zw;
            u_xlat70 = min(u_xlat12.z, 0.999899983);
            u_xlat12.xy = u_xlat12.xy * g_CSMShadowTexture_TexelSize.zw + float2(0.5, 0.5);
            u_xlat14.xy = floor(u_xlat12.xy);
            u_xlat12.xy = u_xlat12.xy + (-u_xlat14.xy);
            u_xlat14.xy = u_xlat14.xy + float2(-0.5, -0.5);
            u_xlat14.xy = u_xlat14.xy * g_CSMShadowTexture_TexelSize.xy;
            u_xlat72.xy = (-u_xlat12.yx) * float2(2.0, 2.0) + float2(3.0, 3.0);
            u_xlat15.xy = u_xlat12.xy * float2(2.0, 2.0) + float2(1.0, 1.0);
            u_xlat73.xy = (-u_xlat12.xy) + float2(2.0, 2.0);
            u_xlat73.xy = u_xlat73.xy / u_xlat72.yx;
            u_xlat12.xy = u_xlat12.xy / u_xlat15.xy;
            u_xlat16.zw = u_xlat73.xy + float2(-1.0, -1.0);
            u_xlat16.xy = u_xlat12.xy + float2(1.0, 1.0);
            u_xlat12.x = u_xlat72.x * u_xlat72.y;
            u_xlat17 = u_xlat16.zwxw * g_CSMShadowTexture_TexelSize.xyxy + u_xlat14.xyxy;
            float3 txVec0 = float3(u_xlat17.xy,u_xlat70);
            u_xlat16_41 = step((txVec0).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec0).xy, 0).r);
            u_xlat72.xy = u_xlat72.xy * u_xlat15.xy;
            float3 txVec1 = float3(u_xlat17.zw,u_xlat70);
            u_xlat16_99 = step((txVec1).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec1).xy, 0).r);
            u_xlat99 = u_xlat16_99 * u_xlat72.x;
            u_xlat12.x = u_xlat12.x * u_xlat16_41 + u_xlat99;
            u_xlat16 = u_xlat16.zyxy * g_CSMShadowTexture_TexelSize.xyxy + u_xlat14.xyxy;
            float3 txVec2 = float3(u_xlat16.xy,u_xlat70);
            u_xlat16_41 = step((txVec2).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec2).xy, 0).r);
            u_xlat12.x = u_xlat72.y * u_xlat16_41 + u_xlat12.x;
            u_xlat41.x = u_xlat15.y * u_xlat15.x;
            float3 txVec3 = float3(u_xlat16.zw,u_xlat70);
            u_xlat16_70 = step((txVec3).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec3).xy, 0).r);
            u_xlat12.x = u_xlat41.x * u_xlat16_70 + u_xlat12.x;
            u_xlat41.x = u_xlat12.x * 0.0625;
            u_xlat16_60 = (-u_xlat12.x) * 0.0625 + 1.0;
            u_xlat16_60 = u_xlat98 * u_xlat16_60 + u_xlat41.x;
        } else {
            u_xlat16_60 = 1.0;
        }
    } else {
        u_xlat16_60 = 1.0;
    }
    u_xlatb98 = 0.0<_SpOpacity;
    u_xlatb12 = u_xlat16_6.w==0.0;
    u_xlatb12 = u_xlatb98 && u_xlatb12;
    if(!u_xlatb12){
        u_xlatb41 = 0.0<u_xlat16_29.x;
        if(u_xlatb41){
            u_xlat16_18.xyz = u_xlat16_0.xxx * g_LightIntensityAndAngleScale[0].xyz;
            u_xlat16_19.xyz = u_xlat3.xyz * ((float3)(u_xlat90)) + u_xlat16_13.xyz;
            u_xlat16_0.x = dot(u_xlat16_19.xyz, u_xlat16_19.xyz);
            u_xlat16_0.x = rsqrt(u_xlat16_0.x);
            u_xlat16_19.xyz = u_xlat16_0.xxx * u_xlat16_19.xyz;
            u_xlat16_0.x = dot(u_xlat16_4.xyz, u_xlat16_19.xyz);
            u_xlat16_0.x = clamp(u_xlat16_0.x, 0.0, 1.0);
            u_xlat16_92 = dot(u_xlat16_13.xyz, u_xlat16_19.xyz);
            u_xlat16_92 = clamp(u_xlat16_92, 0.0, 1.0);
            u_xlat16_60 = u_xlat16_60 + -1.0;
            u_xlat16_60 = _ShadowStrength * u_xlat16_60 + 1.0;
            u_xlat41.xyz = u_xlat16_5.xyz * float3(0.318309873, 0.318309873, 0.318309873);
            u_xlat16_20.xyz = u_xlat16_58.xxx * u_xlat16_4.xyz + u_xlat16_10.zxy;
            u_xlat16_94 = dot(u_xlat16_20.xyz, u_xlat16_20.xyz);
            u_xlat16_94 = rsqrt(u_xlat16_94);
            u_xlat16_20.xyz = ((float3)(u_xlat16_94)) * u_xlat16_20.xyz;
            u_xlat16_94 = u_xlat16_2.x * u_xlat16_89;
            u_xlat16_66.x = u_xlat16_8.y * u_xlat16_8.y + u_xlat16_94;
            u_xlat16_66.x = max(u_xlat16_66.x, 0.00792100001);
            u_xlat16_94 = u_xlat16_8.y * u_xlat16_8.y + (-u_xlat16_94);
            u_xlat16_94 = max(u_xlat16_94, 0.00792100001);
            u_xlat16_95 = dot(u_xlat16_9.zxy, u_xlat16_19.xyz);
            u_xlat16_96 = dot(u_xlat16_20.xyz, u_xlat16_19.xyz);
            u_xlat16_97 = dot(u_xlat16_9.zxy, u_xlat11.xyz);
            u_xlat16_100 = dot(u_xlat16_20.xyz, u_xlat11.xyz);
            u_xlat16_105 = dot(u_xlat16_9.zxy, u_xlat16_13.xyz);
            u_xlat16_106 = dot(u_xlat16_20.xyz, u_xlat16_13.xyz);
            u_xlat16_20.x = u_xlat16_94 * u_xlat16_66.x;
            u_xlat14.x = u_xlat16_0.x * u_xlat16_20.x;
            u_xlat16_43.x = u_xlat16_94 * u_xlat16_95;
            u_xlat16_43.y = u_xlat16_66.x * u_xlat16_96;
            u_xlat14.yz = u_xlat16_43.xy;
            u_xlat14.x = dot(u_xlat14.xyz, u_xlat14.xyz);
            u_xlat14.x = u_xlat16_20.x / u_xlat14.x;
            u_xlat43 = u_xlat14.x * u_xlat16_20.x;
            u_xlat14.x = u_xlat14.x * u_xlat43;
            u_xlat14.x = u_xlat14.x * 0.318309873;
            u_xlat16_96 = u_xlat16_66.x * u_xlat16_97;
            u_xlat16_100 = u_xlat16_94 * u_xlat16_100;
            u_xlat16_100 = u_xlat16_100 * u_xlat16_100;
            u_xlat16_96 = u_xlat16_96 * u_xlat16_96 + u_xlat16_100;
            u_xlat16_96 = u_xlat16_8.x * u_xlat16_8.x + u_xlat16_96;
            u_xlat16_96 = sqrt(u_xlat16_96);
            u_xlat16_96 = u_xlat16_8.x + u_xlat16_96;
            u_xlat16_66.x = u_xlat16_66.x * u_xlat16_105;
            u_xlat16_94 = u_xlat16_94 * u_xlat16_106;
            u_xlat16_94 = u_xlat16_94 * u_xlat16_94;
            u_xlat16_94 = u_xlat16_66.x * u_xlat16_66.x + u_xlat16_94;
            u_xlat16_94 = u_xlat16_29.x * u_xlat16_29.x + u_xlat16_94;
            u_xlat16_94 = sqrt(u_xlat16_94);
            u_xlat16_94 = u_xlat16_29.x + u_xlat16_94;
            u_xlat16_94 = u_xlat16_94 * u_xlat16_96;
            u_xlat16_94 = float(1.0) / u_xlat16_94;
            u_xlat16_92 = (-u_xlat16_92) + 1.0;
            u_xlat16_66.x = u_xlat16_92 * u_xlat16_92;
            u_xlat16_66.x = u_xlat16_66.x * u_xlat16_66.x;
            u_xlat16_96 = u_xlat16_92 * u_xlat16_66.x;
            u_xlat16_100 = u_xlat1.y * 50.0;
            u_xlat16_100 = clamp(u_xlat16_100, 0.0, 1.0);
            u_xlat16_92 = (-u_xlat16_92) * u_xlat16_66.x + 1.0;
            u_xlat16_20.xyz = u_xlat1.xyz * ((float3)(u_xlat16_92));
            u_xlat16_20.xyz = ((float3)(u_xlat16_100)) * ((float3)(u_xlat16_96)) + u_xlat16_20.xyz;
            u_xlat16_92 = u_xlat16_94 * u_xlat14.x;
            u_xlat16_21.xyz = ((float3)(u_xlat16_92)) * u_xlat16_20.xyz;
            u_xlat16_21.xyz = u_xlat16_21.xyz * _SpecularReflectance.xyz;
            u_xlatb14.x = 0<_SwitchSpecular;
            if(u_xlatb14.x){
                u_xlat16_22.xyz = ((float3)(u_xlat16_87)) * u_xlat16_4.xyz + u_xlat16_10.zxy;
                u_xlat16_92 = dot(u_xlat16_22.xyz, u_xlat16_22.xyz);
                u_xlat16_92 = rsqrt(u_xlat16_92);
                u_xlat16_22.xyz = ((float3)(u_xlat16_92)) * u_xlat16_22.xyz;
                u_xlat16_92 = u_xlat16_2.y * u_xlat16_89;
                u_xlat16_94 = u_xlat16_8.y * u_xlat16_8.y + u_xlat16_92;
                u_xlat16_94 = max(u_xlat16_94, 0.00792100001);
                u_xlat16_92 = u_xlat16_8.y * u_xlat16_8.y + (-u_xlat16_92);
                u_xlat16_92 = max(u_xlat16_92, 0.00792100001);
                u_xlat16_66.x = dot(u_xlat16_22.xyz, u_xlat16_19.xyz);
                u_xlat16_96 = dot(u_xlat16_22.xyz, u_xlat11.xyz);
                u_xlat16_13.x = dot(u_xlat16_22.xyz, u_xlat16_13.xyz);
                u_xlat16_42 = u_xlat16_92 * u_xlat16_94;
                u_xlat14.x = u_xlat16_0.x * u_xlat16_42;
                u_xlat16_43.x = u_xlat16_95 * u_xlat16_92;
                u_xlat16_43.y = u_xlat16_94 * u_xlat16_66.x;
                u_xlat14.yz = u_xlat16_43.xy;
                u_xlat14.x = dot(u_xlat14.xyz, u_xlat14.xyz);
                u_xlat14.x = u_xlat16_42 / u_xlat14.x;
                u_xlat43 = u_xlat16_42 * u_xlat14.x;
                u_xlat14.x = u_xlat14.x * u_xlat43;
                u_xlat14.x = u_xlat14.x * 0.318309873;
                u_xlat16_0.x = u_xlat16_94 * u_xlat16_97;
                u_xlat16_66.x = u_xlat16_92 * u_xlat16_96;
                u_xlat16_66.x = u_xlat16_66.x * u_xlat16_66.x;
                u_xlat16_0.x = u_xlat16_0.x * u_xlat16_0.x + u_xlat16_66.x;
                u_xlat16_0.x = u_xlat16_8.x * u_xlat16_8.x + u_xlat16_0.x;
                u_xlat16_0.x = sqrt(u_xlat16_0.x);
                u_xlat16_0.x = u_xlat16_0.x + u_xlat16_8.x;
                u_xlat16_94 = u_xlat16_94 * u_xlat16_105;
                u_xlat16_92 = u_xlat16_92 * u_xlat16_13.x;
                u_xlat16_92 = u_xlat16_92 * u_xlat16_92;
                u_xlat16_92 = u_xlat16_94 * u_xlat16_94 + u_xlat16_92;
                u_xlat16_92 = u_xlat16_29.x * u_xlat16_29.x + u_xlat16_92;
                u_xlat16_92 = sqrt(u_xlat16_92);
                u_xlat16_92 = u_xlat16_29.x + u_xlat16_92;
                u_xlat16_0.x = u_xlat16_0.x * u_xlat16_92;
                u_xlat16_0.x = float(1.0) / u_xlat16_0.x;
                u_xlat16_0.x = u_xlat16_0.x * u_xlat14.x;
                u_xlat16_13.xyz = u_xlat16_0.xxx * u_xlat16_20.xyz;
                u_xlat16_21.xyz = u_xlat16_13.xyz * float3(_SpecularReflectance2.x, _SpecularReflectance2.y, _SpecularReflectance2.z) + u_xlat16_21.xyz;
            }
            u_xlat16_0.x = u_xlat16_29.x * u_xlat16_60;
            u_xlat16_13.xyz = u_xlat16_0.xxx * u_xlat16_18.xyz;
            u_xlat16_18.xyz = u_xlat16_13.xyz * u_xlat16_21.xyz;
            u_xlat16_13.xyz = u_xlat41.xyz * u_xlat16_13.xyz + u_xlat16_18.xyz;
        } else {
            u_xlat16_18.x = float(0.0);
            u_xlat16_18.y = float(0.0);
            u_xlat16_18.z = float(0.0);
            u_xlat16_13.x = float(0.0);
            u_xlat16_13.y = float(0.0);
            u_xlat16_13.z = float(0.0);
        }
    } else {
        u_xlat16_18.x = float(0.0);
        u_xlat16_18.y = float(0.0);
        u_xlat16_18.z = float(0.0);
        u_xlat16_13.x = float(0.0);
        u_xlat16_13.y = float(0.0);
        u_xlat16_13.z = float(0.0);
    }
    u_xlat41.xyz = u_xlat16_5.xyz * float3(0.318309873, 0.318309873, 0.318309873);
    u_xlat16_19.xyz = u_xlat16_58.xxx * u_xlat16_4.xyz + u_xlat16_10.zxy;
    u_xlat16_0.x = dot(u_xlat16_19.xyz, u_xlat16_19.xyz);
    u_xlat16_0.x = rsqrt(u_xlat16_0.x);
    u_xlat16_19.xyz = u_xlat16_0.xxx * u_xlat16_19.xyz;
    u_xlat16_0.xy = u_xlat16_2.xy * ((float2)(u_xlat16_89));
    u_xlat16_31.xy = u_xlat16_8.yy * u_xlat16_8.yy + u_xlat16_0.xy;
    u_xlat16_31.xy = max(u_xlat16_31.xy, float2(0.00792100001, 0.00792100001));
    u_xlat16_0.xy = u_xlat16_8.yy * u_xlat16_8.yy + (-u_xlat16_0.xy);
    u_xlat16_0.xy = max(u_xlat16_0.xy, float2(0.00792100001, 0.00792100001));
    u_xlat16_92 = dot(u_xlat16_9.zxy, u_xlat11.xyz);
    u_xlat16_94 = dot(u_xlat16_19.xyz, u_xlat11.xyz);
    u_xlat16_66.xy = u_xlat16_31.xy * ((float2)(u_xlat16_92));
    u_xlat16_66.xy = u_xlat16_66.xy * u_xlat16_66.xy;
    u_xlat16_92 = u_xlat16_0.x * u_xlat16_94;
    u_xlat16_92 = u_xlat16_92 * u_xlat16_92 + u_xlat16_66.x;
    u_xlat16_92 = u_xlat16_8.x * u_xlat16_8.x + u_xlat16_92;
    u_xlat16_92 = sqrt(u_xlat16_92);
    u_xlat16_92 = u_xlat16_92 + u_xlat16_8.x;
    u_xlat16_94 = u_xlat1.y * 50.0;
    u_xlat16_94 = clamp(u_xlat16_94, 0.0, 1.0);
    u_xlatb14.x = 0<_SwitchSpecular;
    u_xlat16_20.xyz = ((float3)(u_xlat16_87)) * u_xlat16_4.xyz + u_xlat16_10.zxy;
    u_xlat16_87 = dot(u_xlat16_20.xyz, u_xlat16_20.xyz);
    u_xlat16_87 = rsqrt(u_xlat16_87);
    u_xlat16_20.xyz = ((float3)(u_xlat16_87)) * u_xlat16_20.xyz;
    u_xlat16_87 = dot(u_xlat16_20.xyz, u_xlat11.xyz);
    u_xlat16_21.xy = u_xlat16_0.xy * u_xlat16_31.xy;
    u_xlat16_87 = u_xlat16_0.y * u_xlat16_87;
    u_xlat16_87 = u_xlat16_87 * u_xlat16_87 + u_xlat16_66.y;
    u_xlat16_87 = u_xlat16_8.x * u_xlat16_8.x + u_xlat16_87;
    u_xlat16_87 = sqrt(u_xlat16_87);
    u_xlat16_87 = u_xlat16_87 + u_xlat16_8.x;
    u_xlat16_22.xyz = u_xlat16_18.xyz;
    u_xlat16_23.xyz = u_xlat16_13.xyz;
    for(uint u_xlatu_loop_1 = 1u ; u_xlatu_loop_1<3u ; u_xlatu_loop_1++)
    {
        u_xlatb72 = 0.0<g_LightPositionAndFalloff[int(u_xlatu_loop_1)].w;
        if(u_xlatb72){
            u_xlat15.xyz = (-v.vs_POSITIONWS0.xyz) + g_LightPositionAndFalloff[int(u_xlatu_loop_1)].xyz;
            u_xlat16_66.x = dot(u_xlat15.xyz, u_xlat15.xyz);
            u_xlat16_95 = rsqrt(u_xlat16_66.x);
            u_xlat16_24.xyz = ((float3)(u_xlat16_95)) * u_xlat15.xyz;
            u_xlat16_95 = dot(u_xlat16_24.xyz, g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].xyz);
            u_xlat16_96 = max(u_xlat16_66.x, 9.99999975e-05);
            u_xlat16_96 = float(1.0) / u_xlat16_96;
            u_xlat16_66.x = u_xlat16_66.x * g_LightPositionAndFalloff[int(u_xlatu_loop_1)].w;
            u_xlat72.x = (-u_xlat16_66.x) * u_xlat16_66.x + 1.0;
            u_xlat72.x = max(u_xlat72.x, 0.0);
            u_xlat16_66.x = u_xlat16_95 * g_LightIntensityAndAngleScale[int(u_xlatu_loop_1)].w + g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].w;
            u_xlat16_66.x = clamp(u_xlat16_66.x, 0.0, 1.0);
            u_xlat16_66.x = u_xlat16_66.x * u_xlat72.x;
            u_xlat16_66.x = u_xlat16_66.x * u_xlat16_66.x;
            u_xlat16_66.x = u_xlat16_96 * u_xlat16_66.x;
        } else {
            u_xlat16_24.xyz = g_LightDirectionAndAngleOffset[int(u_xlatu_loop_1)].xyz;
            u_xlat16_66.x = 1.0;
        }
        if(!u_xlatb12){
            u_xlat16_95 = dot(u_xlat16_4.xyz, u_xlat16_24.xyz);
            u_xlat16_95 = clamp(u_xlat16_95, 0.0, 1.0);
            u_xlatb72 = 0.0<u_xlat16_95;
            if(u_xlatb72){
                u_xlat16_25.xyz = u_xlat16_66.xxx * g_LightIntensityAndAngleScale[int(u_xlatu_loop_1)].xyz;
                u_xlat16_26.xyz = u_xlat3.xyz * ((float3)(u_xlat90)) + u_xlat16_24.xyz;
                u_xlat16_66.x = dot(u_xlat16_26.xyz, u_xlat16_26.xyz);
                u_xlat16_66.x = rsqrt(u_xlat16_66.x);
                u_xlat16_26.xyz = u_xlat16_66.xxx * u_xlat16_26.xyz;
                u_xlat16_66.x = dot(u_xlat16_4.xyz, u_xlat16_26.xyz);
                u_xlat16_66.x = clamp(u_xlat16_66.x, 0.0, 1.0);
                u_xlat16_96 = dot(u_xlat16_24.xyz, u_xlat16_26.xyz);
                u_xlat16_96 = clamp(u_xlat16_96, 0.0, 1.0);
                u_xlat16_97 = dot(u_xlat16_9.zxy, u_xlat16_26.xyz);
                u_xlat16_100 = dot(u_xlat16_19.xyz, u_xlat16_26.xyz);
                u_xlat16_105 = dot(u_xlat16_9.zxy, u_xlat16_24.xyz);
                u_xlat16_106 = dot(u_xlat16_19.xyz, u_xlat16_24.xyz);
                u_xlat15.x = u_xlat16_66.x * u_xlat16_21.x;
                u_xlat16_44.x = u_xlat16_0.x * u_xlat16_97;
                u_xlat16_44.y = u_xlat16_31.x * u_xlat16_100;
                u_xlat15.yz = u_xlat16_44.xy;
                u_xlat72.x = dot(u_xlat15.xyz, u_xlat15.xyz);
                u_xlat72.x = u_xlat16_21.x / u_xlat72.x;
                u_xlat101 = u_xlat72.x * u_xlat16_21.x;
                u_xlat72.x = u_xlat72.x * u_xlat101;
                u_xlat72.x = u_xlat72.x * 0.318309873;
                u_xlat16_100 = u_xlat16_31.x * u_xlat16_105;
                u_xlat16_106 = u_xlat16_0.x * u_xlat16_106;
                u_xlat16_106 = u_xlat16_106 * u_xlat16_106;
                u_xlat16_100 = u_xlat16_100 * u_xlat16_100 + u_xlat16_106;
                u_xlat16_100 = u_xlat16_95 * u_xlat16_95 + u_xlat16_100;
                u_xlat16_100 = sqrt(u_xlat16_100);
                u_xlat16_100 = u_xlat16_95 + u_xlat16_100;
                u_xlat16_100 = u_xlat16_92 * u_xlat16_100;
                u_xlat16_100 = float(1.0) / u_xlat16_100;
                u_xlat16_96 = (-u_xlat16_96) + 1.0;
                u_xlat16_106 = u_xlat16_96 * u_xlat16_96;
                u_xlat16_106 = u_xlat16_106 * u_xlat16_106;
                u_xlat16_107 = u_xlat16_96 * u_xlat16_106;
                u_xlat16_96 = (-u_xlat16_96) * u_xlat16_106 + 1.0;
                u_xlat16_27.xyz = u_xlat1.xyz * ((float3)(u_xlat16_96));
                u_xlat16_27.xyz = ((float3)(u_xlat16_94)) * ((float3)(u_xlat16_107)) + u_xlat16_27.xyz;
                u_xlat16_96 = u_xlat16_100 * u_xlat72.x;
                u_xlat16_28.xyz = ((float3)(u_xlat16_96)) * u_xlat16_27.xyz;
                u_xlat16_28.xyz = u_xlat16_28.xyz * _SpecularReflectance.xyz;
                if(u_xlatb14.x){
                    u_xlat16_96 = dot(u_xlat16_20.xyz, u_xlat16_26.xyz);
                    u_xlat16_100 = dot(u_xlat16_20.xyz, u_xlat16_24.xyz);
                    u_xlat15.x = u_xlat16_66.x * u_xlat16_21.y;
                    u_xlat16_44.x = u_xlat16_0.y * u_xlat16_97;
                    u_xlat16_44.y = u_xlat16_31.y * u_xlat16_96;
                    u_xlat15.yz = u_xlat16_44.xy;
                    u_xlat72.x = dot(u_xlat15.xyz, u_xlat15.xyz);
                    u_xlat72.x = u_xlat16_21.y / u_xlat72.x;
                    u_xlat101 = u_xlat72.x * u_xlat16_21.y;
                    u_xlat72.x = u_xlat72.x * u_xlat101;
                    u_xlat72.x = u_xlat72.x * 0.318309873;
                    u_xlat16_66.x = u_xlat16_31.y * u_xlat16_105;
                    u_xlat16_96 = u_xlat16_0.y * u_xlat16_100;
                    u_xlat16_96 = u_xlat16_96 * u_xlat16_96;
                    u_xlat16_66.x = u_xlat16_66.x * u_xlat16_66.x + u_xlat16_96;
                    u_xlat16_66.x = u_xlat16_95 * u_xlat16_95 + u_xlat16_66.x;
                    u_xlat16_66.x = sqrt(u_xlat16_66.x);
                    u_xlat16_66.x = u_xlat16_66.x + u_xlat16_95;
                    u_xlat16_66.x = u_xlat16_87 * u_xlat16_66.x;
                    u_xlat16_66.x = float(1.0) / u_xlat16_66.x;
                    u_xlat16_66.x = u_xlat16_66.x * u_xlat72.x;
                    u_xlat16_24.xyz = u_xlat16_66.xxx * u_xlat16_27.xyz;
                    u_xlat16_28.xyz = u_xlat16_24.xyz * float3(_SpecularReflectance2.x, _SpecularReflectance2.y, _SpecularReflectance2.z) + u_xlat16_28.xyz;
                }
                u_xlat16_24.xyz = ((float3)(u_xlat16_95)) * u_xlat16_25.xyz;
                u_xlat16_25.xyz = u_xlat16_24.xyz * u_xlat16_28.xyz;
                u_xlat16_22.xyz = u_xlat16_28.xyz * u_xlat16_24.xyz + u_xlat16_22.xyz;
                u_xlat16_24.xyz = u_xlat41.xyz * u_xlat16_24.xyz + u_xlat16_25.xyz;
            } else {
                u_xlat16_24.x = float(0.0);
                u_xlat16_24.y = float(0.0);
                u_xlat16_24.z = float(0.0);
            }
        } else {
            u_xlat16_24.x = float(0.0);
            u_xlat16_24.y = float(0.0);
            u_xlat16_24.z = float(0.0);
        }
        u_xlat16_23.xyz = u_xlat16_23.xyz + u_xlat16_24.xyz;
    }
    u_xlat16_13.x = dot(g_EnvmapRotationParam.xy, u_xlat16_4.xz);
    u_xlat16_13.z = dot(g_EnvmapRotationParam.zw, u_xlat16_4.xz);
    u_xlat16_13.y = u_xlat16_4.y;
    u_xlat16_0.xyw = u_xlat16_13.xyz * u_xlat16_13.xyz;
    u_xlat16_18.xyz = min(u_xlat16_0.xyw, float3(1.0, 1.0, 1.0));
    u_xlat16_31.x = dot(u_xlat16_13.xyz, u_xlat16_13.xyz);
    u_xlat16_31.x = (-u_xlat16_31.x) + 1.0;
    u_xlat16_31.x = max(u_xlat16_31.x, 0.0);
    u_xlat16_31.xy = u_xlat16_31.xx * float2(0.400000006, 0.5);
    u_xlat16_19.xyz = u_xlat16_31.xxx * _SelfGI.xyz + u_xlat16_31.yyy;
    u_xlat16_20.xyz = u_xlat16_19.xyz * u_xlat16_0.xxx + u_xlat16_18.xxx;
    u_xlat16_18.xyw = u_xlat16_19.xyz * u_xlat16_0.yyy + u_xlat16_18.yyy;
    u_xlat16_0.xyw = u_xlat16_19.xyz * u_xlat16_0.www + u_xlat16_18.zzz;
    u_xlatb3.xyz = ((u_xlat16_13.xyzx) >= ( float4(0.0, 0.0, 0.0, 0.0))).xyz;
    u_xlat16_13.x = (u_xlatb3.x) ? float(1.0) : float(0.0);
    u_xlat16_13.y = (u_xlatb3.y) ? float(1.0) : float(0.0);
    u_xlat16_13.z = (u_xlatb3.z) ? float(1.0) : float(0.0);
    u_xlat16_19.x = (u_xlatb3.x) ? float(0.0) : float(1.0);
    u_xlat16_19.y = (u_xlatb3.y) ? float(0.0) : float(1.0);
    u_xlat16_19.z = (u_xlatb3.z) ? float(0.0) : float(1.0);
    u_xlat16_21.xyz = u_xlat16_20.xyz * g_IrradianceACCoeffs[0].xyz;
    u_xlat16_20.xyz = u_xlat16_20.xyz * g_IrradianceACCoeffs[1].xyz;
    u_xlat16_20.xyz = u_xlat16_19.xxx * u_xlat16_20.xyz;
    u_xlat16_20.xyz = u_xlat16_21.xyz * u_xlat16_13.xxx + u_xlat16_20.xyz;
    u_xlat16_21.xyz = u_xlat16_18.xyw * g_IrradianceACCoeffs[2].xyz;
    u_xlat16_13.xyw = u_xlat16_21.xyz * u_xlat16_13.yyy + u_xlat16_20.xyz;
    u_xlat16_18.xyz = u_xlat16_18.xyw * g_IrradianceACCoeffs[3].xyz;
    u_xlat16_13.xyw = u_xlat16_18.xyz * u_xlat16_19.yyy + u_xlat16_13.xyw;
    u_xlat16_18.xyz = u_xlat16_0.xyw * g_IrradianceACCoeffs[4].xyz;
    u_xlat16_13.xyz = u_xlat16_18.xyz * u_xlat16_13.zzz + u_xlat16_13.xyw;
    u_xlat16_0.xyw = u_xlat16_0.xyw * g_IrradianceACCoeffs[5].xyz;
    u_xlat16_0.xyw = u_xlat16_0.xyw * u_xlat16_19.zzz + u_xlat16_13.xyz;
    u_xlat16_0.xyw = u_xlat16_0.xyw * g_EnvmapIntensity.xyz;
    u_xlat16_0.xyw = (bool(u_xlatb12)) ? float3(0.0, 0.0, 0.0) : u_xlat16_0.xyw;
    u_xlat16_5.xyz = u_xlat16_0.xyw * u_xlat16_5.xyz + u_xlat16_23.xyz;
    if(!u_xlatb12){
        u_xlat16_0.x = dot(u_xlat16_0.xyw, float3(0.333299994, 0.333299994, 0.333299994));
        u_xlat16_29.xyz = u_xlat16_58.xxx * u_xlat16_7.xyz + u_xlat16_10.xyz;
        u_xlat16_31.x = dot(u_xlat16_29.xyz, u_xlat16_29.xyz);
        u_xlat16_31.x = rsqrt(u_xlat16_31.x);
        u_xlat16_29.xyz = u_xlat16_29.xyz * u_xlat16_31.xxx;
        u_xlat16_31.x = -abs(u_xlat16_2.x) * 0.800000012 + 1.0;
        u_xlat16_31.x = u_xlat16_31.x * u_xlat16_8.y;
        u_xlat16_31.x = u_xlat16_31.x * g_EnvmapMipLevelsUsed + (-u_xlat16_31.x);
        u_xlatb3.x = u_xlat16_2.x>=0.0;
        u_xlat16_29.xyz = (u_xlatb3.x) ? u_xlat16_29.xyz : u_xlat16_9.xyz;
        u_xlat16_7.xyz = u_xlat11.xyz * u_xlat16_29.xyz;
        u_xlat16_7.xyz = u_xlat16_29.zxy * u_xlat11.yzx + (-u_xlat16_7.xyz);
        u_xlat16_9.xyz = u_xlat16_29.xyz * u_xlat16_7.xyz;
        u_xlat16_29.xyz = u_xlat16_7.zxy * u_xlat16_29.yzx + (-u_xlat16_9.xyz);
        u_xlat16_60 = u_xlat16_89 * 8.0;
        u_xlat16_60 = min(u_xlat16_60, 1.0);
        u_xlat16_60 = u_xlat16_60 * abs(u_xlat16_2.x);
        u_xlat16_29.xyz = (-u_xlat16_4.xyz) + u_xlat16_29.xyz;
        u_xlat16_29.xyz = ((float3)(u_xlat16_60)) * u_xlat16_29.xyz + u_xlat16_4.xyz;
        u_xlat16_60 = dot(u_xlat16_29.xyz, u_xlat16_29.xyz);
        u_xlat16_60 = rsqrt(u_xlat16_60);
        u_xlat16_29.xyz = u_xlat16_29.xyz * ((float3)(u_xlat16_60));
        u_xlat16_60 = dot((-u_xlat11.xyz), u_xlat16_29.xyz);
        u_xlat16_60 = u_xlat16_60 + u_xlat16_60;
        u_xlat16_29.xyz = u_xlat16_29.xyz * (-((float3)(u_xlat16_60))) + (-u_xlat11.xyz);
        u_xlat16_60 = u_xlat16_89 * u_xlat16_89;
        u_xlat16_4.xyz = (-u_xlat16_29.xyz) + u_xlat16_4.xyz;
        u_xlat16_4.xyz = ((float3)(u_xlat16_60)) * u_xlat16_4.xyz + u_xlat16_29.xyz;
        u_xlat16_29.xyz = u_xlat16_29.xyz + (-u_xlat16_4.xyz);
        u_xlat16_29.xyz = abs(u_xlat16_2.xxx) * u_xlat16_29.xyz + u_xlat16_4.xyz;
        u_xlat16_4.x = dot(g_EnvmapRotationParam.xy, u_xlat16_29.xz);
        u_xlat16_4.z = dot(g_EnvmapRotationParam.zw, u_xlat16_29.xz);
        u_xlat16_4.y = u_xlat16_29.y;
        u_xlat16_2 = g_EnvmapFilterWithGGX.SampleLevel(samplerg_EnvmapFilterWithGGX, u_xlat16_4.xyz, u_xlat16_31.x);
        u_xlat16_29.x = u_xlat16_2.w * 6.0;
        u_xlat16_29.xyz = u_xlat16_29.xxx * u_xlat16_2.xyz;
        u_xlat16_29.xyz = u_xlat16_29.xyz * u_xlat16_29.xyz;
        u_xlat16_29.xyz = u_xlat16_29.xyz * g_EnvmapIntensity.xyz;
        u_xlatb3.x = 0.0<g_EnvmapIntensity.w;
        u_xlat3.x = (u_xlatb3.x) ? u_xlat16_0.x : 1.0;
        u_xlat3.xyz = u_xlat16_29.xyz * u_xlat3.xxx;
        u_xlat16_11.xy = s_DFGTexture.Sample(samplers_DFGTexture, u_xlat16_8.xy).xy;
        u_xlat16_0.x = u_xlat16_94 * u_xlat16_11.y;
        u_xlat16_0.xyz = u_xlat1.xyz * u_xlat16_11.xxx + u_xlat16_0.xxx;
        u_xlat16_4.xyz = u_xlat16_0.xyz * u_xlat3.xyz;
        u_xlat16_0.xyz = u_xlat3.xyz * u_xlat16_0.xyz + u_xlat16_22.xyz;
        u_xlat16_0.x = dot(u_xlat16_0.xyz, float3(0.219999999, 0.707000017, 0.0710000023));
        u_xlat16_0.x = clamp(u_xlat16_0.x, 0.0, 1.0);
        u_xlat16_0.x = u_xlat16_1.w * u_xlat16_4.w + u_xlat16_0.x;
        u_xlat16_6.w = (u_xlatb98) ? u_xlat16_0.x : u_xlat16_6.w;
    } else {
        u_xlat16_4.x = float(0.0);
        u_xlat16_4.y = float(0.0);
        u_xlat16_4.z = float(0.0);
    }
    u_xlat16_0.xyz = u_xlat16_4.xyz + u_xlat16_5.xyz;
    u_xlat16_4.xyz = (-u_xlat16_0.xyz) + g_FogColor.xyz;
    u_xlat16_0.xyz = v.vs_POSITIONWS0.www * u_xlat16_4.xyz + u_xlat16_0.xyz;
    u_xlat16_4.xyz = u_xlat16_0.yyy * _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[1].xyz;
    u_xlat16_0.xyw = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[0].xyz * u_xlat16_0.xxx + u_xlat16_4.xyz;
    u_xlat16_0.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[2].xyz * u_xlat16_0.zzz + u_xlat16_0.xyw;
    u_xlat16_0.xyz = u_xlat16_0.xyz + _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].xyz;
    u_xlatb1 = 0.0>=_RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].w;
    u_xlat16_4.xyz = u_xlat16_0.xyz + float3(0.155000001, 0.155000001, 0.155000001);
    u_xlat16_4.xyz = u_xlat16_0.xyz / u_xlat16_4.xyz;
    u_xlat16_2.xyz = u_xlat16_4.xyz * float3(1.01900005, 1.01900005, 1.01900005);
    u_xlatb30 = 0.0>=_BlendMode;
    u_xlatb59 = g_AlphaOutput<1.0;
    u_xlatb30 = u_xlatb59 && u_xlatb30;
    u_xlat16_87 = dot(u_xlat16_2.xyz, float3(0.333299994, 0.333299994, 0.333299994));
    u_xlat16_2.w = (u_xlatb30) ? u_xlat16_87 : u_xlat16_6.w;
    u_xlat16_6.xyz = min(u_xlat16_0.xyz, float3(65504.0, 65504.0, 65504.0));
    u_xlat16_0 = (bool(u_xlatb1)) ? u_xlat16_2 : u_xlat16_6;
    u_xlatb1 = 2.0<g_ExposureScale.w;
    u_xlat3.xy = hlslcc_FragCoord.xy;
    u_xlat3.z = g_FrameIndexModX.x;
    u_xlat30 = dot(u_xlat3.xyz, float3(0.117647059, 0.411764711, 1.35294116));
    u_xlat30 = frac(u_xlat30);
    u_xlat16_4.xyz = ((float3)(u_xlat30)) * float3(0.00392156886, 0.00392156886, 0.00392156886) + u_xlat16_0.xyz;
    o.SV_Target0.xyz = (bool(u_xlatb1)) ? u_xlat16_4.xyz : u_xlat16_0.xyz;
    o.SV_Target0.w = u_xlat16_0.w;
    return o;

}
ENDHLSL
} } }
