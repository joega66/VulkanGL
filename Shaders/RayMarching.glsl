#include "View.glsl"

const int MAX_MARCHING_STEPS = 255;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float EPSILON = 0.0001;

float SphereSDF(vec3 SamplePoint)
{
	return length(SamplePoint) - 1.0;
}

float SceneSDF(vec3 SamplePoint) 
{
	return SphereSDF(SamplePoint);
}

// Ray March
float ShortestDistanceToSurface(vec3 Eye, vec3 MarchingDirection, float Start, float End) 
{
    float TotalDistance = Start;
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) 
	{
        float Dist = SceneSDF(Eye + TotalDistance * MarchingDirection);
        if (Dist < EPSILON) 
		{
			return TotalDistance;
        }
        TotalDistance += Dist;
        if (TotalDistance >= End) 
		{
            return End;
        }
    }
    return End;
}

vec3 RayDirection(float AspectRatio, vec2 UV) 
{
    return normalize(vec3((2.0 * UV - 1.0) * vec2(AspectRatio, 1.0), -1.0));
}

layout(location = 0) out vec4 OutColor;
layout(location = 0) in vec2 InUV;

void main()
{
	vec3 Dir = RayDirection(View.AspectRatio, InUV);
	vec3 Eye = View.Position;

	// @todo
	Dir = vec3(inverse(mat3(View.View)) * Dir);
	Eye = vec3(inverse(View.View) * vec4(Eye, 1.0f));

    float Dist = ShortestDistanceToSurface(Eye, Dir, MIN_DIST, MAX_DIST);
    
    if (Dist > MAX_DIST - EPSILON) {
        // Didn't hit anything
        OutColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
	else
	{
		OutColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
}