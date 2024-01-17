#version 330 core
layout(location = 0) in vec3 aPos;

out vec4 fragPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
	fragPos = model * vec4(aPos, 1.0f);
	gl_Position = lightSpaceMatrix * fragPos;
}