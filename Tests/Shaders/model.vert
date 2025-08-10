#version 450

layout(location = 0) in vec3 aPos;

//Unused
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

void main()
{
    gl_Position = cameraData.proj * cameraData.view * modelData.model * vec4(aPos, 1.0);
}