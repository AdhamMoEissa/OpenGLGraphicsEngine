#version 330 core
out vec3 upsample;
in vec2 texCoords;

uniform sampler2D srcTexture;
uniform float filterRadius;

void main()
{
	float x = filterRadius;
	float y = filterRadius;

	vec3 a = texture(srcTexture, vec2(texCoords.x - x, texCoords.y + y)).rgb;
	vec3 b = texture(srcTexture, vec2(texCoords.x, texCoords.y + y)).rgb;
	vec3 c = texture(srcTexture, vec2(texCoords.x + x, texCoords.y + y)).rgb;

	vec3 d = texture(srcTexture, vec2(texCoords.x - x, texCoords.y)).rgb;
	vec3 e = texture(srcTexture, vec2(texCoords.x, texCoords.y)).rgb;
	vec3 f = texture(srcTexture, vec2(texCoords.x + x, texCoords.y)).rgb;

	vec3 g = texture(srcTexture, vec2(texCoords.x - x, texCoords.y - y)).rgb;
	vec3 h = texture(srcTexture, vec2(texCoords.x, texCoords.y - y)).rgb;
	vec3 i = texture(srcTexture, vec2(texCoords.x + x, texCoords.y - y)).rgb;

	upsample = e * 4.0f;
	upsample += (b + d + f + h) * 2.0f;
	upsample += (a + c + g + i);
	upsample /= 16.0f;
}