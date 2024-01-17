#version 330
layout(location = 0) out vec3 gMaterialMask;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec3 gMetalRoughAO;

in vec3 materialMask;
in vec2 texCoords;
in vec4 worldSpacePos;
in vec3 normal;

//material properties
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform float time;

const float Pi = 3.14159265359f;

vec3 getNormalFromMap(sampler2D map, vec3 pos, vec3 norm, vec2 uv);

void main()
{
	gMaterialMask = materialMask;

	gNormal.rgb = getNormalFromMap(normalMap, worldSpacePos.xyz, normal, texCoords);
	gNormal.a = 1.0f;

	gAlbedo = texture(albedoMap, texCoords).rgba;
	gMetalRoughAO.r = texture(metallicMap, texCoords).r;
	gMetalRoughAO.g = texture(roughnessMap, texCoords).r;
	gMetalRoughAO.b = texture(aoMap, texCoords).r;
}

vec3 getNormalFromMap(sampler2D map, vec3 pos, vec3 norm, vec2 uv)
{
	vec3 tangentNormal = texture(map, uv).xyz * 2.0f - 1.0f;

	vec3 Q1 = dFdx(pos);
	vec3 Q2 = dFdy(pos);
	vec2 st1 = dFdx(uv);
	vec2 st2 = dFdy(uv);

	vec3 N = normalize(norm);
	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}
