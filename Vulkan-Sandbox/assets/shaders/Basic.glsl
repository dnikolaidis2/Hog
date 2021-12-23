#type vertex
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 u_ViewProjection;
};

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoords;
layout(location = 3) in vec3 a_Color;

layout(location = 0) out vec3 o_Color;
layout(location = 1) out vec2 o_TexCoord;

layout(push_constant) uniform PushConstants
{
    mat4 p_Model;
};

void main() {
    gl_Position = u_ViewProjection * p_Model * vec4(a_Position, 1.0);
    o_TexCoord = a_TexCoords;
    o_Color = a_Color;
}

#type fragment
#version 450

layout(location = 0) in vec3 v_Color;
layout(location = 1) in vec2 v_TexCoord;

layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 0) uniform sampler2D u_Ambient;
layout(set = 1, binding = 1) uniform sampler2D u_Diffuse;
layout(set = 1, binding = 2) uniform sampler2D u_Specular;
layout(set = 1, binding = 3) uniform sampler2D u_SpecularHighlight;
layout(set = 1, binding = 4) uniform sampler2D u_Alpha;
layout(set = 1, binding = 5) uniform sampler2D u_Bump;
layout(set = 1, binding = 6) uniform sampler2D u_Displacement;

void main() {
    o_Color = vec4(texture(u_Diffuse, v_TexCoord).xyz, 1.0);
}