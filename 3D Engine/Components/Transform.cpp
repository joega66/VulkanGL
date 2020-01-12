#include "Transform.h"
#include "DRM.h"

Transform::Transform(const Transform& Other)
	: Transform(Other.Position, Other.Rotation, Other.Angle, Other.ScaleBy)
{
}

Transform::Transform(const glm::vec3& Position, const glm::vec3& Rotation, float Angle, const glm::vec3& InScale)
{
	Translate(Position);
	Rotate(Rotation, Angle);
	Scale(InScale);
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

void Transform::Translate(const glm::vec3& InPosition)
{
	Position = InPosition;
	Clean();
}

void Transform::Rotate(const glm::vec3& InRotation, float InAngle)
{
	Rotation = InRotation;
	Angle = InAngle;
	Clean();
}

void Transform::Scale(const glm::vec3& InScale)
{
	ScaleBy = InScale;
	Clean();
}

void Transform::SetParent(Transform* NewParent)
{
	if (NewParent == this)
	{
		return;
	}

	if (Parent)
	{
		Parent->RemoveChild(this);
	}

	Parent = NewParent;

	if (Parent == nullptr)
	{
		return;
	}

	Parent->AddChild(this);

	Clean();
}

void Transform::AddChild(Transform* Child)
{
	Children.push_back(Child);
}

void Transform::RemoveChild(Transform* Child)
{
	if (std::find(Children.begin(), Children.end(), Child) != Children.end())
	{
		Children.remove(Child);
	}
}

void Transform::Clean()
{
	LocalToWorld = GetLocalToParent();

	if (Parent)
	{
		LocalToWorld = Parent->GetLocalToWorld() * LocalToWorld;
	}

	std::for_each(Children.begin(), Children.end(), [&] (Transform* Child) { Child->Clean(); });
}