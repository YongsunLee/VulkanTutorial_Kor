#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec3 a_Color;

layout(location = 0) out vec3 fragColor;

void main(){
	gl_Position = vec4(a_Position, 0.0f, 1.0f);
	fragColor = a_Color;
}