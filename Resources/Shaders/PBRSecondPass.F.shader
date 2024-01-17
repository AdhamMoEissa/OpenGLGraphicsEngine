#version 330 core
out vec4 fragColor;

in vec2 texCoords;

//material properties
uniform sampler2D gMaterialMask;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMetalRoughAO;
uniform sampler2D gDepth;

//IBL(Image based lighting)
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform sampler2D LoMap;

uniform vec3 camPos;
uniform mat4 invProjection;
uniform mat4 invView;

const float Pi = 3.14159265359f;

float distributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
	denom = Pi * denom * denom;

	return nom / denom;
}

float geometrySchlickGGX(float  NdotV, float roughness)
{
	float r = (roughness + 1.0f);
	float k = (r * r) / 8.0f;

	float nom = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx2 = geometrySchlickGGX(NdotV, roughness);
	float ggx1 = geometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

vec3 getPosition(float depthValue, vec2 textureCoords, mat4 inverseProjection);

void main()
{
	//retrieves material properties
	vec3 material = texture(gMaterialMask, texCoords).rgb;
	if(material.r == 0.0f && material.g == 0.0f && material.b == 0.0f)
	{
		fragColor = vec4(1.0f, 0.0f, 1.0f, 1.0f);
		gl_FragDepth = 0.0f;
		discard;
	}
	float depth = texture(gDepth, texCoords).r;

	vec3 viewPos = getPosition(depth, texCoords, invProjection);
	vec3 worldPos = vec3(invView * vec4(viewPos, 1.0f));
	vec3 albedo = pow(texture(gAlbedo, texCoords).rgb, vec3(2.2f));
	float metallic = texture(gMetalRoughAO, texCoords).r;
	float roughness = texture(gMetalRoughAO, texCoords).g;
	float ao = texture(gMetalRoughAO, texCoords).b;
	
	vec3 N = texture(gNormal, texCoords).rgb;
	vec3 V = normalize(camPos - worldPos);
	vec3 R = reflect(-V, N);

	//uses F0 = 0.04f if material is a di-electric (non-metal) and uses F0 = albedo color if material is a conductor (metal).
	//the mix function linearly interpolates between 0.04 and albedo color according to the metallic value of the material
	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedo, metallic);

	vec3 Lo = texture(LoMap, texCoords).rgb;
	//ambient lighting using IBL
	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0f), F0, roughness);

	vec3 kS = F;
	vec3 kD = vec3(1.0f) - kS;
	kD *= 1.0f - metallic;

	vec3 irradiance = texture(irradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo;

	const float MAX_REFLECTION_LOD = 4.0f;
	vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0f), roughness)).rg;
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * ao;
	float shadow = texture(gNormal, texCoords).a;
	vec3 color = ambient + Lo;

	//HDR + Gamma correction
	color = color / (color + vec3(1.0f));
	color = pow(color, vec3(1.0f / 2.2f));

	fragColor = vec4(color.xyz, 1.0f);
}

vec3 getPosition(float depthValue, vec2 textureCoords, mat4 inverseProjection)
{
	vec2 xy = textureCoords * 2.0f - vec2(1.0f);
	float z = depthValue * 2.0f - 1.0f;
	vec4 clipSpacePosition = vec4(xy, z, 1.0f);
	vec4 viewSpacePosition = inverseProjection * clipSpacePosition;
	vec3 res = viewSpacePosition.xyz / viewSpacePosition.w;
	return res;
}