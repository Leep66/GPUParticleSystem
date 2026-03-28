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


struct vs_input_t
{
    float3 modelSpacePosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct v2p_t
{
    float4 clipSpacePosition : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
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
    return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    float dist = diffuseTexture.Sample(diffuseSampler, input.uv).r;

    float width = fwidth(dist);
    float glyphAlpha = smoothstep(0.5 - width, 0.5 + width, dist);
    float outlineAlpha = smoothstep(0.45 - width, 0.45 + width, dist);
    float3 baseColor = input.color.rgb * ModelColor.rgb;
    float3 finalColor = lerp(float3(0,0,0), baseColor, glyphAlpha);
    float finalAlpha = glyphAlpha * input.color.a * ModelColor.a;

    return float4(finalColor, finalAlpha);
}