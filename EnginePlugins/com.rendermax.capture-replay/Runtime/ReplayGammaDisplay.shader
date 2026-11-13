Shader "Hidden/RenderMax/ReplayGammaDisplay"
{
    Properties { _MainTex ("Source", 2D) = "white" {} }
    SubShader
    {
        Cull Off ZWrite Off ZTest Always
        Pass
        {
            CGPROGRAM
            #pragma vertex vert_img
            #pragma fragment frag
            #include "UnityCG.cginc"
            sampler2D _MainTex;
            float _DisplayAsGamma, _LinearProject;
            float3 Decode(float3 c)
            {
                return float3(c.r<=0.04045 ? c.r/12.92 : pow((c.r+0.055)/1.055,2.4),
                              c.g<=0.04045 ? c.g/12.92 : pow((c.g+0.055)/1.055,2.4),
                              c.b<=0.04045 ? c.b/12.92 : pow((c.b+0.055)/1.055,2.4));
            }
            float3 Encode(float3 c)
            {
                return float3(c.r<=0.0031308 ? c.r*12.92 : 1.055*pow(max(c.r,0),1.0/2.4)-0.055,
                              c.g<=0.0031308 ? c.g*12.92 : 1.055*pow(max(c.g,0),1.0/2.4)-0.055,
                              c.b<=0.0031308 ? c.b*12.92 : 1.055*pow(max(c.b,0),1.0/2.4)-0.055);
            }
            float4 frag(v2f_img i) : SV_Target
            {
                float4 c=tex2D(_MainTex,i.uv);
                // RenderMax GL viewer decodes non-sRGB data when enabled;
                // its sRGB output encode cancels that decode. Account for Unity's
                // output encode in Linear, or do it explicitly in Gamma projects.
                if (_LinearProject>0.5) { if (_DisplayAsGamma>0.5) c.rgb=Decode(c.rgb); }
                else { if (_DisplayAsGamma<0.5) c.rgb=Encode(c.rgb); }
                return c;
            }
            ENDCG
        }
    }
}
