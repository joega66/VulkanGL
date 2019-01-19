const float PI = 3.14159265;

const float Ambient = 0.1;
const float Metallic = 1.0;

struct PointLight
{
	vec3 Position;
	float Intensity;
	vec3 Color;
	float Range;
};

layout(std430) buffer LightBuffer
{
	//int NumPointLights;
	PointLight PointLights[];
};

vec3 PointLighting(vec3 FragPosition, vec3 Normal, vec3 V, vec3 R0, vec3 Albedo)
{
	vec3 Lo = vec3(0.0);

	for (int i = 0; i < PointLights.length(); i++)
	{
		vec3 FragToLight = PointLights[i].Position - FragPosition;
		vec3 L = normalize(FragToLight);
		vec3 H = normalize(L + V);
		float Diffuse = max(dot(Normal, L), 0.0);
		float Distance = length(FragToLight);
		float Attenuation = 1.0 / (Distance * Distance);
		float Specular = pow(max(dot(Normal, H), 0.0), Metallic);
		// Radiance factor as derived in Real-Time Rendering, 3rd Edition, Section 5.5.1
		float Radiance = ((1 / PI) + ((Metallic + 8) * (Specular)) / (8 * PI)) * Diffuse;
		Lo += (Ambient + Radiance) * PointLights[i].Color * Albedo * Attenuation;
	}

	return Lo;
}