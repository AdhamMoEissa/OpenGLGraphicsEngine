#version 330 core
out vec3 downsample;
in vec2 texCoords;

uniform sampler2D srcTexture;
uniform vec2 srcResolution;

uniform int mipLevel = 1;

const float gamma = 2.2f;

vec3 toSRGB(vec3 v)
{
	return pow(v, vec3(1 / gamma));
}

float sRGBToLuma(vec3 col)
{
	//return dot(col, vec3(0.2126f, 0.7152f, 0.0722f));
	return dot(col, vec3(0.299f, 0.587f, 0.114f));
}

float karisAverage(vec3 col)
{
	float luma = sRGBToLuma(toSRGB(col)) * 0.25f;
	return 1.0f / (1.0f + luma);
}

//Note: This shader has been written in a declarative fashion (mostly written in way that makes sense and gets the job done),
//the shader should be rewritten and optimized for performance
void main()
{
	vec2 srcTexelSize = 1.0f / srcResolution;
	float x = srcTexelSize.x;
	float y = srcTexelSize.y;

	// Take 13 samples around current texel:
	// a  -  b  -  c
	// -  j  -  k  -
	// d  -  e  -  f
	// -  l  -  m  -
	// g  -  h  -  i
	// === ('e' is the current texel) ===
	vec3 a = texture(srcTexture, vec2(texCoords.x - 2 * x, texCoords.y + 2 * y)).rgb;
	vec3 b = texture(srcTexture, vec2(texCoords.x, texCoords.y + 2 * y)).rgb;
	vec3 c = texture(srcTexture, vec2(texCoords.x + 2 * x, texCoords.y + 2 * y)).rgb;

	vec3 d = texture(srcTexture, vec2(texCoords.x - 2 * x, texCoords.y)).rgb;
	vec3 e = texture(srcTexture, vec2(texCoords.x, texCoords.y)).rgb;
	vec3 f = texture(srcTexture, vec2(texCoords.x + 2 * x, texCoords.y)).rgb;

	vec3 g = texture(srcTexture, vec2(texCoords.x - 2 * x, texCoords.y - 2 * y)).rgb;
	vec3 h = texture(srcTexture, vec2(texCoords.x, texCoords.y - 2 * y)).rgb;
	vec3 i = texture(srcTexture, vec2(texCoords.x + 2 * x, texCoords.y - 2 * y)).rgb;

	vec3 j = texture(srcTexture, vec2(texCoords.x - x, texCoords.y + y)).rgb;
	vec3 k = texture(srcTexture, vec2(texCoords.x + x, texCoords.y + y)).rgb;

	vec3 l = texture(srcTexture, vec2(texCoords.x - x, texCoords.y - y)).rgb;
	vec3 m = texture(srcTexture, vec2(texCoords.x + x, texCoords.y - y)).rgb;

	// Apply weighted distribution:
	// 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
	// a,b,d,e * 0.125
	// b,c,e,f * 0.125
	// d,e,g,h * 0.125
	// e,f,h,i * 0.125
	// j,k,l,m * 0.5
	// This shows 5 square areas that are being sampled. But some of them overlap,
	// so to have an energy preserving downsample we need to make some adjustments.
	// The weights are the distributed, so that the sum of j,k,l,m (e.g.)
	// contribute 0.5 to the final color output. The code below is written
	// to effectively yield this sum. We get:
	// 0.125*5 + 0.03125*4 + 0.0625*4 = 1

	// Check if we need to perform Karis average on each block of 4 samples
	switch(mipLevel)
	{
		case 0:
			vec3 groups[5];
			groups[0] = (a + b + d + e) * (0.125f / 4.0f);
			groups[1] = (b + c + e + f) * (0.125f / 4.0f);
			groups[2] = (d + e + g + h) * (0.125f / 4.0f);
			groups[3] = (e + f + h + i) * (0.125f / 4.0f);
			groups[4] = (j + k + l + m) * (0.5f / 4.0f);
			groups[0] *= karisAverage(groups[0]);
			groups[1] *= karisAverage(groups[1]);
			groups[2] *= karisAverage(groups[2]);
			groups[3] *= karisAverage(groups[3]);
			groups[4] *= karisAverage(groups[4]);
			downsample = groups[0] + groups[1] + groups[2] + groups[3] + groups[4];
			downsample = max(downsample, 0.000001f);
			break;
		default:
			downsample = e * 0.125f;
			downsample += (a + c + g + i) * 0.03125;
			downsample += (b + d + f + h) * 0.0625;
			downsample += (j + k + l + m) * 0.125;
			break;
	}
}