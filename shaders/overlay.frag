#version 450

layout(set=0,binding=0) uniform overlay_colours
{
    vec4 colours[];
};

layout(set=1,binding=0) uniform sampler2D transparent_image;
layout(set=1,binding=1) uniform sampler2D colour_image;

layout(push_constant) uniform screen_dimensions
{
    layout(offset=8) vec2 screen_size;
};

layout(location=0) flat in uvec4 d0;
layout(location=1) flat in uvec4 d1;
//layout(location=2) flat in uvec4 d2;

layout(location=3) in vec4 c_in;

layout(location=0) out vec4 c_out;

void main()
{
    vec4 c;
    switch(d1.x)
    {
        case 0: c=vec4(1.0.xxx,textureLod(transparent_image,gl_FragCoord.xy-d0.xy+d1.yz,0));break;
        case 1: c=textureLod(colour_image,gl_FragCoord.xy-d0.xy+d1.yz,0);break;
        default: c=1.0.xxxx;
    }

    c_out=c_in*colours[0]*c;
}
