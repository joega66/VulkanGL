#include "Transform.h"

Transform::Transform(
	EntityManager& ecs,
	Entity Owner,
	const glm::vec3& Position, 
	const glm::vec3& Rotation, 
	float Angle, 
	const glm::vec3& InScale
) : Owner(Owner)
{
	Translate(ecs, Position);
	Rotate(ecs, Rotation, Angle);
	Scale(ecs, InScale);
}

Transform::Transform(Transform&& Other)
	: Owner(Other.Owner)
	, Parent(Other.Parent)
	, Position(Other.Position)
	, Rotation(Other.Rotation)
	, Angle(Other.Angle)
	, ScaleBy(Other.ScaleBy)
	, LocalToWorld(std::move(Other.LocalToWorld))
	, Children(std::move(Other.Children))
{
}

Transform& Transform::operator=(Transform&& Other)
{
	Owner = Other.Owner;
	Parent = Other.Parent;
	Position = Other.Position;
	Rotation = Other.Rotation;
	Angle = Other.Angle;
	ScaleBy = Other.ScaleBy;
	LocalToWorld = std::move(Other.LocalToWorld);
	Children = std::move(Other.Children);
	return *this;
}

const glm::mat4& Transform::GetLocalToWorld() const
{
	return LocalToWorld;
}

glm::mat4 Transform::GetLocalToParent()
{
	glm::mat4 LocalToParent = glm::translate(glm::mat4(), Position);
	LocalToParent = glm::rotate(LocalToParent, glm::radians(Angle), Rotation);
	LocalToParent = glm::scale(LocalToParent, ScaleBy);

	return LocalToParent;
}

void Transform::Translate(EntityManager& ecs, const glm::vec3& InPosition)
{
	Position = InPosition;
	Clean(ecs);
}

void Transform::Rotate(EntityManager& ecs, const glm::vec3& InRotation, float InAngle)
{
	Rotation = InRotation;
	Angle = InAngle;
	Clean(ecs);
}

void Transform::Scale(EntityManager& ecs, const glm::vec3& InScale)
{
	ScaleBy = InScale;
	Clean(ecs);
}

void Transform::SetParent(EntityManager& ecs, Entity NewParent)
{
	if (ecs.IsValid(Parent))
	{
		Transform& ParentTransform = ecs.GetComponent<Transform>(Parent);
		ParentTransform.RemoveChild(Owner);
	}

	Transform& ParentTransform = ecs.GetComponent<Transform>(NewParent);
	ParentTransform.AddChild(Owner);

	Clean(ecs);
}

void Transform::AddChild(Entity Child)
{
	Children.push_back(Child);
}

void Transform::RemoveChild(Entity Child)
{
	if (std::find(Children.begin(), Children.end(), Child) != Children.end())
	{
		Children.remove(Child);
	}
}

void Transform::Clean(EntityManager& ecs)
{
	LocalToWorld = GetLocalToParent();

	if (ecs.IsValid(Parent))
	{
		const Transform& ParentTransform = ecs.GetComponent<Transform>(Parent);
		LocalToWorld = ParentTransform.GetLocalToWorld() * LocalToWorld;
	}

	std::for_each(Children.begin(), Children.end(), [&] (Entity Child) 
	{
		Transform& ChildTransform = ecs.GetComponent<Transform>(Child);
		ChildTransform.Clean(ecs);
	});
}