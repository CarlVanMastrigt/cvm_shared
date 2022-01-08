#version 450

vec2 pos[3]=
{
    vec2(-1,-1),
    vec2(-1, 3),
    vec2( 3,-1),
};

void main()
{
    gl_Position=vec4(pos[gl_VertexIndex],1.0,1.0);
}
