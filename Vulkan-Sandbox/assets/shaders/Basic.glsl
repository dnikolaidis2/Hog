#type vertex
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 u_ViewProjection;
};

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoords;
layout(location = 3) in vec3 a_Color;

layout(location = 0) out vec3 o_Color;

layout(push_constant) uniform PushConstants
{
    mat4 p_Model;
};

void main() {
    gl_Position = u_ViewProjection * p_Model * vec4(a_Position, 1.0);
    o_Color = a_Color;
}

#type fragment
#version 450

layout(location = 0) in vec3 v_Color;


layout(location = 0) out vec4 o_Color;

void main() {
    o_Color = vec4(v_Color, 1.0);
}