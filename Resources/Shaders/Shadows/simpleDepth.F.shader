#version 330 core
out float fragDepth;
in vec4 fragPos;

uniform int doLinearizeDepth = 0;
uniform vec3 lightPos;
uniform float farPlane = 100.0f;

float linearizeDepth()
{
	float lightDistance = length(lightPos - fragPos.xyz);

	return lightDistance / farPlane;
}

void main()
{
	if(doLinearizeDepth != 0)
	{
		fragDepth = linearizeDepth();
	}
	else
	{
		fragDepth = gl_FragCoord.z;
	}
}