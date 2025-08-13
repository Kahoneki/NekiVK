#version 450

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec3 FragPos;
layout(location = 2) in vec3 WorldNormal;
layout(location = 3) in mat3 TBN;

layout(set = 1, binding = 0) uniform sampler2DArray albedoSampler;
layout(set = 1, binding = 1) uniform sampler2DArray normalSampler;
layout(set = 1, binding = 3) uniform sampler2DArray metallicSampler;
layout(set = 1, binding = 4) uniform sampler2DArray roughnessSampler;
layout(set = 1, binding = 5) uniform sampler2DArray aoSampler;
layout(set = 1, binding = 6) uniform sampler2DArray emissiveSampler;

layout(location = 0) out vec4 outColour;

void main()
{
    //todo: implement cook-torrance pbr brdf with these - this will probably be done in VulkanPBR repo (https://github.com/Kahoneki/VulkanPBR)
    float metallic = texture(metallicSampler, vec3(TexCoord, 0)).g;
    float roughness = texture(roughnessSampler, vec3(TexCoord, 0)).b;
    
    vec3 lightPos = vec3(1,-1,1);
    vec3 lightColour = vec3(0.1,0.2,0.1);
    
    vec3 albedo = texture(albedoSampler, vec3(TexCoord, 0)).rgb;
    vec3 tangentNormal = texture(normalSampler, vec3(TexCoord, 0)).rgb * 2.0 - 1.0;
    vec3 worldNormal = normalize(TBN * tangentNormal);
    
    float ambientStrength = 0.01;
    vec3 ambient = (ambientStrength * lightColour) * texture(aoSampler, vec3(TexCoord, 0)).r;
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(worldNormal, lightDir), 0.0);
    vec3 diffuse = diff * lightColour;
    
    vec3 finalColour = ((ambient + diffuse) * albedo) + texture(emissiveSampler, vec3(TexCoord, 0)).rgb;
    
    outColour = vec4(finalColour, 1.0);
}