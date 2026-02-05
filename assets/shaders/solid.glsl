#version 330 core

#ifdef VERTEX
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
#endif

#ifdef FRAGMENT
uniform vec4 uColor;
out vec4 FragColor;
void main() { FragColor = uColor; }
#endif
