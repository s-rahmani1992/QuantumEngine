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
    float intensity;
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    Attenuation attenuation;
    float radius;
    
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

inline float3 PhongDirectionalLight(DirectionalLight light, float3 camPosition, float3 position, float3 normal, float3 ads)
{
    float3 norm = normalize(normal);
    float3 lightDir = normalize(light.direction);

    float diff = max(dot(norm, -lightDir), 0.0);
    float diffuse = diff * ads.y;

    float specular = 0;
    
    if (diff > 0.0f)
    {
        float3 viewDir = normalize(camPosition - position);
        float3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        specular = spec * ads.z;
    }
    

    return light.intensity * (ads.x + specular + diffuse) * light.color.xyz;
}

inline float3 PhongPointLight(PointLight light, float3 camPosition, float3 position, float3 normal, float3 ads)
{
    float3 norm = normalize(normal);
    float3 lightDir = -position + light.position;
    float sqrlightMag = dot(lightDir, lightDir);

    if (sqrlightMag > pow(light.radius, 2))
    { //light is too far away from pixel
        return light.intensity * ads.x * light.color.xyz;
    }
	
    float lightMag = sqrt(sqrlightMag);
    lightDir = normalize(lightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    float diffuse = diff * ads.y;

    float specular = 0;
    
    if (diff > 0.0f)
    {
        float3 viewDir = normalize(camPosition - position);
        float3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        specular = spec * ads.z;
    }
    
    float att = light.AttenuationFactor(lightMag);
    return light.intensity * (ads.x + specular + diffuse) * light.color.xyz;
}

inline float3 PhongLight(LightData lightData, float3 camPosition, float3 position, float3 normal, float3 ads)
{
    float3 lightFactor = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < lightData.directionalLightCount; i++)
        lightFactor += PhongDirectionalLight(lightData.directionalLights[i], camPosition, position, normal, ads);
    for (uint i = 0; i < lightData.pointLightCount; i++)
        lightFactor += PhongPointLight(lightData.pointLights[i], camPosition, position, normal, ads);
    
    return lightFactor;
}