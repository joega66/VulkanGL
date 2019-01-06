#include "CTransform.h"
#include "GL.h"

CTransform::CTransform(const CTransform& Other)
	: CTransform(Other.Position, Other.Rotation, Other.Angle, Other.ScaleBy)
{
}

CTransform::CTransform(const glm::vec3& Position, const glm::vec3& Rotation, float Angle, const glm::vec3& InScale)
{
	LocalToWorldUniform = GLCreateUniformBuffer<glm::mat4>(EUniformUpdate::Frequent);
	Translate(Position);
	Rotate(Rotation, Angle);
	Scale(InScale);
}

CTransform::~CTransform()
{
	if (Parent)
	{
		Parent->RemoveChild(this);
	}
}

const glm::mat4& CTransform::GetLocalToWorld()
{
	return LocalToWorld;
}

const glm::mat4& CTransform::GetLocalToParent()
{
	LocalToParent = glm::mat4();
	LocalToParent = glm::translate(LocalToParent, Position);
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
	if (Parent)
	{
		Parent->RemoveChild(this);
	}

	Parent = NewParent;
	Parent->AddChild(Parent);

	Clean();
}

void CTransform::AddChild(CTransform* Child)
{
	Children.push_back(Child);
}

void CTransform::RemoveChild(CTransform* Child)
{
	Children.remove(Child);
}

void CTransform::Clean()
{
	LocalToWorld = GetLocalToParent();

	if (Parent)
	{
		LocalToWorld = Parent->GetLocalToWorld() * LocalToWorld;
	}

	LocalToWorldUniform->Set(LocalToWorld);

	std::for_each(Children.begin(), Children.end(), [&] (CTransform* Child) { Child->Clean(); });
}