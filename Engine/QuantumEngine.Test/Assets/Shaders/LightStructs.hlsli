struct Attenuation
{
    float c0;
    float c1;
    float c2;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float ambient;
    float diffuse;
    float specular;
    float2 dummy;
};

struct PointLight
{
    float4 color;
    float3 position;
    float ambient;
    Attenuation attenuation;
    float diffuse;
    float specular;
    float radius;
    float2 dummy;
    
    float AttenuationFactor(float distance)
    {
        return 1.0f / (attenuation.c0 + attenuation.c1 * distance + attenuation.c2 * distance * distance);
    }
};

struct LightData
{
    DirectionalLight directionalLights[10];
    PointLight pointLights[10];
    uint directionalLightCount;
    uint pointLightCount;
};

float3 PhongDirectionalLight(DirectionalLight light, float3 camPosition, float3 position, float3 normal)
{
    float3 norm = normalize(normal);
    float3 lightDir = normalize(light.direction);

    float diff = max(dot(norm, -lightDir), 0.0);
    float diffuse = diff * light.diffuse;

    float3 viewDir = normalize(camPosition - position);
    float3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    float specular = spec * light.specular;

    return (light.ambient + specular + diffuse) * light.color.xyz;
}

float3 PhongPointLight(PointLight light, float3 camPosition, float3 position, float3 normal)
{
    float3 norm = normalize(normal);
    float3 lightDir = -position + light.position;
    float sqrlightMag = dot(lightDir, lightDir);

    if (sqrlightMag > pow(light.radius, 2))
    { //light is too far away from pixel
        return float3(0.0f, 0.0f, 0.0f);
    }
	
    float lightMag = sqrt(sqrlightMag);
    lightDir = normalize(lightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    float diffuse = diff * light.diffuse;

    float3 viewDir = normalize(camPosition - position);
    float3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 2);
    float specular = spec * light.specular;
    
    float att = light.AttenuationFactor(lightMag);
    return att * (light.ambient + diff + specular) * light.color.xyz;
}
