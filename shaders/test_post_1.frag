#version 450

layout (binding=0) uniform test_uniforms
{
    mat4 inverse_projection;
    vec2 inverse_screen_size;
};

layout(input_attachment_index=0,set=0,binding=1)uniform subpassInput depth_input;
layout(input_attachment_index=1,set=0,binding=2)uniform subpassInput colour_input;
layout(input_attachment_index=2,set=0,binding=3)uniform subpassInput normal_input;

layout(location=0) out vec3 c_out;

void main()
{
    c_out=subpassLoad(colour_input).rgb*(0.5+0.5*dot(0.57735026919.xxx,subpassLoad(normal_input).xyz*2.0-1.0));
}
