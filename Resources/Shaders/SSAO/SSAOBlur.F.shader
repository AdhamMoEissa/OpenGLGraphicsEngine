#version 330
layout(location = 0) out vec3 gMaterialMask;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormalShadow;
layout(location = 3) out vec4 gAlbedo;
layout(location = 4) out vec3 gMetalRoughAO;

in vec2 texCoords;

uniform sampler2D occlusionBuffer;
uniform vec2 resolution;

void main()
{
	float x = 1.0f / resolution.x;
	float y = 1.0f / resolution.y;

	float occ = 2.0f * texture(occlusionBuffer, texCoords + vec2(0.0f, 0.0f)).b;

	occ += 0.5f * texture(occlusionBuffer, texCoords + vec2(0.5 * x, 0.5 * y)).b;
	occ += 0.5f * texture(occlusionBuffer, texCoords + vec2(0.5 * -x, 0.5 * y)).b;
	occ += 0.5f * texture(occlusionBuffer, texCoords + vec2(0.5 * x, 0.5 * -y)).b;
	occ += 0.5f * texture(occlusionBuffer, texCoords + vec2(0.5 * -x, 0.5 * -y)).b;

	occ += 0.25f * texture(occlusionBuffer, texCoords + vec2(1.5 * x, 1.5 * y)).b;
	occ += 0.25f * texture(occlusionBuffer, texCoords + vec2(1.5 * -x, 1.5 * y)).b;
	occ += 0.25f * texture(occlusionBuffer, texCoords + vec2(1.5 * x, 1.5 * -y)).b;
	occ += 0.25f * texture(occlusionBuffer, texCoords + vec2(1.5 * -x, 1.5 * -y)).b;

	occ = occ / 5.0f;

	gMetalRoughAO.r = texture(occlusionBuffer, texCoords).r;
	gMetalRoughAO.g = texture(occlusionBuffer, texCoords).g;
	gMetalRoughAO.b = occ;
}