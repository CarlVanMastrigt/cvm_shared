#version 450

layout(location=0) in vec3 lpos;

layout(location=0) out vec3 c_out;

layout (binding=0) uniform test_uniforms
{
    mat4 projection_matrix;
    vec3 colour_offsets;
    vec3 colour_multipliers;
};

void main()
{
    c_out=lpos*colour_multipliers;
}
