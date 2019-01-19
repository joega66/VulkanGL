const int MAX_MARCHING_STEPS = 64;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float EPSILON = 0.0001;

/* @start CSG boolean operations */

float UnionSDF(float d1, float d2)
{
	return min(d1, d2);
}

float DifferenceSDF(float d1, float d2)
{
	return max(-d2, d1);
}

/* @end */

/* @start Primitives SDF functions */

float PlaneSDF(vec3 P)
{
	return P.y;
}

float SphereSDF(vec3 P, float S)
{
	return length(P) - S;
}

float RectangleSDF(vec3 P, vec3 B)
{
	vec3 D = abs(P) - B;
	return length(max(D, 0.0)) + min(max(D.x, max(D.y, D.z)), 0.0);
}

float RoundBoxSDF(vec3 P, vec3 B, float R)
{
	vec3 Q = abs(P) - B;
	return min(max(Q.x, max(Q.y, Q.z)), 0.0) + length(max(Q, 0.0)) - R;
}

/* @end */

float SceneSDF(vec3 P)
{
	/*float Res = DifferenceSDF(RoundBoxSDF(P, vec3(0.15), 0.05), SphereSDF(P, 0.25));*/
	float Res = RoundBoxSDF(P, vec3(0.15), 0.05);
	return Res;
}

float RayMarch(vec3 RayOrigin, vec3 RayDir, float Start, float End)
{
	float TotalDistance = Start;
	for (int i = 0; i < MAX_MARCHING_STEPS; i++)
	{
		float Distance = SceneSDF(RayOrigin + TotalDistance * RayDir);
		if (Distance < EPSILON)
		{
			return TotalDistance;
		}
		TotalDistance += Distance;
		if (TotalDistance >= End)
		{
			return End;
		}
	}
	return End;
}

vec3 CalcNormalSDF(vec3 P)
{
	const float h = 0.0001;
	const vec2 k = vec2(1, -1);
	return normalize(
		k.xyy * SceneSDF(P + k.xyy*h) +
		k.yyx * SceneSDF(P + k.yyx*h) +
		k.yxy * SceneSDF(P + k.yxy*h) +
		k.xxx * SceneSDF(P + k.xxx*h));
}

vec3 RayDirection(float AspectRatio, float FieldOfView, vec2 UV)
{
	return normalize(vec3((2.0 * UV - 1.0) * vec2(AspectRatio, -1.0) * tan(radians(FieldOfView) / 2), -1.0));
}