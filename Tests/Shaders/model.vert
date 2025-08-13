#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout(set = 0, binding = 0) uniform CameraData
{
    mat4 view;
    mat4 proj;
} cameraData;

layout(push_constant) uniform ModelData
{
    mat4 model;
} modelData;

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec3 FragPos;
layout(location = 2) out vec3 WorldNormal;
layout(location = 3) out mat3 TBN;


void main()
{
    TexCoord = aTexCoord;
    gl_Position = cameraData.proj * cameraData.view * modelData.model * vec4(aPos, 1.0);
    FragPos = vec3(modelData.model * vec4(aPos, 1.0));
    mat3 normalMatrix = transpose(inverse(mat3(modelData.model)));
    vec3 T = normalize(vec3(modelData.model * vec4(aTangent, 0.0)));
    vec3 B = normalize(vec3(modelData.model * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(modelData.model * vec4(aNormal, 0.0)));
    TBN = mat3(T,B,N);
    WorldNormal = N;
}