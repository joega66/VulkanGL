#include "CameraCommon.glsl"
#define CUBEMAP_SET 1
#define SAMPLER_SET 2
#include "SceneResources.glsl"

const float INFINITY = 1.0f / 0.0f;

float length2( vec3 x )
{
	return dot( x, x );
}

uint WangHash( inout uint seed )
{
	seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> 4);
	seed *= uint(0x27d4eb2d);
	seed = seed ^ (seed >> 15);
	return seed;
}

/** Generate a random float [0...1] */
float RandomFloat( inout uint seed )
{
	return float( WangHash( seed ) ) / 4294967296.0;
}

vec3 RandomCosineDirection( inout uint seed )
{
	const float r1 = RandomFloat( seed );
	const float r2 = RandomFloat( seed );
	const float z = sqrt( 1 - r2 );

	const float phi = 2 * PI * r1;
	const float x = cos( phi ) * sqrt( r2 );
	const float y = sin( phi ) * sqrt( r2 );

	return vec3(x, y, z);
}

struct Ray
{
	vec3 origin;
	vec3 direction;
};

vec3 Ray_At(Ray ray, float t)
{
	return ray.origin + t * ray.direction;
}

struct HitRecord
{
	vec3 p;
	float t;
	vec3 normal;
	bool frontFace;
	vec3 albedo;
};

void HitRecord_SetFaceNormal(inout HitRecord rec, Ray ray, vec3 outwardNormal)
{
	rec.frontFace = dot(ray.direction, outwardNormal) < 0;
	rec.normal = rec.frontFace ? outwardNormal : -outwardNormal;
}

struct Sphere
{
	vec3 center;
	float radius;
	vec3 albedo;
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
			rec.albedo = sphere.albedo;
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
			rec.albedo = sphere.albedo;
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
	vec3 albedo;
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
	rec.albedo = rect.albedo;
	//rec.uv = vec2((x - rect.x0) / (rect.x1 - rect.x0), (y - rect.y0) / (rect.y1 - rect.y0));
	return true;
}

struct YZRect
{
	float y0, y1, z0, z1, k;
	vec3 albedo;
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
	rec.albedo = rect.albedo;
	//rec.uv = vec2((y - rect.y0) / (rect.y1 - rect.y0), (z - rect.z0) / (rect.z1 - rect.z0));
	return true;
}

struct XZRect
{
	float x0, x1, z0, z1, k;
	vec3 albedo;
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
	rec.albedo = rect.albedo;
	//rec.uv = glm::dvec2((x - rect.x0) / (rect.x1 - rect.x0), (z - rect.z0) / (rect.z1 - rect.z0));
	return true;
}

struct ONB
{
	vec3 axis[3];
};

void ONB_BuildFromW( inout ONB onb, vec3 n )
{
	onb.axis[2] = normalize(n);
	const vec3 a = (abs(onb.axis[2].x) > 0.9) ? vec3(0, 1, 0) : vec3(1, 0, 0);
	onb.axis[1] = normalize(cross(onb.axis[2], a));
	onb.axis[0] = cross(onb.axis[2], onb.axis[1]);
}

vec3 ONB_Transform( inout ONB onb, vec3 p )
{
	return p.x * onb.axis[0] + p.y * onb.axis[1] + p.z * onb.axis[2];
}

const int NUM_SPHERES = 2;
Sphere spheres[NUM_SPHERES] = { { vec3(0, 0, -1), 0.5, vec3(0.5) }, { vec3(0, -100.5, -1), 100, vec3(0.5) } };

const int NUM_YZ_RECTS = 2;
YZRect yzRects[NUM_YZ_RECTS] = { { 0, 555, 0, 555, 555, vec3(.12, .45, .15) }, { 0, 555, 0, 555, 0, vec3(.65, .05, .05) } };

const int NUM_XZ_RECTS = 2;
XZRect xzRects[NUM_XZ_RECTS] = { { 0, 555, 0, 555, 0, vec3(.73) }, { 0, 555, 0, 555, 555, vec3(.73) } };

XYRect xyRect = { 0, 555, 0, 555, 555, vec3(0.73) };

bool TraceRay(Ray ray, inout HitRecord rec)
{
	HitRecord tempRec;
	bool hitAnything = false;
	float closestSoFar = INFINITY;

	for (int i = 0; i < NUM_SPHERES; i++)
	{
		if (Sphere_Hit(spheres[i], ray, 0.001, closestSoFar, tempRec))
		{
			closestSoFar = tempRec.t;
			rec = tempRec;
		}
	}

	for (int i = 0; i < NUM_YZ_RECTS; i++)
	{
		if (YZRect_Hit(yzRects[i], ray, 0.001, closestSoFar, tempRec))
		{
			closestSoFar = tempRec.t;
			rec = tempRec;
		}
	}

	for (int i = 0; i < NUM_XZ_RECTS; i++)
	{
		if (XZRect_Hit(xzRects[i], ray, 0.001, closestSoFar, tempRec))
		{
			closestSoFar = tempRec.t;
			rec = tempRec;
		}
	}

	if (XYRect_Hit(xyRect, ray, 0.001, closestSoFar, tempRec))
	{
		closestSoFar = tempRec.t;
		rec = tempRec;
	}

	return closestSoFar != INFINITY;
}

float CosinePDF(vec3 normal, vec3 direction)
{
	const float cosine = dot(normal, normalize(direction));
	return cosine < 0 ? 0 : cosine / PI;
}

float Lambertian_ScatteringPDF(Ray ray, HitRecord rec, Ray scattered)
{
	return CosinePDF(rec.normal, scattered.direction);
}

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	const ivec2 sceneColorSize = imageSize( SceneColor );
	if ( any( greaterThanEqual( gl_GlobalInvocationID.xy, sceneColorSize.xy ) ) )
		return;

	const ivec2 screenCoords = ivec2(gl_GlobalInvocationID.xy);
	
	uint seed = uint(uint(screenCoords.x) * uint(1973) + uint(screenCoords.y) * uint(9277) + _FrameNumber * uint(26699)) | uint(1);

	vec3 color = vec3(1);

	const float s = ( float(screenCoords.x) /*+ RandomDouble()*/ ) / ( float(sceneColorSize.x) );
	const float t = 1.0 - ( float(screenCoords.y) /*+ RandomDouble()*/ ) / ( float(sceneColorSize.y) );

	Ray ray;
	ray.origin = _Origin.xyz;
	ray.direction = _LowerLeftCorner.xyz + s * _Horizontal.xyz + t * _Vertical.xyz - _Origin.xyz;

	const int rayDepth = 8;

	for (int i = 0; i < rayDepth; i++)
	{
		HitRecord rec;
		if ( TraceRay( ray, rec ) )
		{
			// Lambertian Scatter
			ONB onb;
			ONB_BuildFromW(onb, rec.normal);

			Ray scattered;
			scattered.origin = rec.p;
			scattered.direction = ONB_Transform(onb, RandomCosineDirection(seed));
			
			const float pdf = CosinePDF(rec.normal, scattered.direction);

			// Attenuate
			color *= rec.albedo/* * Lambertian_ScatteringPDF(ray, rec, scattered) / pdf*/;

			ray = scattered;
		}
		else
		{
			color *= SampleCubemap(_Skybox, _SkyboxSampler, ray.direction).rgb;
			break;
		}
	}

	if (_FrameNumber < 1024) // Prevents color banding
	{
		const vec4 prevFrameColor = imageLoad(SceneColor, screenCoords);
		const float blend = (_FrameNumber == 0) ? 1.0f : (1.0f / (1.0f + (1.0f / prevFrameColor.a)));
		color = mix(prevFrameColor.rgb, color, blend);
		imageStore(SceneColor, screenCoords, vec4(color, blend));
	}
}