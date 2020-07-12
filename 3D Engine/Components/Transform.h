#pragma once
#include <ECS/EntityManager.h>
#include <glm/gtx/quaternion.hpp>

class Transform : public Component
{
public:
	static const glm::vec3 forward;

	Transform(
		EntityManager& ecs,
		Entity owner,
		const glm::vec3& position = glm::vec3(0.0f),
		const glm::vec3& eulerAngles = glm::vec3(0.0f, 0.0f, 0.0),
		const glm::vec3& scale = glm::vec3(1.0f));

	Transform(Transform&&);

	Transform& operator=(Transform&& other);
	
	const glm::mat4& GetLocalToWorld() const;

	void Translate(EntityManager& ecs, const glm::vec3& position);

	void Rotate(EntityManager& ecs, const glm::vec3& eulerAngles);

	void Rotate(EntityManager& ecs, float angle, const glm::vec3& axis);

	void Scale(EntityManager& ecs, const glm::vec3& scale);

	void SetParent(EntityManager& ecs, Entity parent);

	void RemoveChild(Entity child);

	inline const glm::vec3& GetPosition() const { return _Position; }
	inline const glm::quat& GetRotation() const { return _Rotation; }
	inline const glm::vec3& GetScale() const { return _Scale; }
	inline const glm::vec3 GetEulerAngles() const { return glm::eulerAngles(_Rotation); }
	inline const glm::vec3 GetForward() const { return glm::normalize( _Rotation * forward ); }

	void Clean(EntityManager& ecs);

private:
	/** The owning entity. */
	Entity _Owner = {};

	/** The parent entity. */
	Entity _Parent = {};

	/** Position. */
	glm::vec3 _Position = glm::vec3(0.0f);

	/** Rotation. */
	glm::quat _Rotation;

	/** Scale. */
	glm::vec3 _Scale = glm::vec3(1.0f);

	/** Cached local to world. */
	glm::mat4 _LocalToWorld;

	/** Child entities of this transform. */
	std::list<Entity> _Children;

	void AddChild(Entity child);

	glm::mat4 GetLocalToParent() const;
};