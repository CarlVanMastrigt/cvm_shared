#version 450

layout(location=0) in vec4 colour;

layout(location=0) out vec4 c_out;

void main()
{
    c_out=colour;
}


