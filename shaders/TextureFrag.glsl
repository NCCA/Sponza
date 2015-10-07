#version 330 core
/// @brief our output fragment colour
layout (location =0) out vec4 fragColour;
// this is a pointer to the current 2D texture object
uniform sampler2D tex;
// the vertex UV
in vec2 vertUV;
uniform vec3 ka;
uniform float transp;
void main ()
{
  vec4 diffuse = texture(tex,vertUV);
  if (diffuse.a == 0)
      discard;
 // set the fragment colour to the current texture
 fragColour = vec4(ka,transp)*diffuse;
}
