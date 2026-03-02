cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
    float3   CameraPosition;
    float    _pad0;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4   ModelColor;
};

cbuffer ParticleSystemConstants : register(b8)
{
    float4x4 ParticleTransform;
    float    DeltaTime;
    float    SystemTime;
    int      MainBillboardType;
    int      SubstageBillboardType;
    float    _pad1[2];
};

#define MAX_LIGHTS 8

struct Light
{
    float4 c_color;
    float3 c_worldPosition;
    float  _pad0;
    float3 c_spotForward;
    float  c_ambience;
    float  c_innerRadius;
    float  c_outerRadius;
    float  c_innerDotThreshold;
    float  c_outerDotThreshold;
};

cbuffer LightConstants : register(b4)
{
    float4 c_sunColor;
    float3 c_sunNormal;
    int    c_numLights;
    Light  c_lightsArray[MAX_LIGHTS];
};

Texture2D<float> g_SceneDepth : register(t5);
SamplerState     g_SoftSamp   : register(s5);

cbuffer SoftParams : register(b9)
{
    float nearZ;
    float farZ;
    float softRange;
    float softBias;
};




static float RangeMap(float v, float a, float b, float c, float d)
{
    return c + ((v - a) / max(b - a, 1e-6)) * (d - c);
}

static float SmoothStep3(float x)
{
    return (3.0 * (x * x)) - (2.0 * x) * (x * x);
}

static void EvaluateLighting(
    float3 worldPos,
    float3 normalWS,
    float3 camPosWS,
    float glossiness,
    float specularity,
    out float3 outDiffuse,
    out float3 outSpecular)
{
    float3 totalDiffuseLight = float3(0,0,0);
    float3 totalSpecularLight = float3(0,0,0);
    float specularExponent = RangeMap(glossiness, 0, 1, 1, 32);
    float3 pixelToCameraDir = normalize(camPosWS - worldPos);

    float sunlightStrength = c_sunColor.a * saturate(dot(-c_sunNormal, normalWS));

    float3 diffuseLightFromSun = sunlightStrength * c_sunColor.rgb;
    totalDiffuseLight += diffuseLightFromSun;

    float3 pixelToSunDir = -c_sunNormal;
    float3 sunIdealReflectionDir = normalize(pixelToSunDir + pixelToCameraDir);
    float sunSpecularDot = saturate(dot(sunIdealReflectionDir, normalWS));
    float sunSpecularStrength = glossiness * c_sunColor.a * pow(sunSpecularDot, specularExponent);
    float3 sunSpecularLight = sunSpecularStrength * c_sunColor.rgb;
    totalSpecularLight += sunSpecularLight;

    for (int lightIndex = 0; lightIndex < c_numLights; ++lightIndex)
    {
        float ambience = c_lightsArray[lightIndex].c_ambience;
        float3 lightPos = c_lightsArray[lightIndex].c_worldPosition;
        float3 lightColor = c_lightsArray[lightIndex].c_color.rgb;
        float lightBrightness = c_lightsArray[lightIndex].c_color.a;
        float innerRadius = c_lightsArray[lightIndex].c_innerRadius;
        float outerRadius = c_lightsArray[lightIndex].c_outerRadius;
        float innerPenumbraDot = c_lightsArray[lightIndex].c_innerDotThreshold;
        float outerPenumbraDot = c_lightsArray[lightIndex].c_outerDotThreshold;

        float3 pixelToLightDisp = lightPos - worldPos;
        float3 pixelToLightDir = normalize(pixelToLightDisp);
        float3 lightToPixelDir = -pixelToLightDir;
        float distToLight = length(pixelToLightDisp);

        float falloff = saturate(RangeMap(distToLight, innerRadius, outerRadius, 1, 0));
        falloff = SmoothStep3(falloff);

        float penumbra = 1;
        if (length(c_lightsArray[lightIndex].c_spotForward) > 0.1f)
        {
            float3 spotDir = normalize(c_lightsArray[lightIndex].c_spotForward);
            float cosAngle = dot(spotDir, lightToPixelDir);
            penumbra = saturate(RangeMap(cosAngle, outerPenumbraDot, innerPenumbraDot, 0, 1));
            penumbra = SmoothStep3(penumbra);
        }

        float lightStrength = penumbra * falloff * lightBrightness *
            saturate(RangeMap(dot(pixelToLightDir, normalWS), -ambience, 1, 0, 1));

        float3 diffuseLight = lightStrength * lightColor;
        totalDiffuseLight += diffuseLight;

        float3 idealReflectionDir = normalize(pixelToCameraDir + pixelToLightDir);
        float specularDot = saturate(dot(idealReflectionDir, normalWS));
        float specularStrength = glossiness * lightBrightness * pow(specularDot, specularExponent);
        specularStrength *= falloff * penumbra;
        float3 specularLight = specularStrength * lightColor;
        totalSpecularLight += specularLight;
    }

    outDiffuse = saturate(totalDiffuseLight);
    outSpecular = totalSpecularLight * specularity;
}

struct Particle
{
    float3 Position;
    float  Lifetime;
    float3 Velocity;
    float  MaxLifetime;
    float4 Color;
    float  Size;
    float  SoftFactor;
    float  Emissive;
    float  Stage;
    float  Orientation;
    float  AngularVelocity;
    float  p_pad0;
	float  p_pad1;
};

StructuredBuffer<Particle> Particles : register(t0);
Texture2D ParticleTexture0 : register(t1);
Texture2D ParticleTexture1 : register(t6);

static const float2 kQuadCorners[4] = {
    float2(-0.5, -0.5),
    float2( 0.5, -0.5),
    float2(-0.5,  0.5),
    float2( 0.5,  0.5)
};

static const uint kQuadIndices[6] = {0, 2, 1, 2, 3, 1};

static const float2 kQuadUVs[4] = {
    float2(0, 0),
    float2(1, 0),
    float2(0, 1),
    float2(1, 1)
};

struct MeshVertexInput
{
    float3 Position    : POSITION;
    float4 Color       : COLOR;
    float2 TexCoords   : TEXCOORD;
    float3 Tangent     : TANGENT;
    float3 Bitangent   : BITANGENT;
    float3 Normal      : NORMAL;
};

Texture2D DiffuseTexture : register(t2);
Texture2D NormalTexture : register(t3);
Texture2D SpecGlossEmitTexture : register(t4);

struct ParticleVertexInput
{
    uint VertexID   : SV_VertexID;
    uint InstanceID : SV_InstanceID;
};

struct UniversalVSOutput
{
    float4 Position    : SV_Position;
    float4 Color       : COLOR;
    float2 UV          : TEXCOORD0;
    float3 WorldPos    : TEXCOORD1;
    float3 WorldNormal : TEXCOORD2;
    float3 WorldTangent : TEXCOORD3;
    float3 WorldBitangent : TEXCOORD4;
    float  Emissive    : TEXCOORD5;
    float  UseLight    : TEXCOORD6;
    float  Gloss       : TEXCOORD7;
    float  IsParticle  : TEXCOORD8;
    float  StageIndex  : TEXCOORD9;
    float  BillboardType : TEXCOORD10;
};

UniversalVSOutput MeshVertexMain(MeshVertexInput input)
{
    UniversalVSOutput output;

    output.IsParticle = 0.0f;
    output.Emissive = 0.0f;
    output.UseLight = 1.0f;
    output.Gloss = 0.5f;
    output.StageIndex = 0.0f;
    output.BillboardType = 0.0f;

    float4 modelPos = float4(input.Position, 1.0);
    float4 worldPos = mul(ModelToWorldTransform, modelPos);
    float4 cameraPos = mul(WorldToCameraTransform, worldPos);
    float4 renderPos = mul(CameraToRenderTransform, cameraPos);
    float4 clipPos = mul(RenderToClipTransform, renderPos);

    output.Position = clipPos;
    output.Color = input.Color * ModelColor;
    output.UV = input.TexCoords;
    output.WorldPos = worldPos.xyz;

    float4 worldNormal = mul(ModelToWorldTransform, float4(input.Normal, 0.0));
    float4 worldTangent = mul(ModelToWorldTransform, float4(input.Tangent, 0.0));
    float4 worldBitangent = mul(ModelToWorldTransform, float4(input.Bitangent, 0.0));

    output.WorldNormal = normalize(worldNormal.xyz);
    output.WorldTangent = normalize(worldTangent.xyz);
    output.WorldBitangent = normalize(worldBitangent.xyz);

    return output;
}

UniversalVSOutput ParticleVertexMain(ParticleVertexInput input)
{
    UniversalVSOutput output;

    output.IsParticle = 1.0f;

    Particle p = Particles[input.InstanceID];

    if (p.Lifetime <= 0.0f && p.Lifetime > -0.5f || p.Size <= 0.0f)
    {
        output.Position = float4(0,0,0,1);
        output.Color = float4(0,0,0,0);
        output.UV = 0;
        output.WorldPos = 0;
        output.WorldNormal = float3(0,0,1);
        output.WorldTangent = float3(1,0,0);
        output.WorldBitangent = float3(0,1,0);
        output.Emissive = 0;
        output.UseLight = 0;
        output.Gloss = 0;
        output.StageIndex = 0;
        output.BillboardType = 0;
        return output;
    }

    uint lid = input.VertexID % 6;
    uint cornerIndex = kQuadIndices[lid];
    
    // rotate the corner in 2D using particle orientation (radians)
    float s = sin(p.Orientation);
    float c = cos(p.Orientation);
    
    float2 corner = kQuadCorners[cornerIndex]; // (-0.5..0.5)
    float2 rotCorner = float2(
        corner.x * c - corner.y * s,
        corner.x * s + corner.y * c
    );
    
    float2 quad2 = rotCorner * p.Size;


    uint stage = (uint)round(saturate(p.Stage));
    int billboardType = (stage == 0u) ? MainBillboardType : SubstageBillboardType;
    
    float3 worldForward = float3(1, 0, 0);
    float3 worldLeft = float3(0, 1, 0);   
    float3 worldUp = float3(0, 0, 1);
    float3 toCamera = normalize(CameraPosition - p.Position);

    float3 left, up, worldBillboardPos, particleNormal;
    
    if (billboardType == 1)
    {
        float3 right = worldForward; 
        float3 forward = worldLeft;  
        up = worldUp; 

        worldBillboardPos = p.Position + right * quad2.x + forward * quad2.y;
        
        particleNormal = worldUp; 
        
        left = right;
    }
    else
    {
        if (abs(dot(toCamera, worldUp)) > 0.99f)
        {
            float3 altUp = float3(1,0,0);
            left = normalize(cross(altUp, toCamera));
            up = normalize(cross(toCamera, left));
        }
        else
        {
            left = normalize(cross(worldUp, toCamera));
            up = normalize(cross(toCamera, left));
        }

        worldBillboardPos = p.Position + (-left) * quad2.x + up * quad2.y;
        particleNormal = normalize(-toCamera);
    }

    float4 worldPos = float4(worldBillboardPos, 1.0f);
    float4 cameraPos = mul(WorldToCameraTransform, worldPos);
    float4 renderPos = mul(CameraToRenderTransform, cameraPos);
    float4 clipPos = mul(RenderToClipTransform, renderPos);

    output.Position = clipPos;
    output.Color = p.Color * ModelColor;
    output.UV = kQuadUVs[cornerIndex];
    output.WorldPos = worldBillboardPos;

    output.WorldNormal = particleNormal;
    output.WorldTangent = left;
    output.WorldBitangent = up;

    output.Emissive = p.Emissive;
    output.UseLight = 0.0f;
    output.Gloss = saturate(p.SoftFactor);

    output.StageIndex = saturate(p.Stage);
    output.BillboardType = float(billboardType);
    
    return output;
}

SamplerState MainSampler : register(s0);

float3 DecodeRGBToXYZ(float3 color)
{
    return (color * 2.0) - 1.0;
}

float4 PixelMain(UniversalVSOutput input) : SV_Target
{
    if (input.IsParticle > 0.5f)
    {
        uint stage = (uint)round(saturate(input.StageIndex));

        float4 tex0 = ParticleTexture0.Sample(MainSampler, input.UV);
        float4 tex1 = ParticleTexture1.Sample(MainSampler, input.UV);
        float4 tex = (stage == 0u) ? tex0 : tex1;

        float4 baseColor = tex * input.Color;

        if (baseColor.a <= 0.001f) discard;

        uint billboardType = (uint)input.BillboardType;
        
        float3 particleNormal;
        if (billboardType == 1)
        {
            particleNormal = float3(0, 0, 1);
        }
        else
        {
            particleNormal = normalize(-normalize(CameraPosition - input.WorldPos));
        }

        float specularity = 0.3f;
        float glossiness = 0.4f;
        float emissiveness = input.Emissive;

        float3 finalColor = baseColor.rgb;


        finalColor += baseColor.rgb * emissiveness;
        finalColor = min(finalColor, float3(1.0, 1.0, 1.0));

        return float4(finalColor, baseColor.a);
    }
    else
    {
        float4 diffuseTexel = DiffuseTexture.Sample(MainSampler, input.UV);
        float4 baseColor = diffuseTexel * input.Color;

        if (baseColor.a <= 0.001f) discard;

        float4 normalTexel = NormalTexture.Sample(MainSampler, input.UV);
        float4 specGlossEmitTexel = SpecGlossEmitTexture.Sample(MainSampler, input.UV);

        float3 pixelNormalTBNSpace = normalize(DecodeRGBToXYZ(normalTexel.rgb));
        float3x3 tbnToWorld = float3x3(
            normalize(input.WorldTangent),
            normalize(input.WorldBitangent),
            normalize(input.WorldNormal)
        );
        float3 normal = mul(pixelNormalTBNSpace, tbnToWorld);

        float specularity = specGlossEmitTexel.r;
        float glossiness = specGlossEmitTexel.g;
        float emissiveness = specGlossEmitTexel.b;

        float3 diffuseLight, specularLight;
        EvaluateLighting(input.WorldPos, normal, CameraPosition, glossiness, specularity,
                         diffuseLight, specularLight);

        float3 finalColor = baseColor.rgb;

        if (input.UseLight > 0.5f)
        {
            finalColor = finalColor * diffuseLight + specularLight;
        }

        finalColor += baseColor.rgb * emissiveness;

        return float4(finalColor, baseColor.a);
    }
}