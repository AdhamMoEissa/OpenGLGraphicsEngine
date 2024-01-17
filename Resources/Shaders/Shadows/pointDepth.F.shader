#version 330 core
out float fragDepth;
in vec4 fragPos;

uniform vec3 lightPos;
uniform float farPlane = 100.0f;

void main()
{
	float lightDistance = length(fragPos.xyz - lightPos);

	lightDistance = lightDistance / farPlane;
	fragDepth = lightDistance;
}