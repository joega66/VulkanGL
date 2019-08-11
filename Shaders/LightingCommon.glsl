const float PI = 3.14159265;
const float Ambient = 0.1f;

struct PointLight
{
	vec3 Position;
	float Intensity;
	vec3 Color;
	float Range;
};

layout(std430) buffer LightBuffer
{
	PointLight PointLights[];
};

vec3 PointLighting(in MaterialParams Material, vec3 V, vec3 R0)
{
	vec3 Lo = vec3(0.0);

	for (int i = 0; i < PointLights.length(); i++)
	{
		vec3 FragToLight = PointLights[i].Position - Material.Position;
		vec3 L = normalize(FragToLight);
		vec3 H = normalize(L + V);
		float Diffuse = max(dot(Material.Normal, L), 0.0);
		float Distance = length(FragToLight);
		float Attenuation = 1.0 / (Distance * Distance);
		float Specular = pow(max(dot(Material.Normal, H), 0.0), Material.Metallic);
		// Radiance factor as derived in Real-Time Rendering, 3rd Edition, Section 5.5.1
		float Radiance = ((1 / PI) + ((Material.Metallic + 8) * (Specular)) / (8 * PI)) * Diffuse;
		Lo += (Ambient + Radiance) * PointLights[i].Color * Material.Albedo * Attenuation;
	}

	return Lo;
}

vec4 Shade(vec3 ViewPosition, in MaterialParams Material)
{
	vec3 Lo = vec3(0.0);
	vec3 R0 = vec3(0.04);
	vec3 V = normalize(ViewPosition - Material.Position);

	R0 = mix(R0, Material.Albedo, Material.Metallic);

	Lo += PointLighting(Material, V, R0);

	// Gamma
	Lo = Lo / (Lo + vec3(1.0));
	Lo = pow(Lo, vec3(1.0 / 2.2));

	return vec4(Lo, 1.0);
}