#version 330 core
layout(location = 0) out vec3 gMaterialMask;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormalShadow;
layout(location = 3) out vec4 gAlbedo;
layout(location = 4) out vec3 gMetalRoughAO;

in vec2 texCoords;

uniform int karis2;
uniform vec2 resolution;
uniform sampler2D ShadowFirstPassBuffer;

vec3 PowVec3(vec3 v, float p)
{
	return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float invGamma = 1.0f / 2.2f;
vec3 ToSRGB(vec3 v) { return PowVec3(v, invGamma); }

float sRGBToLuma(vec3 col)
{
	//return dot(col, vec3(0.2126f, 0.7152f, 0.0722f));
	return dot(col, vec3(0.299f, 0.587f, 0.114f));
}

float KarisAverage(vec3 col)
{
	// Formula is 1 / (1 + luma)
	float luma = sRGBToLuma(ToSRGB(col)) * 0.25f;
	return 1.0f / (1.0f + luma);
}

void main()
{
	float x = 1.0 / resolution.x;
	float y = 1.0 / resolution.y;

	float a = texture(ShadowFirstPassBuffer, texCoords).a;

	a += texture(ShadowFirstPassBuffer, texCoords + vec2( x,  y)).a;
	a += texture(ShadowFirstPassBuffer, texCoords + vec2(-x,  y)).a;
	a += texture(ShadowFirstPassBuffer, texCoords + vec2( x, -y)).a;
	a += texture(ShadowFirstPassBuffer, texCoords + vec2(-x, -y)).a;

	a += texture(ShadowFirstPassBuffer, texCoords + vec2(2 * x, 2 * y)).a;
	a += texture(ShadowFirstPassBuffer, texCoords + vec2(2 *-x, 2 * y)).a;
	a += texture(ShadowFirstPassBuffer, texCoords + vec2(2 * x, 2 *-y)).a;
	a += texture(ShadowFirstPassBuffer, texCoords + vec2(2 *-x, 2 *-y)).a;

	a /= 9.0;

	//vec3 a = texture(shadowMap[i], vec2(texCoords.x - 2 * x, texCoords.y + 2 * y)).rgb;
	//vec3 b = texture(shadowMap[i], vec2(texCoords.x		   , texCoords.y + 2 * y)).rgb;
	//vec3 c = texture(shadowMap[i], vec2(texCoords.x + 2 * x, texCoords.y + 2 * y)).rgb;
	//
	//vec3 d = texture(shadowMap[i], vec2(texCoords.x - 2 * x, texCoords.y)).rgb;
	//vec3 e = texture(shadowMap[i], vec2(texCoords.x		   , texCoords.y)).rgb;
	//vec3 f = texture(shadowMap[i], vec2(texCoords.x + 2 * x, texCoords.y)).rgb;
	//
	//vec3 g = texture(shadowMap[i], vec2(texCoords.x - 2 * x, texCoords.y - 2 * y)).rgb;
	//vec3 h = texture(shadowMap[i], vec2(texCoords.x		   , texCoords.y - 2 * y)).rgb;
	//vec3 i = texture(shadowMap[i], vec2(texCoords.x + 2 * x, texCoords.y - 2 * y)).rgb;
	//
	//vec3 j = texture(shadowMap[i], vec2(texCoords.x - x, texCoords.y + y)).rgb;
	//vec3 k = texture(shadowMap[i], vec2(texCoords.x + x, texCoords.y + y)).rgb;
	//vec3 l = texture(shadowMap[i], vec2(texCoords.x - x, texCoords.y - y)).rgb;
	//vec3 m = texture(shadowMap[i], vec2(texCoords.x + x, texCoords.y - y)).rgb;

	// keep the shadow at 0.0 when outside the farPlane region of the light's frustum.
	gNormalShadow.rgb = texture(ShadowFirstPassBuffer, texCoords).rgb;
	gNormalShadow.a += a;
}
