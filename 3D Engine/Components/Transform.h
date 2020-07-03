#pragma once
#include <ECS/EntityManager.h>

class Transform : public Component
{
public:
	Transform(
		EntityManager& ecs,
		Entity Owner,
		const glm::vec3& Position = glm::vec3(0.0f), 
		const glm::vec3& Rotation = glm::vec3(0.0f, 1.0f, 0.0), 
		float Angle = 0.0f, 
		const glm::vec3& Scale = glm::vec3(1.0f)
	);

	Transform(Transform&&);

	Transform& operator=(Transform&& Transform);
	
	const glm::mat4& GetLocalToWorld() const;

	void Translate(EntityManager& ecs, const glm::vec3& Position);

	void Rotate(EntityManager& ecs, const glm::vec3& Axis, float Angle);

	void Scale(EntityManager& ecs, const glm::vec3& ScaleBy);

	void SetParent(EntityManager& ecs, Entity Parent);

	void RemoveChild(Entity Child);

	inline const glm::vec3& GetPosition() const { return Position; }
	inline const glm::vec3& GetRotation() const { return Rotation; }
	inline const glm::vec3& GetScale() const { return ScaleBy; }
	inline float GetAngle() const { return Angle; }

	void Clean(EntityManager& ecs);

private:
	/** The owning entity. */
	Entity Owner = {};

	/** The parent entity. */
	Entity Parent = Entity();

	/** Position. */
	glm::vec3 Position = glm::vec3(0.0f);

	/** Rotational axis. */
	glm::vec3 Rotation = glm::vec3(0.0f, 1.0f, 0.0);

	/** Amount (in degrees) to rotate. */
	float Angle = 0.0f;
	
	/** How much to scale by. */
	glm::vec3 ScaleBy = glm::vec3(1.0f);

	/** Cached local to world. */
	glm::mat4 LocalToWorld;

	/** Child entities of this transform. */
	std::list<Entity> Children;

	void AddChild(Entity Child);

	glm::mat4 GetLocalToParent();
};