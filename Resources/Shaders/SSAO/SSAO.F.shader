#version 330 core
out vec4 fragColor;

in vec2 texCoords;

uniform vec3 samples[64];
uniform int kernelSize = 64;
uniform float radius = 0.5;
uniform float bias = 0.025;
uniform float time = 1.0f;
uniform vec2 noiseScale;

uniform mat4 projection;
uniform mat4 invProjection;

uniform sampler2D gMaterialMask;
uniform sampler2D gNormalShadow;
uniform sampler2D gMetalRoughAO;
uniform sampler2D gDepth;
uniform sampler2D noiseTex;

const float Pi = 3.14159265359f;

vec3 getPosition(sampler2D depthMap, vec2 textureCoords, mat4 inverseProjection);

void main()
{
	vec3 material = texture(gMaterialMask, texCoords).rgb;
	if(material == vec3(0.0f)) //Discard fragment if the material mask buffer is empty at this position
		discard;

	vec3 result = vec3(0.0f);
	vec3 viewPos = getPosition(gDepth, texCoords, invProjection);
	vec3 normal = normalize(texture(gNormalShadow, texCoords).rgb);
	vec3 randomVec = normalize(texture(noiseTex, texCoords * noiseScale * time).rgb);

	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float occlusion = 0.0f;

	for(int i = 0; i < kernelSize; i++)
	{
		vec3 samplePos = TBN * samples[i];
		samplePos = viewPos + samplePos * radius;

		vec4 offset = vec4(samplePos, 1.0f);
		offset = projection * offset;
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * 0.5 + 0.5;

		float sampleDepth = getPosition(gDepth, offset.xy, invProjection).z;

		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(viewPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0f - (occlusion / kernelSize);
	occlusion = texture(gMetalRoughAO, texCoords).b * occlusion;
	result = vec3(texture(gMetalRoughAO, texCoords).rg, occlusion);
	fragColor = vec4(result, 1.0f);
}

vec3 getPosition(sampler2D depthMap, vec2 textureCoords, mat4 inverseProjection)
{
	float depthValue = texture(depthMap, textureCoords).r;
	vec2 xy = textureCoords * 2.0f - vec2(1.0f);
	float z = depthValue * 2.0f - 1.0f;
	vec4 clipSpacePosition = vec4(xy, z, 1.0f);
	vec4 viewSpacePosition = inverseProjection * clipSpacePosition;
	vec3 res = viewSpacePosition.xyz / viewSpacePosition.w;
	return res;
}