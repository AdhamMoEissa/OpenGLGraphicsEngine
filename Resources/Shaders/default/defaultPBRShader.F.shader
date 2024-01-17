#version 330 core
#define DIRECTIONAL_LIGHT 0
#define PERSPECTIVE_LIGHT 1
#define POINT_LIGHT 2
out vec4 FragColor;
in vec2 texCoords;
in vec3 worldPos;
in vec3 normal;
in vec4 fragPosLightSpace;

struct PointLight
{
	int m_Type;
	float m_FarPlane;
	vec3 m_Pos;
	vec3 m_Color;
	vec4 fragPosLightSpace;
	samplerCube m_ShadowMap;
};
struct Light
{
	int m_Type;
	float m_FarPlane;
	vec3 m_Pos;
	vec3 m_Color;
	vec4 fragPosLightSpace;
	sampler2D m_ShadowMap;
};


//material properties
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

//IBL(Image based lighting)
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

//lights
uniform vec3 lightPositions;
uniform vec3 lightColors;

uniform Light lights;

uniform vec3 camPos;

const float Pi = 3.14159265359f;

vec3 getNormalFromMap()
{
	vec3 tangentNormal = texture(normalMap, texCoords).xyz * 2.0f - 1.0f;

	vec3 Q1 = dFdx(worldPos);
	vec3 Q2 = dFdy(worldPos);
	vec2 st1 = dFdx(texCoords);
	vec2 st2 = dFdy(texCoords);

	vec3 N = normalize(normal);
	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
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

float calculateShadow(int index)
{
	/*if(lights[index].m_Type == DIRECTIONAL_LIGHT)
	{
		// perform perspective divide
		vec3 projCoords = fragPosLightSpace[index].xyz / fragPosLightSpace[index].w;
		//vec3 projCoords = fragPosLightSpace.xyz;
		projCoords = projCoords * 0.5 + 0.5;
		// get depth of current fragment from light's perspective
		float currentDepth = projCoords.z;


		vec3 lightDir = normalize(lights[index].m_Pos - worldPos);
		//float bias = max(0.01f * dot(normal, lightDir), 0.001f);
		float bias = 0.0f;
		bias = mix(0.001f, 0.01f, max(dot(normal, lightDir), 0.0f));

		//PCF
		float shadow = 0.0;
		vec2 texelSize = 1.0 / textureSize(lights[index].m_2DShadowMap, 0);
		for(int x = -1; x <= 1; ++x)
		{
			for(int y = -1; y <= 1; ++y)
			{
				float pcfDepth = texture(lights[index].m_2DShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
				shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
			}
		}
		shadow /= 9.0;

		//float shadow = currentDepth - bias > texture(lights[index].m_ShadowMap, projCoords.xy).r ? 1.0 : 0.0;

		// keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
		if(projCoords.z > 1.0)
			shadow = 0.0;

		return shadow;
	}
	else if(lights[index].m_Type == PERSPECTIVE_LIGHT)
	{
		// perform perspective divide
		vec3 projCoords = fragPosLightSpace[index].xyz / fragPosLightSpace[index].w;
		//vec3 projCoords = fragPosLightSpace.xyz;
		projCoords = projCoords * 0.5 + 0.5;
		// get depth of current fragment from light's perspective
		float currentDepth = projCoords.z;


		vec3 lightDir = normalize(lights[index].m_Pos - worldPos);
		//float bias = max(0.01f * dot(normal, lightDir), 0.001f);
		float bias = 0.0f;
		bias = mix(0.00005f, 0.0005f, max(dot(normal, lightDir), 0.0f));

		//PCF
		float shadow = 0.0;
		vec2 texelSize = 1.0 / textureSize(lights[index].m_2DShadowMap, 0);
		for(int x = -1; x <= 1; ++x)
		{
			for(int y = -1; y <= 1; ++y)
			{
				float pcfDepth = texture(lights[index].m_2DShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
				shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
			}
		}
		shadow /= 9.0;

		//float shadow = currentDepth - bias > texture(lights[index].m_ShadowMap, projCoords.xy).r ? 1.0 : 0.0;

		// keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
		if(projCoords.z > 1.0)
			shadow = 0.0;

		return shadow;
	}
	else if(lights[index].m_Type == POINT_LIGHT)*/
	{
		vec3 fragToLight = worldPos - lights[index].m_Pos;
		float currentDepth = length(fragToLight) / lights[index].m_FarPlane;


		vec3 lightDir = normalize(lights[index].m_Pos - worldPos);
		//float bias = max(0.01f * dot(normal, lightDir), 0.001f);
		float bias = mix(0.001f, 0.01f, max(dot(normal, lightDir), 0.0f));

		//PCF
		float shadow = 0.0;
		float diskRadius = (1.0 + (length(camPos - worldPos) / lights[index].m_FarPlane)) / 25.0;
		for(float r = 0.0f; r < 9.0f; r++)
		{
			float x = sin(r / 9 * 2 * Pi / 3);
			float y = sin(r / 9 * 4 * Pi / 3);
			float z = sin(r / 9 * 6 * Pi / 3);
			float pcfDepth = texture(lights[index].m_CubeShadowMap, fragToLight + vec3(x, y, z) * diskRadius).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
		shadow /= 9.0;

		//float shadow = currentDepth - bias > texture(lights[index].m_ShadowMap, projCoords.xy).r ? 1.0 : 0.0;

		// keep the shadow at 0.0 when outside the farPlane region of the light's frustum.
		if(length(fragToLight / lights[index].m_FarPlane) > 1.0)
			shadow = 0.0;

		return shadow;
	}
	return 0.0f;
}

void main()
{
	//retrieves material properties
	vec3 albedo = pow(texture(albedoMap, texCoords).rgb, vec3(2.2f));
	float metallic = texture(metallicMap, texCoords).r;
	float roughness = texture(roughnessMap, texCoords).r;
	float ao = texture(aoMap, texCoords).r;

	vec3 N = normal; getNormalFromMap();
	vec3 V = normalize(camPos - worldPos);
	vec3 R = reflect(-V, N);

	//uses F0 = 0.04f if material is a di-electric (non-metal) and uses F0 = albedo color if material is a conductor (metal).
	//the mix function linearly interpolates between 0.04 and albedo color according to the metallic value of the material
	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedo, metallic);

	vec3 Lo = vec3(0.0f);
	for(int i = 0; i < 4; i++)
	{
		//regular per light calculations
		vec3 L = normalize(lightPositions[i] - worldPos);
		vec3 H = normalize(V + L);
		float distance = length(lightPositions[i] - worldPos);
		float attenuation = 1.0f / (1 + distance + distance * distance);
		vec3 radiance = lightColors[i] * attenuation;

		//Cook-Torrance BRDF
		float NDF = distributionGGX(N, H, roughness);
		float G = geometrySmith(N, V, L, roughness);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

		vec3 numerator = NDF * G * F;
		float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f;//we add this negligible value at the end to prevent divide by zero errors
		vec3 specular = numerator / denominator;

		//kS is equal to the fresnel value
		vec3 kS = F;
		//to keep lighting energy conservative, we must make sure that both the specular(kS) and diffuse(kD) parts equal 1.0f
		vec3 kD = vec3(1.0f) - kS;
		//the more metallic the material, the less it diffuses light
		kD *= 1.0f - metallic;

		float NdotL = max(dot(N, L), 0.0f);

		Lo += (kD * albedo / Pi + specular) * radiance * NdotL;//note: the BRDF part has already been multiplied by the fresnel value (kS) so doesn't need to be multiplied by kS again
	}

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
	float shadow = 0.0f;
	shadow += calculateShadow(0);
	shadow += calculateShadow(1);
	shadow += calculateShadow(2);
	shadow += calculateShadow(3);
	shadow *= 0.25f;
	shadow = smoothstep(0.0f, 1.0f, shadow);
	vec3 color = ambient + Lo * (1.0f - shadow);
	//vec3 color = vec3(1.0f - shadow);

	//HDR + Gamma correction
	color = color / (color + vec3(1.0f));
	color = pow(color, vec3(1.0f / 2.2f));

	FragColor = vec4(color, 1.0f);
}