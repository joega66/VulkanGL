#pragma once
#include <GPU/GPU.h>

BEGIN_DESCRIPTOR_SET(PostProcessingDescriptors)
	DESCRIPTOR(gpu::StorageImage, _DisplayColor)
	DESCRIPTOR(gpu::StorageImage, _HDRColor)
END_DESCRIPTOR_SET(PostProcessingDescriptors)