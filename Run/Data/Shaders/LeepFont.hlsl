Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
};

cbuffer PerFrameConstants : register(b1)
{
    float time;
    int DebugInt;
    float DebugFloat;
    float padding;
};

struct vs_input_t
{
    float3 modelSpacePosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
#ifdef USE_FONT_VERTEX
    float2 glyphPos : TEXCOORD1;
    float2 textPos  : TEXCOORD2;
    float charIndex : TEXCOORD3;
#endif
};

struct v2p_t
{
    float4 clipSpacePosition : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
#ifdef USE_FONT_VERTEX
    float2 glyphPos : TEXCOORD1;
    float2 textPos  : TEXCOORD2;
    float charIndex : TEXCOORD3;
#endif
};

v2p_t VertexMain(vs_input_t input)
{
    float4 modelSpacePosition = float4(input.modelSpacePosition, 1);
    float4 worldSpacePosition = mul(ModelToWorldTransform, modelSpacePosition);
    float4 cameraSpacePosition = mul(WorldToCameraTransform, worldSpacePosition);
    float4 renderSpacePosition = mul(CameraToRenderTransform, cameraSpacePosition);
    float4 clipSpacePosition = mul(RenderToClipTransform, renderSpacePosition);

    v2p_t v2p;
    v2p.clipSpacePosition = clipSpacePosition;
    v2p.color = input.color;
    v2p.uv = input.uv;
#ifdef USE_FONT_VERTEX
    v2p.glyphPos = input.glyphPos;
    v2p.textPos  = input.textPos;
    v2p.charIndex = input.charIndex;
#endif
    return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    float dist = diffuseTexture.Sample(diffuseSampler, input.uv).r;
    float width = fwidth(dist);
    float glyphAlpha = smoothstep(0.5 - width, 0.5 + width, dist);

#ifdef USE_FONT_VERTEX

    float2 uv = input.uv;

    float glyph = glyphAlpha;

    float edge = smoothstep(0.45 - width, 0.45 + width, dist) - glyph;
    float glow = smoothstep(0.25, 0.5, dist);

    float flow  = sin(uv.y * 6.0 + time * 0.1) * 0.5 + 0.5;
    float drift = sin(uv.x * 4.0 + time * 0.02) * 0.5 + 0.5;

    float noise = frac(sin(dot(uv * 25.0, float2(12.9898,78.233))) * 43758.5453);
    float flicker = lerp(0.9, 1.1, noise);

    float scan = frac(time * 0.04 + uv.x * 1.2);
    float band = smoothstep(0.45, 0.5, scan) - smoothstep(0.5, 0.55, scan);

    float3 sandBase   = float3(0.45, 0.32, 0.18);
    float3 sandBright = float3(0.75, 0.55, 0.30);
    float3 heatColor  = float3(0.9, 0.5, 0.2);

    float3 coreColor = lerp(sandBase, sandBright, flow * 0.5);
    float3 edgeColor = sandBright * 1.5;
    float3 glowColor = heatColor * 1.5 * flicker;
    float3 scanColor = sandBright * 1.8 * band;

    float3 finalColor =
        coreColor * glyph * 0.7 +
        edgeColor * edge * 1.8 +
        glowColor * glow * (0.6 + drift * 0.4) +
        scanColor * glyph;

    finalColor *= 0.85;

    float finalAlpha =
        (glyph + edge * 0.8 + glow * 0.4) *
        input.color.a * ModelColor.a;

    return float4(finalColor, finalAlpha);

#else

    float alpha = glyphAlpha * input.color.a * ModelColor.a;
    float3 color = input.color.rgb * ModelColor.rgb;
    return float4(color, alpha);

#endif
}