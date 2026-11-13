Shader "Hidden/RenderMax/CapturedAlbedo"
{
    Properties
    {
        _RMTexture ("Captured texture", 2D) = "white" {}
        _RMCull ("Cull", Float) = 2
        _RMZTest ("Depth test", Float) = 4
        _RMZWrite ("Depth write", Float) = 1
        _RMColorMask ("Color mask", Float) = 15
    }
    SubShader
    {
        Pass
        {
            Cull [_RMCull]
            ZTest [_RMZTest]
            ZWrite [_RMZWrite]
            ColorMask [_RMColorMask]
            Blend Off
            HLSLPROGRAM
            #pragma target 3.5
            #pragma vertex vert
            #pragma fragment frag
            #pragma multi_compile __ RENDERMAX_EDITABLE_SCENE
            #include "UnityCG.cginc"
            float4 _RMObject[4];
            float4 _RMVP[4];
            float4 _RMST;
            float _RMFlipY;
            sampler2D _RMTexture;
            struct Input { float4 position : POSITION; float2 uv : TEXCOORD0; };
            struct Varying { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
            Varying vert(Input v)
            {
                Varying o;
                #if defined(RENDERMAX_EDITABLE_SCENE)
                o.position = UnityObjectToClipPos(v.position);
                #else
                // Preserve the captured GLSL column-vector arithmetic order.
                float3 world = v.position.yyy * _RMObject[1].xyz;
                world = _RMObject[0].xyz * v.position.xxx + world;
                world = _RMObject[2].xyz * v.position.zzz + world;
                world = world + _RMObject[3].xyz;
                float4 clip = world.yyyy * _RMVP[1];
                clip = _RMVP[0] * world.xxxx + clip;
                clip = _RMVP[2] * world.zzzz + clip;
                clip = clip + _RMVP[3];
                // Captured GL clip depth is [-w,w]. D3D-style backends use [0,w].
                #if defined(SHADER_API_D3D11) || defined(SHADER_API_METAL) || defined(SHADER_API_VULKAN)
                    clip.z = (clip.z + clip.w) * 0.5;
                #endif
                o.position = clip;
                o.position.y *= _RMFlipY;
                #endif
                o.uv = v.uv;
                return o;
            }
            half4 frag(Varying i) : SV_Target
            {
                half2 uv = i.uv * _RMST.xy + _RMST.zw;
                return tex2D(_RMTexture, uv);
            }
            ENDHLSL
        }
    }
}
