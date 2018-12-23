#include "ComponentArray.h"
#include "Entity.h"

void IComponentArray::QueueEntityForRenderUpdate(const uint64 EntityID)
{
	GEntityManager.QueueEntityForRenderUpdate(EntityID);
}