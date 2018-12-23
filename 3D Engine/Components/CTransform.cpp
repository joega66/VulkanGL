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
	if (bDirty)
	{
		LocalToWorld = GetLocalToParent();

		if (Parent)
		{
			LocalToWorld = Parent->GetLocalToWorld() * LocalToWorld;
		}
		
		LocalToWorldUniform->Set(LocalToWorld);
		bDirty = false;
	}

	return LocalToWorld;
}

const glm::mat4& CTransform::GetLocalToParent()
{
	if (bDirty)
	{
		LocalToParent = glm::mat4();
		LocalToParent = glm::translate(LocalToParent, Position);
		LocalToParent = glm::rotate(LocalToParent, glm::radians(Angle), Rotation);
		LocalToParent = glm::scale(LocalToParent, ScaleBy);
	}

	return LocalToParent;
}

void CTransform::Translate(const glm::vec3& InPosition)
{
	Position = InPosition;
	MarkDirty();
}

void CTransform::Rotate(const glm::vec3& InRotation, float InAngle)
{
	Rotation = InRotation;
	Angle = InAngle;
	MarkDirty();
}

void CTransform::Scale(const glm::vec3& InScale)
{
	ScaleBy = InScale;
	MarkDirty();
}

void CTransform::SetParent(CTransform* NewParent)
{
	if (Parent)
	{
		Parent->RemoveChild(this);
	}

	Parent = NewParent;

	Parent->AddChild(Parent);

	MarkDirty();
}

void CTransform::AddChild(CTransform* Child)
{
	Children.push_back(Child);
}

void CTransform::RemoveChild(CTransform* Child)
{
	Children.remove(Child);
}

bool CTransform::IsDirty() const
{
	return bDirty;
}

void CTransform::MarkDirty()
{
	bDirty = true;
	GetLocalToWorld(); // @todo Ugh.
	std::for_each(Children.begin(), Children.end(), [&] (CTransform* Child) { Child->MarkDirty(); });
}