#version 450

layout(location=0) in vec3 lpos;

layout(location=0) out vec3 c_out;

void main()
{
    c_out=lpos;
}
