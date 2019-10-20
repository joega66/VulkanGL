#include "CTransform.h"
#include "DRM.h"

CTransform::CTransform(const CTransform& Other)
	: CTransform(Other.Position, Other.Rotation, Other.Angle, Other.ScaleBy)
{
}

CTransform::CTransform(const glm::vec3& Position, const glm::vec3& Rotation, float Angle, const glm::vec3& InScale)
{
	Translate(Position);
	Rotate(Rotation, Angle);
	Scale(InScale);
}

const glm::mat4& CTransform::GetLocalToWorld() const
{
	return LocalToWorld;
}

glm::mat4 CTransform::GetLocalToParent()
{
	glm::mat4 LocalToParent = glm::translate(glm::mat4(), Position);
	LocalToParent = glm::rotate(LocalToParent, glm::radians(Angle), Rotation);
	LocalToParent = glm::scale(LocalToParent, ScaleBy);

	return LocalToParent;
}

void CTransform::Translate(const glm::vec3& InPosition)
{
	Position = InPosition;
	Clean();
}

void CTransform::Rotate(const glm::vec3& InRotation, float InAngle)
{
	Rotation = InRotation;
	Angle = InAngle;
	Clean();
}

void CTransform::Scale(const glm::vec3& InScale)
{
	ScaleBy = InScale;
	Clean();
}

void CTransform::SetParent(CTransform* NewParent)
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

void CTransform::AddChild(CTransform* Child)
{
	Children.push_back(Child);
}

void CTransform::RemoveChild(CTransform* Child)
{
	if (std::find(Children.begin(), Children.end(), Child) != Children.end())
	{
		Children.remove(Child);
	}
}

void CTransform::Clean()
{
	LocalToWorld = GetLocalToParent();

	if (Parent)
	{
		LocalToWorld = Parent->GetLocalToWorld() * LocalToWorld;
	}

	std::for_each(Children.begin(), Children.end(), [&] (CTransform* Child) { Child->Clean(); });
}