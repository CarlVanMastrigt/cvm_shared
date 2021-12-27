#version 450

layout (location=0) in vec3 pos;

layout (location = 0) out vec3 lpos;

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
    lpos=pos;
}
