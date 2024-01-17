#version 330 core
out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform sampler2D lensDirtTexture;

uniform float exposure = 1.0f;
uniform float bloomStrength = 0.05f;
uniform float NrOfMips = 5.0f;
uniform bool isLensDirt;

vec3 bloom(vec3 hdr, vec3 bloom, vec3 dirt, bool lensDirt)
{
	if(lensDirt)
		return mix(hdr, (bloom + dirt * bloomStrength) / pow(NrOfMips, 1 / (1.0f + bloomStrength)), bloomStrength);
	else
		return mix(hdr, (bloom) / pow(NrOfMips, 1 / (1.0f + bloomStrength)), bloomStrength);

	return mix(mix(hdr, bloom + dirt * bloomStrength, bloomStrength),
			   mix(hdr, bloom + dirt, bloomStrength) / sqrt(NrOfMips),
			   bloomStrength);
}

void main()
{
	vec3 hdrColor = texture(scene, texCoords).rgb;
	vec3 bloomColor = texture(bloomBlur, texCoords).rgb;
	vec3 lensDirt = texture(lensDirtTexture, texCoords).rgb;

	vec3 result = bloom(hdrColor, bloomColor, lensDirt, isLensDirt);

	// HDR/Tone mapping
	result = smoothstep(vec3(0.0f), vec3(1.0f - exp(-(exposure + 0.000001))), result * (exposure + 0.000001));
	result = smoothstep(vec3(0.0f), vec3(1.0f - exp(-(exposure + 0.000001))), result * (exposure + 0.000001));

	// gamma correction
	result = pow(result, vec3(1.0f / 2.2f));

	FragColor = vec4(result, 1.0f);
}