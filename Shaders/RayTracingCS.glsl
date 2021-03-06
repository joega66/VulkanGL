#define CAMERA_SET 0
#include "CameraCommon.glsl"
#define CUBEMAP_SET 1
#include "SceneResources.glsl"
#include "RayTracingCommon.glsl"
#include "LightingCommon.glsl"
#include "Common.glsl"

const float INFINITY = 1.0f / 0.0f;
const int MAX_RAY_DEPTH = 8;
const int MAX_TEMPORAL_SAMPLES = 1024;

layout(push_constant) uniform Params { RayTracingParams _Params; };

struct Ray
{
	vec3 origin;
	vec3 direction;
};

vec3 Ray_At(Ray ray, float t)
{
	return ray.origin + t * ray.direction;
}

#define MAT_LAMBERTIAN	1
#define MAT_GGX			2
#define MAT_DIELECTRIC	3

struct Material
{
	vec3 albedo;
	float roughness;
	float metallic;
	float refractiveIndex;
	bool isEmitter;
	int type;
};

struct HitRecord
{
	vec3 p;
	float t;
	vec3 normal;
	bool frontFace;
};

HitRecord HitRecord_Init()
{
	HitRecord rec;
	rec.p = vec3(0);
	rec.t = 0;
	rec.normal = vec3(0);
	rec.frontFace = false;
	return rec;
}

void HitRecord_SetFaceNormal(inout HitRecord rec, Ray ray, vec3 outwardNormal)
{
	rec.frontFace = dot(ray.direction, outwardNormal) < 0;
	rec.normal = rec.frontFace ? normalize(outwardNormal) : normalize(-outwardNormal);
}

struct Sphere
{
	vec3 center;
	float radius;
};

bool Sphere_Hit(Sphere sphere, Ray ray, float tMin, float tMax, inout HitRecord rec)
{
	const vec3 oc = ray.origin - sphere.center;
	const float a = length2(ray.direction);
	const float halfB = dot(oc, ray.direction);
	const float c = length2(oc) - sphere.radius * sphere.radius;
	const float discriminant = halfB * halfB - a * c;

	if (discriminant > 0)
	{
		float root = sqrt(discriminant);
		float temp = (-halfB - root) / a;
		if (temp < tMax && temp > tMin)
		{
			rec.t = temp;
			rec.p = Ray_At(ray, rec.t);
			vec3 outwardNormal = (rec.p - sphere.center) / sphere.radius;
			HitRecord_SetFaceNormal(rec, ray, outwardNormal);
			/*rec.mat = _Material;
			rec.uv = GetSphereUV(rec.p);*/
			return true;
		}
		temp = (-halfB + root) / a;
		if (temp < tMax && temp > tMin)
		{
			rec.t = temp;
			rec.p = Ray_At(ray, rec.t);
			vec3 outwardNormal = (rec.p - sphere.center) / sphere.radius;
			HitRecord_SetFaceNormal(rec, ray, outwardNormal);
			/*rec.mat = _Material;
			rec.uv = GetSphereUV(rec.p);*/
			return true;
		}
	}
	return false;
}

struct XYRect
{
	float x0, x1, y0, y1, k;
};

bool XYRect_Hit(XYRect rect, Ray r, float tMin, float tMax, inout HitRecord rec)
{
	float t = (rect.k - r.origin.z) / r.direction.z;
	if (t < tMin || t > tMax)
		return false;
	float x = r.origin.x + t * r.direction.x;
	float y = r.origin.y + t * r.direction.y;
	if (x < rect.x0 || x > rect.x1 || y < rect.y0 || y > rect.y1)
		return false;
	rec.t = t;
	HitRecord_SetFaceNormal(rec, r, vec3(0, 0, 1));
	rec.p = Ray_At(r, t);
	//rec.uv = vec2((x - rect.x0) / (rect.x1 - rect.x0), (y - rect.y0) / (rect.y1 - rect.y0));
	return true;
}

struct YZRect
{
	float y0, y1, z0, z1, k;
};

bool YZRect_Hit(YZRect rect, Ray r, float tMin, float tMax, inout HitRecord rec)
{
	float t = (rect.k - r.origin.x) / r.direction.x;
	if (t < tMin || t > tMax)
		return false;
	float y = r.origin.y + t * r.direction.y;
	float z = r.origin.z + t * r.direction.z;
	if (y < rect.y0 || y > rect.y1 || z < rect.z0 || z > rect.z1)
		return false;
	rec.t = t;
	rec.p = Ray_At(r, t);
	HitRecord_SetFaceNormal(rec, r, vec3(1, 0, 0));
	//rec.uv = vec2((y - rect.y0) / (rect.y1 - rect.y0), (z - rect.z0) / (rect.z1 - rect.z0));
	return true;
}

struct XZRect
{
	float x0, x1, z0, z1, k;
};

bool XZRect_Hit(XZRect rect, Ray r, float tMin, float tMax, inout HitRecord rec)
{
	float t = (rect.k - r.origin.y) / r.direction.y;
	if (t < tMin || t > tMax)
		return false;
	float x = r.origin.x + t * r.direction.x;
	float z = r.origin.z + t * r.direction.z;
	if (x < rect.x0 || x > rect.x1 || z < rect.z0 || z > rect.z1)
		return false;
	rec.t = t;
	rec.p = Ray_At(r, t);
	HitRecord_SetFaceNormal(rec, r, vec3(0, 1, 0));
	//rec.uv = glm::dvec2((x - rect.x0) / (rect.x1 - rect.x0), (z - rect.z0) / (rect.z1 - rect.z0));
	return true;
}

vec3 XZRect_Random(XZRect rect, vec3 origin)
{
	vec3 randomPoint = vec3( RandomFloat(rect.x0, rect.x1), rect.k, RandomFloat(rect.z0, rect.z1) );
	return randomPoint - origin;
}

float XZRect_PDF(XZRect rect, vec3 origin, vec3 direction)
{
	HitRecord rec;

	if (!XZRect_Hit(rect, Ray(origin, direction), 0.001, INFINITY, rec))
		return 0;

	float area = (rect.x1 - rect.x0) * (rect.z1 - rect.z0);
	float distanceSquared = rec.t * rec.t * length2(direction);
	float cosine = abs(dot(direction, rec.normal) / length(direction));

	return distanceSquared / (cosine * area);
}

/** @begin Scene */

XZRect lightSource = { 213, 343, 227, 332, 554 };

Sphere spheres[] = 
{ 
	{ vec3(277.5, 100, 277.5), 100 },
};

Material sphereMaterials[] =
{
	{ vec3(1.0), 0.25, 0.99, 1.1, false, MAT_LAMBERTIAN },
};

YZRect yzRects[] =
{ 
	{ 0, 555, 0, 555, 555 }, 
	{ 0, 555, 0, 555, 0 } 
};

Material yzRectMaterials[] =
{
	{ vec3(.12, .45, .15), 1.0, 0.0, 0, false, MAT_LAMBERTIAN },
	{ vec3(.65, .05, .05), 1.0, 0.0, 0, false, MAT_LAMBERTIAN },
};

XZRect xzRects[] = 
{
	{ 0, 555, 0, 555, 0 }, 
	{ 0, 555, 0, 555, 555 }, 
	lightSource
};

Material xzRectMaterials[] =
{
	{ vec3(.73), 1.0, 0.0, 0, false, MAT_LAMBERTIAN },
	{ vec3(.73), 1.0, 0.0, 0, false, MAT_LAMBERTIAN },
	{ vec3(150.), 1.0, 0.0, 0, true, MAT_LAMBERTIAN },
};

XYRect xyRects[] = 
{ 
	{ 0, 555, 0, 555, 555 } 
};

Material xyRectMaterials[] =
{
	{ vec3(.73), 1.0, 0.0, 0, false, MAT_LAMBERTIAN },
};

/** @end Scene */

bool TraceRay(Ray ray, inout HitRecord rec, inout Material mat)
{
	HitRecord tempRec = HitRecord_Init();
	bool hitAnything = false;
	float closestSoFar = INFINITY;

	for (int i = 0; i < spheres.length(); i++)
	{
		if (Sphere_Hit(spheres[i], ray, 0.001, closestSoFar, tempRec))
		{
			closestSoFar = tempRec.t;
			rec = tempRec;
			mat = sphereMaterials[i];
		}
	}

	for (int i = 0; i < yzRects.length(); i++)
	{
		if (YZRect_Hit(yzRects[i], ray, 0.001, closestSoFar, tempRec))
		{
			closestSoFar = tempRec.t;
			rec = tempRec;
			mat = yzRectMaterials[i];
		}
	}

	for (int i = 0; i < xzRects.length(); i++)
	{
		if (XZRect_Hit(xzRects[i], ray, 0.001, closestSoFar, tempRec))
		{
			closestSoFar = tempRec.t;
			rec = tempRec;
			mat = xzRectMaterials[i];
		}
	}

	for (int i = 0; i < xyRects.length(); i++)
	{
		if (XYRect_Hit(xyRects[i], ray, 0.001, closestSoFar, tempRec))
		{
			closestSoFar = tempRec.t;
			rec = tempRec;
			mat = xyRectMaterials[i];
		}
	}
	
	return closestSoFar != INFINITY;
}

vec3 RayColor(Ray ray)
{
	vec3 color = vec3(1);

	for ( int rayDepth = 0; rayDepth < MAX_RAY_DEPTH; rayDepth++ )
	{
		HitRecord rec = HitRecord_Init();
		Material mat;
		if ( !TraceRay(ray, rec, mat) )
		{
			color *= SampleCubemap(_Params._Skybox, ray.direction).rgb;
			//color *= vec3(0);
			return color;
		}

		if ( mat.isEmitter )
		{
			color *= mat.albedo;
			return color;
		}

		ONB onb;
		ONB_BuildFromW(onb, rec.normal);

		Ray scattered;
		scattered.origin = rec.p;
	
		float scatteringPDF;

		if (mat.type == MAT_LAMBERTIAN)
		{
			scattered.direction = ONB_Transform(onb, RandomCosineDirection());
			scatteringPDF = Lambertian_ScatteringPDF(rec.normal, scattered.direction);
		}
		else if (mat.type == MAT_GGX)
		{
			const float alpha = mat.roughness * mat.roughness;

			float cosH;
			const vec3 halfwayDir = ONB_Transform(onb, GGX_ImportanceSample(alpha, cosH));
			const vec3 viewDir = normalize(ray.origin - rec.p);
			const vec3 scatterDir = normalize(2 * dot(viewDir, halfwayDir) * halfwayDir - viewDir);

			scattered.direction = scatterDir;

			const float ndoth = max(dot(rec.normal, halfwayDir), 0);
			const float ndotv = max(dot(rec.normal, viewDir), 0);
			const float ndotl = max(dot(rec.normal, scatterDir), 0);
			const float vdoth = max(dot(viewDir, halfwayDir), 1e-6);

			const float ndf		= NormalGGX(ndoth, alpha);
			const float g		= SmithGF(ndotv, ndotl, alpha);
			const vec3 fresnel	= FresnelSchlick(mix(vec3(0.04), mat.albedo, mat.metallic), ndotl);
			const vec3 specular = (ndf * g * fresnel) / max(4.0 * ndotv * ndotl, 1e-6);

			mat.albedo = specular * ndotl;

			scatteringPDF = GGX_ScatteringPDF(ndf, vdoth, cosH);
		}
		else if (mat.type == MAT_DIELECTRIC)
		{
			const float eta = rec.frontFace ? 1.0 / mat.refractiveIndex : mat.refractiveIndex;
			const vec3 unitDirection = normalize(ray.direction);
			const float cosTheta = min(dot(-unitDirection, rec.normal), 1.0);
			const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

			if (eta * sinTheta > 1.0)
			{
				scattered.direction = reflect(unitDirection, rec.normal);
			}
			else
			{
				const float reflectChance = Schlick(cosTheta, eta);
				if (RandomFloat() < reflectChance)
				{
					scattered.direction = reflect(unitDirection, rec.normal);
				}
				else
				{
					scattered.direction = refract(unitDirection, rec.normal, eta);
				}
			}

			scatteringPDF = 1.0;
		}

		const vec3 threshold = vec3(1);

		// Clamp sample values to fight fireflies.
		color *= min( mat.albedo * scatteringPDF, threshold );

		ray = scattered;
	}

	return vec3(0);
}

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	const ivec2 sceneColorSize = imageSize(_SceneColor);
	if ( any( greaterThanEqual( gl_GlobalInvocationID.xy, sceneColorSize.xy ) ) )
		return;

	const ivec2 screenCoords = ivec2(gl_GlobalInvocationID.xy);
	
	seed = RandomInit(screenCoords, _Params._FrameNumber);

	vec3 color = vec3(0);

	{
		const float s = (float(screenCoords.x) + RandomFloat()) / (float(sceneColorSize.x));
		const float t = 1.0 - (float(screenCoords.y) + RandomFloat()) / (float(sceneColorSize.y));

		Ray ray;
		ray.origin = _Params._Origin.xyz;
		ray.direction = _Params._LowerLeftCorner.xyz + s * _Params._Horizontal.xyz + t * _Params._Vertical.xyz - _Params._Origin.xyz;

		color += RayColor(ray);
	}

	if (_Params._FrameNumber < MAX_TEMPORAL_SAMPLES) // Prevents color banding
	{
		const vec4 prevFrameColor = imageLoad(_SceneColor, screenCoords);
		const float blend = (_Params._FrameNumber == 0) ? 1.0f : (1.0f / (1.0f + (1.0f / prevFrameColor.a)));
		color = mix(prevFrameColor.rgb, color, blend);
		imageStore(_SceneColor, screenCoords, vec4(color, blend));
	}
}