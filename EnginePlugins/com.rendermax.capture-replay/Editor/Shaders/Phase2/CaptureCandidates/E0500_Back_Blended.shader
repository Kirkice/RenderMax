// Associated EIDs: 500
// Original shader name: unavailable in exported evidence.
// GENERATED CANDIDATE: not compiled or GPU validated. Captured matrices must be bound.
Shader "RenderMax/Phase2/Editable/E0500_Back_Blended" {
Properties { _RMClipDepth ("GL to D3D clip depth (unvalidated)", Vector) = (0.5,0.5,0,0)
_RMCull ("Cull", Float) = 0
_RMZTest ("Depth test", Float) = 8
_RMZWrite ("Depth write", Float) = 0
_RMColorMask ("Color mask", Float) = 15
_RMCaptured_Time ("_RMCaptured_Time", Vector) = (0,0,0,0)
_RMCaptured_ProjectionParams ("_RMCaptured_ProjectionParams", Vector) = (0,0,0,0)
_MainTex_ST ("_MainTex_ST", Vector) = (0,0,0,0)
_ReflectMask_ST ("_ReflectMask_ST", Vector) = (0,0,0,0)
_ReflectMaskTime ("_ReflectMaskTime", Vector) = (0,0,0,0)
_RMCaptured_WorldSpaceCameraPos ("_RMCaptured_WorldSpaceCameraPos", Vector) = (0,0,0,0)
g_CSMShadowTexture_TexelSize ("g_CSMShadowTexture_TexelSize", Vector) = (0,0,0,0)
_BaseColor ("_BaseColor", Vector) = (0,0,0,0)
_UVXX ("_UVXX", Vector) = (0,0,0,0)
_ShadowColor ("_ShadowColor", Vector) = (0,0,0,0)
_ReflectColor ("_ReflectColor", Vector) = (0,0,0,0)
_Metalness ("_Metalness", Float) = 0
_BaseTwist ("_BaseTwist", Float) = 0
_ShadowDistance1 ("_ShadowDistance1", Float) = 0
_ShadowDistance2 ("_ShadowDistance2", Float) = 0
_ShadowTwist ("_ShadowTwist", Vector) = (0,0,0,0)
_ReflectMask ("_ReflectMask", 2D) = "white" {}
_ReflectionTex ("_ReflectionTex", 2D) = "white" {}
_MainTex ("_MainTex", 2D) = "white" {}
g_CSMShadowTexture ("g_CSMShadowTexture", 2D) = "white" {}
hlslcc_zcmpg_CSMShadowTexture ("hlslcc_zcmpg_CSMShadowTexture", 2D) = "white" {} }
SubShader { Pass {
// Pipeline state sourced from the first associated Draw; verify per-Draw state before replay.
Cull Back
ZTest LEqual
ZWrite On
Blend SrcAlpha OneMinusSrcAlpha, SrcAlpha OneMinusSrcAlpha
BlendOp Add, Add
ColorMask RGBA
HLSLPROGRAM
#pragma target 3.5
#pragma prefer_hlslcc gles
#pragma vertex RMVertex
#pragma fragment RMFragment
#include "UnityCG.cginc"
float4 _RMCaptured_Time;
float4 _RMCaptured_ProjectionParams;
float4 _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[4];
float4 _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[4];
float4 _RMCaptured_hlslcc_mtx4x4_SGameShadowMatrix[4];
float4 _MainTex_ST;
float4 _ReflectMask_ST;
float4 _ReflectMaskTime;
float3 _RMCaptured_WorldSpaceCameraPos;
float4 g_CSMShadowTexture_TexelSize;
float4 _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[4];
float4 _BaseColor;
float4 _UVXX;
float4 _ShadowColor;
float4 _ReflectColor;
float _Metalness;
float _BaseTwist;
float _ShadowDistance1;
float _ShadowDistance2;
float4 _ShadowTwist;
Texture2D _ReflectMask; SamplerState sampler_ReflectMask;
Texture2D _ReflectionTex; SamplerState sampler_ReflectionTex;
Texture2D _MainTex; SamplerState sampler_MainTex;
Texture2D g_CSMShadowTexture; SamplerState samplerg_CSMShadowTexture;
Texture2D hlslcc_zcmpg_CSMShadowTexture; SamplerState samplerhlslcc_zcmpg_CSMShadowTexture; // Captured D16 stored as RFloat; explicit <= comparison.
float4 _RMClipDepth;
struct RMInput { float4 in_POSITION0 : POSITION0;
float3 in_NORMAL0 : NORMAL0;
float2 in_TEXCOORD0 : TEXCOORD0; };
struct RMVaryings { float4 position : SV_POSITION;
float3 vs_NORMAL0 : TEXCOORD0;
float4 vs_TEXCOORD0 : TEXCOORD1;
float4 vs_TEXCOORD1 : TEXCOORD2;
float4 vs_TEXCOORD2 : TEXCOORD3;
float3 vs_TEXCOORD3 : TEXCOORD4; };
struct RMColor { float4 SV_Target0 : SV_Target0; };
RMVaryings RMVertex(RMInput v) { RMVaryings o = (RMVaryings)0;
















float4 u_xlat0;
float4 u_xlat1;
float4 u_xlat2;
float2 u_xlat8;
float u_xlat11;


    u_xlat0 = v.in_POSITION0.yyyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1];
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0] * v.in_POSITION0.xxxx + u_xlat0;
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2] * v.in_POSITION0.zzzz + u_xlat0;
    u_xlat1 = u_xlat0 + _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3];
    u_xlat0 = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[3] * v.in_POSITION0.wwww + u_xlat0;
    u_xlat2 = u_xlat1.yyyy * _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[1];
    u_xlat2 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[0] * u_xlat1.xxxx + u_xlat2;
    u_xlat2 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[2] * u_xlat1.zzzz + u_xlat2;
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4unity_MatrixVP[3] * u_xlat1.wwww + u_xlat2;
    o.position = u_xlat1;
    u_xlat2.xyz = v.in_NORMAL0.yyy * _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[1].xyz;
    u_xlat2.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[0].xyz * v.in_NORMAL0.xxx + u_xlat2.xyz;
    u_xlat2.xyz = _RMCaptured_hlslcc_mtx4x4unity_ObjectToWorld[2].xyz * v.in_NORMAL0.zzz + u_xlat2.xyz;
    u_xlat11 = dot(u_xlat2.xyz, u_xlat2.xyz);
    u_xlat11 = rsqrt(u_xlat11);
    o.vs_NORMAL0.xyz = ((float3)(u_xlat11)) * u_xlat2.xyz;
    u_xlat2.xy = _RMCaptured_Time.xx * _ReflectMaskTime.xy;
    u_xlat2.xy = frac(u_xlat2.xy);
    u_xlat8.xy = v.in_TEXCOORD0.xy * _ReflectMask_ST.xy + _ReflectMask_ST.zw;
    o.vs_TEXCOORD0.zw = u_xlat2.xy + u_xlat8.xy;
    o.vs_TEXCOORD0.xy = v.in_TEXCOORD0.xy * _MainTex_ST.xy + _MainTex_ST.zw;
    u_xlat1.y = u_xlat1.y * _RMCaptured_ProjectionParams.x;
    u_xlat2.xzw = u_xlat1.xwy * float3(0.5, 0.5, 0.5);
    o.vs_TEXCOORD1.zw = u_xlat1.zw;
    o.vs_TEXCOORD1.xy = u_xlat2.zz + u_xlat2.xw;
    u_xlat1 = u_xlat0.yyyy * _RMCaptured_hlslcc_mtx4x4_SGameShadowMatrix[1];
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4_SGameShadowMatrix[0] * u_xlat0.xxxx + u_xlat1;
    u_xlat1 = _RMCaptured_hlslcc_mtx4x4_SGameShadowMatrix[2] * u_xlat0.zzzz + u_xlat1;
    o.vs_TEXCOORD2 = _RMCaptured_hlslcc_mtx4x4_SGameShadowMatrix[3] * u_xlat0.wwww + u_xlat1;
    o.vs_TEXCOORD3.xyz = u_xlat0.xyz;
    o.position = UnityObjectToClipPos(v.in_POSITION0); return o;

}
RMColor RMFragment(RMVaryings v) { RMColor o = (RMColor)0;























float3 u_xlat0;
float4 u_xlat16_0;
float4 u_xlat1;
float4 u_xlat16_1;
float3 u_xlat16_2;
float2 u_xlat3;
bool2 u_xlatb3;
float4 u_xlat4;
float4 u_xlat5;
float3 u_xlat16_6;
float3 u_xlat16_7;
float2 u_xlat8;
float2 u_xlat16_8;
float u_xlat9;
float u_xlat16_9;
float3 u_xlat11;
bool u_xlatb11;
float u_xlat16;
float u_xlat16_16;
float2 u_xlat17;
float u_xlat16_18;
float2 u_xlat19;
float u_xlat24;
bool u_xlatb25;
float u_xlat16_26;
float u_xlat16_28;


    u_xlat0.xyz = (-v.vs_TEXCOORD3.xyz) + _RMCaptured_WorldSpaceCameraPos.xyz;
    u_xlat24 = dot(u_xlat0.xyz, u_xlat0.xyz);
    u_xlat1.x = rsqrt(u_xlat24);
    u_xlat0.xyz = u_xlat0.xyz * u_xlat1.xxx;
    u_xlat0.x = dot(v.vs_NORMAL0.xyz, u_xlat0.xyz);
    u_xlat0.x = clamp(u_xlat0.x, 0.0, 1.0);
    u_xlat0.x = (-u_xlat0.x) + _Metalness;
    u_xlat16_8.xy = _ReflectMask.Sample(sampler_ReflectMask, v.vs_TEXCOORD0.zw).xy;
    u_xlat16_2.xy = u_xlat16_8.xy * float2(2.0, 2.0) + float2(-1.0, -1.0);
    u_xlat1.xy = u_xlat16_2.xy * _ShadowTwist.xy + v.vs_TEXCOORD2.xy;
    u_xlat1.zw = v.vs_TEXCOORD2.zw;
    u_xlat1.xyz = u_xlat1.xyz / u_xlat1.www;
    u_xlat8.xy = u_xlat1.xy * float2(0.5, 0.5) + float2(0.5, 0.5);
    u_xlatb3.xy = ((abs(u_xlat8.xyxx)) < ( float4(0.9921875, 0.9921875, 0.0, 0.0))).xy;
    u_xlatb25 = u_xlatb3.y && u_xlatb3.x;
    if(u_xlatb25){
        u_xlat1.xy = abs(u_xlat1.xy) + float2(-0.980000019, -0.980000019);
        u_xlat1.xy = clamp(u_xlat1.xy, 0.0, 1.0);
        u_xlat1.x = u_xlat1.y + u_xlat1.x;
        u_xlat1.x = u_xlat1.x * 1000.0;
        u_xlat9 = u_xlat1.z * 0.5 + 0.499749988;
        u_xlat9 = min(u_xlat9, 0.999899983);
        u_xlat8.xy = u_xlat8.xy * g_CSMShadowTexture_TexelSize.zw + float2(0.5, 0.5);
        u_xlat17.xy = floor(u_xlat8.xy);
        u_xlat8.xy = u_xlat8.xy + (-u_xlat17.xy);
        u_xlat17.xy = u_xlat17.xy + float2(-0.5, -0.5);
        u_xlat17.xy = u_xlat17.xy * g_CSMShadowTexture_TexelSize.xy;
        u_xlat3.xy = (-u_xlat8.yx) * float2(2.0, 2.0) + float2(3.0, 3.0);
        u_xlat19.xy = u_xlat8.xy * float2(2.0, 2.0) + float2(1.0, 1.0);
        u_xlat4.xy = (-u_xlat8.xy) + float2(2.0, 2.0);
        u_xlat4.xy = u_xlat4.xy / u_xlat3.yx;
        u_xlat8.xy = u_xlat8.xy / u_xlat19.xy;
        u_xlat4.zw = u_xlat4.xy + float2(-1.0, -1.0);
        u_xlat4.xy = u_xlat8.xy + float2(1.0, 1.0);
        u_xlat8.x = u_xlat3.x * u_xlat3.y;
        u_xlat5 = u_xlat4.zwxw * g_CSMShadowTexture_TexelSize.xyxy + u_xlat17.xyxy;
        float3 txVec0 = float3(u_xlat5.xy,u_xlat9);
        u_xlat16_16 = step((txVec0).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec0).xy, 0).r);
        u_xlat3.xy = u_xlat3.xy * u_xlat19.xy;
        float3 txVec1 = float3(u_xlat5.zw,u_xlat9);
        u_xlat16_28 = step((txVec1).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec1).xy, 0).r);
        u_xlat3.x = u_xlat3.x * u_xlat16_28;
        u_xlat8.x = u_xlat8.x * u_xlat16_16 + u_xlat3.x;
        u_xlat4 = u_xlat4.zyxy * g_CSMShadowTexture_TexelSize.xyxy + u_xlat17.xyxy;
        float3 txVec2 = float3(u_xlat4.xy,u_xlat9);
        u_xlat16_16 = step((txVec2).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec2).xy, 0).r);
        u_xlat8.x = u_xlat3.y * u_xlat16_16 + u_xlat8.x;
        u_xlat16 = u_xlat19.y * u_xlat19.x;
        float3 txVec3 = float3(u_xlat4.zw,u_xlat9);
        u_xlat16_9 = step((txVec3).z, hlslcc_zcmpg_CSMShadowTexture.SampleLevel(samplerhlslcc_zcmpg_CSMShadowTexture, (txVec3).xy, 0).r);
        u_xlat8.x = u_xlat16 * u_xlat16_9 + u_xlat8.x;
        u_xlat8.x = u_xlat8.x * 0.0625 + u_xlat1.x;
        u_xlat8.x = clamp(u_xlat8.x, 0.0, 1.0);
        u_xlat16_8.x = u_xlat8.x;
    } else {
        u_xlat16_8.x = 1.0;
    }
    u_xlat1.xy = v.vs_TEXCOORD1.xy / v.vs_TEXCOORD1.ww;
    u_xlat17.xy = u_xlat16_2.xy * _UVXX.xy;
    u_xlat1.xy = u_xlat17.xy * float2(float2(_BaseTwist, _BaseTwist)) + u_xlat1.xy;
    u_xlat1.xy = u_xlat1.xy * _UVXX.zw;
    u_xlat16_1 = _ReflectionTex.Sample(sampler_ReflectionTex, u_xlat1.xy);
    u_xlat16_1 = u_xlat16_1 * _ReflectColor;
    u_xlat16_6.xyz = u_xlat16_1.xyz * u_xlat16_1.xyz;
    u_xlat16_6.xyz = u_xlat16_1.www * u_xlat16_6.xyz;
    u_xlat3.x = u_xlat0.x + 1.0;
    u_xlat3.x = clamp(u_xlat3.x, 0.0, 1.0);
    u_xlat3.x = u_xlat16_1.w * u_xlat3.x;
    u_xlat16_7.xyz = u_xlat16_8.xxx + _ShadowColor.xyz;
    u_xlat16_7.xyz = clamp(u_xlat16_7.xyz, 0.0, 1.0);
    u_xlat16_18 = (-u_xlat16_8.x) + 1.0;
    u_xlat16_18 = u_xlat16_18 * _ShadowColor.w;
    u_xlat11.x = sqrt(u_xlat24);
    u_xlat11.x = u_xlat11.x + (-_ShadowDistance1);
    u_xlat16_26 = max(_ShadowDistance2, 0.100000001);
    u_xlat11.x = u_xlat11.x / u_xlat16_26;
    u_xlat11.x = (-u_xlat11.x) + 1.0;
    u_xlat11.x = clamp(u_xlat11.x, 0.0, 1.0);
    u_xlat11.x = u_xlat16_18 * u_xlat11.x;
    u_xlat19.xy = u_xlat16_2.xy * _UVXX.xy + v.vs_TEXCOORD0.xy;
    u_xlat16_0 = _MainTex.Sample(sampler_MainTex, u_xlat19.xy);
    u_xlat3.x = max(u_xlat11.x, u_xlat3.x);
    u_xlat1 = u_xlat16_0 * _BaseColor;
    u_xlat16_2.xyz = u_xlat1.xyz * u_xlat1.xyz + u_xlat16_6.xyz;
    u_xlat4.xyz = u_xlat16_7.xyz * u_xlat16_7.xyz + float3(-1.0, -1.0, -1.0);
    u_xlat11.xyz = u_xlat11.xxx * u_xlat4.xyz + float3(1.0, 1.0, 1.0);
    u_xlat11.xyz = u_xlat16_2.xyz * u_xlat11.xyz;
    u_xlat3.x = u_xlat3.x * u_xlat16_0.w + u_xlat1.w;
    u_xlat3.x = clamp(u_xlat3.x, 0.0, 1.0);
    u_xlat16_2.xyz = u_xlat11.yyy * _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[1].xyz;
    u_xlat16_2.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[0].xyz * u_xlat11.xxx + u_xlat16_2.xyz;
    u_xlat16_2.xyz = _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[2].xyz * u_xlat11.zzz + u_xlat16_2.xyz;
    u_xlat16_2.xyz = u_xlat16_2.xyz + _RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].xyz;
    u_xlatb11 = 0.0>=_RMCaptured_hlslcc_mtx4x4g_ColorMatrix[3].w;
    u_xlat16_6.xyz = u_xlat16_2.xyz + float3(0.155000001, 0.155000001, 0.155000001);
    u_xlat16_6.xyz = u_xlat16_2.xyz / u_xlat16_6.xyz;
    u_xlat16_6.xyz = u_xlat16_6.xyz * float3(1.01900005, 1.01900005, 1.01900005);
    u_xlat16_2.xyz = min(u_xlat16_2.xyz, float3(65504.0, 65504.0, 65504.0));
    o.SV_Target0.xyz = (bool(u_xlatb11)) ? u_xlat16_6.xyz : u_xlat16_2.xyz;
    o.SV_Target0.w = u_xlat3.x;
    return o;

}
ENDHLSL
} } }
