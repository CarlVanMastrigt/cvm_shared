#version 450

layout(location=0) in vec3 lpos;
layout(location=1) in vec3 wpos;

layout(location=0) out vec3 c_out;
layout(location=1) out vec3 n_out;

layout (binding=0) uniform test_uniforms
{
    mat4 projection_matrix;
    vec3 colour_offsets;
    vec3 colour_multipliers;
};

void main()
{
    c_out=lpos*colour_multipliers+colour_offsets;
    n_out=normalize(cross(dFdx(wpos), -dFdy(wpos)))*0.5+0.5;
}
