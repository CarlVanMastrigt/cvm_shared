#version 450

layout(location=0) out vec4 c_out;

layout (push_constant) uniform screen_dimensions
{
	layout(offset=8) vec2 inv_screen_size;
};

const vec2 positions[4]=
{
    vec2(0,0),
    vec2(1,0),
    vec2(0,1),
    vec2(1,1)
};

void main()
{

    gl_Position=vec4(positions[gl_VertexIndex]-0.5.xx,0.0,1.0);
    c_out=vec4(0.2,0.3,0.7,0.6);
}
