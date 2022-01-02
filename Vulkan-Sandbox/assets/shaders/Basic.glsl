#type vertex
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 u_ViewProjection;
};

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoords;
layout(location = 3) in int a_MaterialIndex;

layout(location = 0) out vec2 o_TexCoord;
layout(location = 1) out flat int o_MaterialIndex;

layout(push_constant) uniform PushConstants
{
    mat4 p_Model;
};

void main() {
    gl_Position = u_ViewProjection * p_Model * vec4(a_Position, 1.0);
    o_TexCoord = a_TexCoords;
    o_MaterialIndex = a_MaterialIndex;
}

#type fragment
#version 450

struct MaterialData
{
    vec3 AmbientColor;
    int AmbientTextureIndex;

    vec3 DiffuseColor;
    int DiffuseTextureIndex;

    vec3 SpecularColor;
    int SpecularTextureIndex;

    int SpecularHighlightTextureIndex;
    float Specularity;
    float IOR;
    float Dissolve;

    vec3 EmissiveColor;
    int AlphaMapIndex;
    
    vec3 TransmittanceFilter;
    int BumpMapIndex;

    int DisplacementMapIndex;
    int IlluminationModel;
};

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in flat int v_MaterialIndex;

layout(location = 0) out vec4 o_Color;

layout(std140, set = 0, binding = 1) uniform MaterialDataStub 
{
    MaterialData u_MaterialData[MATERIAL_ARRAY_SIZE];
};

layout(set = 0, binding = 2) uniform sampler2D u_Textures[TEXTURE_ARRAY_SIZE];

void main() {
    if (u_MaterialData[v_MaterialIndex].DiffuseTextureIndex != -1)
    {
        o_Color = vec4(
            texture(u_Textures[u_MaterialData[v_MaterialIndex].DiffuseTextureIndex],
            v_TexCoord).xyz,
            1.0);
    }
    else
    {
        o_Color = vec4(0.733, 0.047, 0.047, 1.0);
    }
}