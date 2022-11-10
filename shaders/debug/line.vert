#version 450

layout (location=0) in vec3 pos;
layout (location=1) in vec4 col;
/// instanced input for 4x3 transposed matrix containing position

layout (location = 0) out vec4 colour;

layout (set=0,binding=0) uniform primary_uniforms
{
    mat4 projection_matrix;
//    mat4 inverse_projection;
//    vec2 inverse_screen_size;
//    uint sample_count;///not relevant here (non MSAA)
//    uint sample_mask;///not relevant here (non MSAA)
//    vec2 sample_locations[16];///not relevant here (non MSAA)
};


void main()
{
    colour=col.abgr;
    gl_Position=projection_matrix*vec4(pos,1.0);
}
