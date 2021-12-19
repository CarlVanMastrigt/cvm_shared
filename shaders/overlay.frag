#version 450

layout(set=0,binding=0) uniform overlay_colours
{
    vec4 colours[16];
};

layout(set=1,binding=0) uniform sampler2D transparent_image;
layout(set=1,binding=1) uniform sampler2D colour_image;

layout(push_constant) uniform screen_dimensions
{
    layout(offset=8) vec2 screen_size;
};

layout(location=0) flat in uvec4 d0;
layout(location=1) flat in uvec4 d1;
layout(location=2) flat in uvec4 d2;

//layout(location=3) in vec4 c_in;

layout(location=0) out vec4 c_out;

void main()
{
    switch(d1.x&0x3000)
    {
        case 0x1000: c_out=vec4(1.0.xxx,textureLod(transparent_image,gl_FragCoord.xy-d0.xy+d1.yz,0).x);break;
        case 0x2000: c_out=textureLod(colour_image,gl_FragCoord.xy-d0.xy+d1.yz,0);break;
        default:c_out=c_out=1.0.xxxx;
    }

    switch(d1.x&0xC000)
    {
        case 0x4000: c_out.a=min(c_out.a,textureLod(transparent_image,gl_FragCoord.xy-d0.xy+d2.xy,0).x);break;
        case 0x8000: c_out.a*=textureLod(transparent_image,gl_FragCoord.xy-d0.xy+d2.xy,0).x;break;
    }

    if((d1.x&0x3000)!=0x2000)c_out*=colours[d1.x&0x0FFF];
}
