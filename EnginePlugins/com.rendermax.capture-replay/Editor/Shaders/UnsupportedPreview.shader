Shader "RenderMax/Diagnostic/Untranslated"
{
    Properties { _Color("Diagnostic color (NOT captured shader)", Color) = (0.8,0.2,0.8,1) }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        Pass
        {
            Cull Off
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"
            float4 _Color;
            float4 vert(float4 position:POSITION):SV_POSITION { return UnityObjectToClipPos(position); }
            fixed4 frag():SV_Target { return _Color; }
            ENDCG
        }
    }
}
