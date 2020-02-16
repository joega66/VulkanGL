#include "Transform.h"
#include "DRM.h"

Transform::Transform(
	EntityManager& ECS,
	Entity Owner,
	const glm::vec3& Position, 
	const glm::vec3& Rotation, 
	float Angle, 
	const glm::vec3& InScale
) : Owner(Owner)
{
	Translate(ECS, Position);
	Rotate(ECS, Rotation, Angle);
	Scale(ECS, InScale);
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

void Transform::Translate(EntityManager& ECS, const glm::vec3& InPosition)
{
	Position = InPosition;
	Clean(ECS);
}

void Transform::Rotate(EntityManager& ECS, const glm::vec3& InRotation, float InAngle)
{
	Rotation = InRotation;
	Angle = InAngle;
	Clean(ECS);
}

void Transform::Scale(EntityManager& ECS, const glm::vec3& InScale)
{
	ScaleBy = InScale;
	Clean(ECS);
}

void Transform::SetParent(EntityManager& ECS, Entity NewParent)
{
	if (Parent.IsValid())
	{
		Transform& ParentTransform = ECS.GetComponent<Transform>(Parent);
		ParentTransform.RemoveChild(Owner);
	}

	Transform& ParentTransform = ECS.GetComponent<Transform>(NewParent);
	ParentTransform.AddChild(Owner);

	Clean(ECS);
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

void Transform::Clean(EntityManager& ECS)
{
	LocalToWorld = GetLocalToParent();

	if (Parent.IsValid())
	{
		const Transform& ParentTransform = ECS.GetComponent<Transform>(Parent);
		LocalToWorld = ParentTransform.GetLocalToWorld() * LocalToWorld;
	}

	std::for_each(Children.begin(), Children.end(), [&] (Entity Child) 
	{
		Transform& ChildTransform = ECS.GetComponent<Transform>(Child);
		ChildTransform.Clean(ECS);
	});
}