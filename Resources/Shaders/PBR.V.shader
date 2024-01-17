#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec2 texCoords;
out vec3 worldPos;
out vec3 normal;
out vec4 fragPosLightSpace[4];

uniform mat4 lightSpaceMatrix[4];
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	texCoords = aTexCoords;
	worldPos = vec3(model * vec4(aPos, 1.0f));
	normal = mat3(model) * aNormal;
	for(int i = 0; i < 4; i++)
	{
		fragPosLightSpace[i] = lightSpaceMatrix[i] * vec4(worldPos, 1.0);
	}

	gl_Position = projection * view * vec4(worldPos, 1.0f);
}