#version 450

layout (location=0) in vec3 pos;
layout (location=1) in vec3 c_in;

layout (location = 0) out vec3 c_out;

layout (binding=0) uniform test_uniforms
{
    vec4 colour_multiplier;
};

layout (push_constant) uniform test_push_constants
{
	layout(offset=0)mat4 projection_matrix;
};

void main()
{
    gl_Position=projection_matrix*vec4(pos,1.0);
    c_out=c_in*colour_multiplier.rgb;
}
