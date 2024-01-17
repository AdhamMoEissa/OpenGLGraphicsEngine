#version 330 core
out vec4 fragColor;

in vec2 texCoords;

struct Light
{
	int m_Type;
	float m_FarPlane;
	float m_Resolution;
	vec3 m_Pos;
	vec3 m_Color;
	//sampler2D m_2DShadowMap;
	samplerCube m_CubeShadowMap;
};

uniform vec3 camPos;
uniform mat4 invView;
uniform mat4 invProjection;
uniform Light light;
uniform sampler2D gMaterialMask;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMetalRoughAO;
uniform sampler2D gDepth;

vec3 PowVec3(vec3 v, float p)
{
	return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float invGamma = 1.0f / 2.2f;
const float Pi = 3.14159265359f;
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
	vec3 result = vec3(0.0f);
	float depth = texture(gDepth, texCoords).r;
	vec3 viewPos = getPosition(depth, texCoords, invProjection);
	vec3 worldPos = vec3(invView * vec4(viewPos, 1.0f));
	float shadow = 0.0f;

	vec3 fragToLight = worldPos - light.m_Pos;
	float currentDepth = length(fragToLight) / light.m_FarPlane;
	vec3 lightDir = normalize(-fragToLight);
	vec3 normal = normalize(texture(gNormal, texCoords).rgb);
	float bias = max(0.0001f * dot(normal, lightDir), 0.01f);
	vec3 viewDir = normalize(camPos - worldPos);
	{	//shadow calculations	
		float x = 2.0 / light.m_Resolution;
		float y = x;
		float z = x;

		float depth = currentDepth - bias;

		float shadowMapDepth = texture(light.m_CubeShadowMap, normalize(fragToLight + vec3(0.0f, 0.0f, 0.0f))).r;
		shadow = depth > shadowMapDepth ? 1.0f : 0.0f;


		shadowMapDepth = texture(light.m_CubeShadowMap, normalize(fragToLight + vec3(x, y, z))).r;
		shadow += depth > shadowMapDepth ? 1.0f : 0.0f;
		shadowMapDepth = texture(light.m_CubeShadowMap, normalize(fragToLight + vec3(-x, y, z))).r;
		shadow += depth > shadowMapDepth ? 1.0f : 0.0f;
		shadowMapDepth = texture(light.m_CubeShadowMap, normalize(fragToLight + vec3(x, -y, z))).r;
		shadow += depth > shadowMapDepth ? 1.0f : 0.0f;
		shadowMapDepth = texture(light.m_CubeShadowMap, normalize(fragToLight + vec3(-x, -y, z))).r;
		shadow += depth > shadowMapDepth ? 1.0f : 0.0f;
		shadowMapDepth = texture(light.m_CubeShadowMap, normalize(fragToLight + vec3(x, y, -z))).r;
		shadow += depth > shadowMapDepth ? 1.0f : 0.0f;
		shadowMapDepth = texture(light.m_CubeShadowMap, normalize(fragToLight + vec3(-x, y, -z))).r;
		shadow += depth > shadowMapDepth ? 1.0f : 0.0f;
		shadowMapDepth = texture(light.m_CubeShadowMap, normalize(fragToLight + vec3(x, -y, -z))).r;
		shadow += depth > shadowMapDepth ? 1.0f : 0.0f;
		shadowMapDepth = texture(light.m_CubeShadowMap, normalize(fragToLight + vec3(-x, -y, -z))).r;
		shadow += depth > shadowMapDepth ? 1.0f : 0.0f;

		shadow /= 9.0;

		// keep the shadow at 0.0 when outside the farPlane region of the light's frustum.
		//if(length(fragToLight / light.m_FarPlane) > 1.0)
		//	shadow = 0.0;
	}

	//lighting calculations
	{
		vec3 albedo = texture(gAlbedo, texCoords).rgb;
		float metallic = texture(gMetalRoughAO, texCoords).r;
		float roughness = texture(gMetalRoughAO, texCoords).g;
		vec3 F0 = vec3(0.04f);
		F0 = mix(F0, albedo, metallic);
		//regular per light calculations
		vec3 lightDir = normalize(-fragToLight);
		vec3 halfwayDir = normalize(viewDir + lightDir);
		float distance = length(fragToLight);
		float attenuation = 1.0f / (1 + distance + distance * distance);
		vec3 radiance = light.m_Color * attenuation;

		//Cook-Torrance BRDF
		float NDF = distributionGGX(normal, halfwayDir, roughness);
		float G = geometrySmith(normal, viewDir, lightDir, roughness);
		vec3 F = fresnelSchlick(max(dot(halfwayDir, viewDir), 0.0f), F0);

		vec3 numerator = NDF * G * F;
		float denominator = 4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDir), 0.0f) + 0.0001f;//we add this negligible value at the end to prevent divide by zero errors
		vec3 specular = numerator / denominator;

		//kS is equal to the fresnel value
		vec3 kS = F;
		//to keep lighting energy conservative, we must make sure that both the specular(kS) and diffuse(kD) parts equal 1.0f
		vec3 kD = vec3(1.0f) - kS;
		//the more metallic the material, the less it diffuses light
		kD *= 1.0f - metallic;

		float NdotL = max(dot(normal, lightDir), 0.0f);

		result = (kD * albedo / Pi + specular) * radiance * NdotL;//note: the BRDF part has already been multiplied by the fresnel value (kS) so doesn't need to be multiplied by kS again
	}

	result = result * (1.0f - shadow);
	fragColor = vec4(result.rgb, 1.0f);
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