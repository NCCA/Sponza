#version 400 core
/// @brief the vertex passed in
layout (location = 0) in vec3 inVert;
/// @brief the normal passed in
layout (location = 2) in vec3 inNormal;
/// @brief the in uv
layout (location = 1) in vec2 inUV;
uniform mat4 MVP;
// we use this to pass the UV values to the frag shader
out vec2 vertUV;

void main(void)
{
// pre-calculate for speed we will use this a lot

// calculate the vertex position
gl_Position = MVP*vec4(inVert, 1.0);
// pass the UV values to the frag shader
vertUV=inUV.st;
}
