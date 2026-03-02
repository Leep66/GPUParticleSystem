#ifndef COMMON_LIGHTING_INCLUDED
#define COMMON_LIGHTING_INCLUDED

#define MAX_LIGHTS 8

struct Light 
{
    float4  c_color;
    float3  c_worldPosition; float _pad0;
    float3  c_spotForward;  float c_ambience;
    float   c_innerRadius;
    float   c_outerRadius;
    float   c_innerDotThreshold;
    float   c_outerDotThreshold;
};

cbuffer LightConstants : register(b4)
{
    float4  c_sunColor;
    float3  c_sunNormal; int c_numLights;
    Light   c_lightsArray[MAX_LIGHTS];
}

static float RangeMap(float v,float a,float b,float c,float d){ return c + ((v-a)/(b-a))*(d-c); }
static float SmoothStep3(float x){ return (3.0*(x*x)) - (2.0*x)*(x*x); }

static void EvaluateLightingLambertBlinnPhong(
    float3 worldPos,
    float3 normalWS,
    float3 camPosWS,
    float  glossiness,   // [0,1]
    out float3 outDiffuse,
    out float3 outSpecular)
{
    float3 totalDiffuse  = 0;
    float3 totalSpecular = 0;
    float  specExp = RangeMap(glossiness, 0.f, 1.f, 1.f, 32.f);
    float3 V = normalize(camPosWS - worldPos);

    // Sun
    float sunAmbience = 0.2f;
    float nDotL = dot(-c_sunNormal, normalWS);
    float sunStrength = c_sunColor.a * saturate(RangeMap(nDotL, -sunAmbience, 1.0, 0.0, 5.0));
    totalDiffuse += sunStrength * c_sunColor.rgb;

    float3 Ls = -c_sunNormal;
    float3 Hs = normalize(Ls + V);
    float sunSpec = saturate(dot(Hs, normalWS));
    totalSpecular += (glossiness * c_sunColor.a * pow(sunSpec, specExp)) * c_sunColor.rgb;

    // Points/Spots
    [loop]
    for (int i=0;i<c_numLights;++i)
    {
        Light L = c_lightsArray[i];
        float3 toL   = L.c_worldPosition - worldPos;
        float  dist  = length(toL);
        float3 Ldir  = (dist>1e-5)? toL/dist : 0;
        float3 lightToPix = -Ldir;

        float falloff = SmoothStep3( saturate(RangeMap(dist, L.c_innerRadius, L.c_outerRadius, 1.f, 0.f)) );
        float penumbra = SmoothStep3( saturate(RangeMap(dot(L.c_spotForward, lightToPix), L.c_outerDotThreshold, L.c_innerDotThreshold, 0.f, 1.f)) );

        float lambert = saturate(RangeMap(dot(Ldir, normalWS), -L.c_ambience, 1.0, 0.0, 1.0));
        float lightStrength = penumbra * falloff * L.c_color.a * lambert;

        totalDiffuse += lightStrength * L.c_color.rgb;

        float3 H = normalize(V + Ldir);
        float sdot = saturate(dot(H, normalWS));
        float s = glossiness * L.c_color.a * pow(sdot, specExp) * falloff * penumbra;
        totalSpecular += s * L.c_color.rgb;
    }

    outDiffuse  = totalDiffuse;
    outSpecular = totalSpecular;
}

#endif
