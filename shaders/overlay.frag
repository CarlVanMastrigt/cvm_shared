#version 450

layout(set=0,binding=0) uniform overlay_colours
{
    vec4 colours[];
};

layout(push_constant) uniform screen_dimensions
{
    layout(offset=0) vec2 screen_size;
};

layout(location=0) in vec4 c_in;

layout(location=0) out vec4 c_out;

void main()
{
    c_out=c_in*colours[1];
}
