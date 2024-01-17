#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 texCoords;

void main()
{
	texCoords = aTexCoords;
	gl_Position = vec4(aPos.xy, 1.0f, 1.0f);
}