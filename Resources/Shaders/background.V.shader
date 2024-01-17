#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 worldPos;

void main()
{
	worldPos = aPos;

	mat4 rotView = mat4(mat3(view));
	vec4 clipPos = projection * rotView * vec4(worldPos, 1.0f);

	gl_Position = vec4(clipPos.xy, clipPos.w, clipPos.w);
}