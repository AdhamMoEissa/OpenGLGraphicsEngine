#version 330 core
out vec4 FragColor;
in vec3 worldPos;

uniform samplerCube environmentMap;

void main()
{
	//stores environment map color
	vec3 envColor = textureLod(environmentMap, worldPos, 0.0).rgb;

	//HDR + Gamma correction
	envColor = envColor / (envColor + vec3(1.0f));
	envColor = pow(envColor, vec3(1.0f / 2.2f));

	FragColor = vec4(envColor, 1.0f);
	gl_FragDepth = 1.0f;
}