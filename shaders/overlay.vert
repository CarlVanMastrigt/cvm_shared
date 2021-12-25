#version 450

layout (location=0) in uvec4 data0;
layout (location=1) in uvec2 data1;
layout (location=2) in uvec4 data2;


layout(location=0) flat out uvec4 d0;
layout(location=1) flat out uvec2 d1;
layout(location=2) flat out uvec4 d2;

layout (push_constant) uniform screen_dimensions
{
	layout(offset=0) vec2 inv_screen_size_x2;
};

void main()
{
    uvec2 p;
    switch(gl_VertexIndex)
    {
        case 0: p=data0.xy;break;
        case 1: p=data0.zy;break;
        case 2: p=data0.xw;break;
        default: p=data0.zw;
    }

    gl_Position=vec4(p*inv_screen_size_x2-1.0,0.0,1.0);

    d0=data0;
    d1=data1;
    d2=data2;
}
