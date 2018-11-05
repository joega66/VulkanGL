#pragma once
#include "../GL.h"

class SceneRenderTargets : public WindowResizeListener
{
public:
	virtual void OnWindowResize(int32 X, int32 Y) final
	{
		for (auto& Dependent : ResolutionDependents)
		{
		}

		GLRebuildResolutionDependents();
	}

	void AddResolutionDependent(GLImageRef Image)
	{
		ResolutionDependents.push_back(Image);
	}

	static SceneRenderTargets& Get()
	{
		static SceneRenderTargets SceneRenderTargets;
		return SceneRenderTargets;
	}

private:
	std::vector<GLImageRef> ResolutionDependents;
};