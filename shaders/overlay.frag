#version 450

layout(set=0,binding=0) uniform overlay_colours
{
    vec4 colours[];
};

layout(set=1,binding=0) uniform sampler2D transparent_image;

layout(push_constant) uniform screen_dimensions
{
    layout(offset=0) vec2 screen_size;
};

layout(location=0) in vec4 c_in;

layout(location=0) out vec4 c_out;

void main()
{
    c_out=c_in*colours[0]*vec4(textureLod(transparent_image,gl_FragCoord.xy,0).rrr,1.0);
}
