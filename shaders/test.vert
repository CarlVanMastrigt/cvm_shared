#version 450

layout (location=0) in vec3 pos;

layout (location = 0) out vec3 lpos;

layout (binding=0) uniform test_uniforms
{
    mat4 projection_matrix;
    vec3 colour_offsets;
    vec3 colour_multipliers;
};

layout (push_constant) uniform test_push_constants
{
	layout(offset=0,row_major)mat4x3 transformation_matrix;///set as row major for packing, column major 4x3 still takes 16 floats otherwise
};

void main()
{
    gl_Position=projection_matrix*vec4(transformation_matrix*vec4(pos,1.0),1.0);
    lpos=pos;
}
