#include "Transform.h"
#include <glm/gtx/euler_angles.hpp>

const glm::vec3 Transform::forward = glm::vec3(0.0f, 0.0f, 1.0f);

Transform::Transform(
	EntityManager& ecs,
	Entity owner,
	const glm::vec3& position,
	const glm::vec3& eulerAngles,
	const glm::vec3& scale
) : _Owner(owner)
{
	Translate(ecs, position);
	Rotate(ecs, eulerAngles);
	Scale(ecs, scale);
}

Transform::Transform(Transform&& other)
	: _Owner(other._Owner)
	, _Parent(other._Parent)
	, _Position(other._Position)
	, _Rotation(other._Rotation)
	, _Scale(other._Scale)
	, _LocalToWorld(std::move(other._LocalToWorld))
	, _Children(std::move(other._Children))
{
}

Transform& Transform::operator=(Transform&& other)
{
	_Owner = other._Owner;
	_Parent = other._Parent;
	_Position = other._Position;
	_Rotation = other._Rotation;
	_Scale = other._Scale;
	_LocalToWorld = std::move(other._LocalToWorld);
	_Children = std::move(other._Children);
	return *this;
}

glm::mat4 Transform::GetLocalToParent() const
{
	glm::mat4 localToParent = glm::translate(glm::mat4(), _Position);
	localToParent = localToParent * glm::mat4_cast(_Rotation);
	localToParent = glm::scale(localToParent, _Scale);
	return localToParent;
}

void Transform::Translate(EntityManager& ecs, const glm::vec3& position)
{
	_Position = position;
	Clean(ecs);
}

void Transform::Rotate(EntityManager& ecs, const glm::vec3& eulerAngles)
{
	_Rotation = glm::quat(eulerAngles);
	Clean(ecs);
}

void Transform::Rotate(EntityManager& ecs, float angle, const glm::vec3& axis)
{
	_Rotation = glm::angleAxis(angle, axis);
	Clean(ecs);
}

void Transform::Scale(EntityManager& ecs, const glm::vec3& scale)
{
	_Scale = scale;
	Clean(ecs);
}

void Transform::SetParent(EntityManager& ecs, Entity newParent)
{
	if (ecs.IsValid(_Parent))
	{
		Transform& parentTransform = ecs.GetComponent<Transform>(_Parent);
		parentTransform.RemoveChild(_Owner);
	}

	Transform& parentTransform = ecs.GetComponent<Transform>(newParent);
	parentTransform.AddChild(_Owner);

	Clean(ecs);
}

void Transform::AddChild(Entity child)
{
	_Children.push_back(child);
}

void Transform::RemoveChild(Entity child)
{
	if (std::find(_Children.begin(), _Children.end(), child) != _Children.end())
	{
		_Children.remove(child);
	}
}

void Transform::Clean(EntityManager& ecs)
{
	_LocalToWorld = GetLocalToParent();

	if (ecs.IsValid(_Parent))
	{
		const Transform& parentTransform = ecs.GetComponent<Transform>(_Parent);
		_LocalToWorld = parentTransform.GetLocalToWorld() * _LocalToWorld;
	}

	std::for_each(_Children.begin(), _Children.end(), [&] (Entity child) 
	{
		Transform& childTransform = ecs.GetComponent<Transform>(child);
		childTransform.Clean(ecs);
	});
}