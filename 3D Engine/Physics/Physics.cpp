#include "Physics.h"
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>

const Plane Plane::XY = Plane{ glm::vec3(0.0f, 0.0f, 1.0f), 0.0f };
const Plane Plane::YZ = Plane{ glm::vec3(1.0f, 0.0f, 0.0f), 0.0f };
const Plane Plane::XZ = Plane{ glm::vec3(0.0f, 1.0f, 0.0f), 0.0f };

bool Physics::Raycast(const Ray& Ray, Entity Entity, float& T)
{
	CStaticMesh& Mesh = Entity.GetComponent<CStaticMesh>();
	CTransform& Transform = Entity.GetComponent<CTransform>();
	BoundingBox& Bounds = Mesh.StaticMesh->Bounds;

	glm::vec4 Min = Transform.GetLocalToWorld() * glm::vec4(Bounds.Min, 1.0f);
	glm::vec4 Max = Transform.GetLocalToWorld() * glm::vec4(Bounds.Max, 1.0f);

	glm::vec3 DirInv = glm::vec3(
		1.0f / Ray.Direction.x,
		1.0f / Ray.Direction.y,
		1.0f / Ray.Direction.z
	);

	float T1 = (Min.x - Ray.Origin.x) * DirInv.x;
	float T2 = (Max.x - Ray.Origin.x) * DirInv.x;
	float T3 = (Min.y - Ray.Origin.y) * DirInv.y;
	float T4 = (Max.y - Ray.Origin.y) * DirInv.y;
	float T5 = (Min.z - Ray.Origin.z) * DirInv.z;
	float T6 = (Max.z - Ray.Origin.z) * DirInv.z;

	float TMin = std::max(std::max(std::min(T1, T2), std::min(T3, T4)), std::min(T5, T6));
	float TMax = std::min(std::min(std::max(T1, T2), std::max(T3, T4)), std::max(T5, T6));

	if (TMax < 0)
	{
		T = TMax;
		return false;
	}

	if (TMin > TMax)
	{
		T = TMax;
		return false;
	}

	T = TMin;
	return true;
}

bool Physics::Raycast(const Ray& Ray, Entity Entity)
{
	float T;
	return Raycast(Ray, Entity, T);

	/*glm::vec3 Diagonal = glm::vec3(Max - Min);
	Diagonal /= 2;

	glm::vec3 C = glm::vec3(Min) + Diagonal;
	float R = glm::length(Diagonal);
	float R_2 = R * R;
	glm::vec3 L = C - Ray.Origin;
	float S = glm::dot(L, Ray.Direction);
	float L_2 = glm::dot(L, L);

	if (S < 0 && L_2 > R_2)
	{
		return false;
	}

	float M_2 = L_2 - S * S;

	if (M_2 > R_2)
	{
		return false;
	}

	float Q = std::sqrt(R_2 - M_2);
	float T = L_2 > R_2 ? S - Q : S + Q;

	return true;*/
}

bool Physics::Raycast(const Ray& Ray, const Plane& Plane, float& T)
{
	check(Plane.Normal != glm::vec3(0.0f), "Plane should have non-zero normal.");

	glm::vec3 PointOnPlane = [&]()
	{
		if (Plane.Normal.x != 0.0f)
		{
			return glm::vec3(-Plane.Distance / Plane.Normal.x, 0.0f, 0.0f);
		}
		else if (Plane.Normal.y != 0.0f)
		{
			return glm::vec3(0.0f, -Plane.Distance / Plane.Normal.y, 0.0f);
		}
		else //Plane.Normal.z != 0.0f
		{
			return glm::vec3(0.0f, 0.0f, -Plane.Distance / Plane.Normal.z);
		}
	}();

	if (float Denom = glm::dot(Ray.Direction, Plane.Normal); Denom == 0.0f)
	{
		// Line and plane are parallel.
		if (glm::dot(PointOnPlane - Ray.Origin, Plane.Normal) == 0.0f)
		{
			// Line is contained in the plane.
			T = 0;
		}
		else
		{
			// No intersection.
			T = std::numeric_limits<float>().lowest();
		}
		return false;
	}
	else
	{
		// There is a single point of intersection.
		T = (glm::dot(PointOnPlane - Ray.Origin, Plane.Normal)) / Denom;
		return true;
	}
}
