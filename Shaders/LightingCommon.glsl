const float PI = 3.14159265;
const float Ambient = 0.01f;

struct LightParams
{
	vec3 L;
	float Intensity;
	vec3 Color;
};

vec3 DirectLighting(in vec3 V, in LightParams Light, in MaterialParams Material)
{
	vec3 H = normalize(Light.L + V);
	float Diffuse = max(dot(Material.Normal, Light.L), 0.0);
	float Specular = pow(max(dot(Material.Normal, H), 0.0), Material.Metallic);
	float Radiance = ((1 / PI) + ((Material.Metallic + 8) * (Specular)) / (8 * PI)) * Diffuse;
	vec3 Lo = (Ambient + Radiance) * Light.Intensity * Light.Color * Material.Albedo;
	return Lo;
}

vec4 Shade(in MaterialParams Material)
{
	vec3 Lo = vec3(0.0);
	vec3 V = normalize(View.Position - Material.Position);

	// Directional lights
	for (int LightIndex = 0; LightIndex < NumDirectionalLights.x; LightIndex++)
	{
		LightParams Light;
		Light.L = DirectionalLights[LightIndex].Direction;
		Light.Intensity = DirectionalLights[LightIndex].Intensity;
		Light.Color = DirectionalLights[LightIndex].Color;

		Lo += DirectLighting(V, Light, Material);
	}

	// Point lights
	for (int LightIndex = 0; LightIndex < NumPointLights.x; LightIndex++)
	{
		vec3 FragToLight = PointLights[LightIndex].Position - Material.Position;
		float Distance = length(FragToLight);
		float Attenuation = 1.0 / (Distance * Distance);

		LightParams Light;
		Light.L = normalize(FragToLight);
		Light.Intensity = PointLights[LightIndex].Intensity;
		Light.Color = PointLights[LightIndex].Color;

		Lo += DirectLighting(V, Light, Material) * Attenuation;
	}

	// Gamma
	Lo = Lo / (Lo + vec3(1.0));
	Lo = pow(Lo, vec3(1.0 / 2.2));

	return vec4(Lo, 1.0);
}