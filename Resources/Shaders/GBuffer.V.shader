#version 330
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 materialMask;
out vec2 texCoords;
out vec4 worldSpacePos;
out vec3 normal;

uniform vec3 material = vec3(0.0f);
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	materialMask = material;
	texCoords = aTexCoords;
	worldSpacePos = model * vec4(aPos, 1.0f);
	vec3 viewSpacePos = vec3(view * worldSpacePos);
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	normal = normalMatrix * aNormal;
	gl_Position = projection * vec4(viewSpacePos, 1.0f);
}