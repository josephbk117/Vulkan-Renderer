#include "MemoryPool.h"
#include <exception>
#include <string>
#include <stdexcept>

template<size_t T>
MemoryPool<T>::MemoryPool(size_t unitCount)
{
	this->unitCount = unitCount;

	constexpr unsigned int memUnitSize = sizeof(MemoryUnit);
	totalBytesAllocated = unitCount * (unitSize + memUnitSize);

	memblockPtr = malloc(totalBytesAllocated);
	if (memblockPtr)
	{
		memset(memblockPtr, 255, totalBytesAllocated);

		for (size_t index = 0; index < unitCount; ++index)
		{
			MemoryUnit* curUnitPtr = (MemoryUnit*)((char*)memblockPtr + index * (unitSize + sizeof(MemoryUnit)));

			curUnitPtr->prevPtr = nullptr;
			curUnitPtr->nextPtr = freeUnitsPtr; //Inserts new unit at head

			if (freeUnitsPtr)
			{
				freeUnitsPtr->prevPtr = curUnitPtr;
			}
			freeUnitsPtr = curUnitPtr;
		}
	}
	else
	{
		throw std::bad_alloc();
	}
}

template<size_t T>
MemoryPool<T>::~MemoryPool()
{
	free(memblockPtr);
	memblockPtr = nullptr;
}

template<size_t T>
void* MemoryPool<T>::Allocate()
{
	return Internal_Allocate();
}

template<size_t T>
void MemoryPool<T>::Free(void* ptr)
{
	if (BelongsToPool(ptr))
	{
		Internal_Free(ptr);
	}
	else
	{
		throw std::runtime_error("Cannot deallocate pointer not belonging to pool");
	}

}

template<size_t T>
void* MemoryPool<T>::GetStart() const
{
	return memblockPtr;
}

template<size_t T>
void* MemoryPool<T>::GetEnd() const
{
	return (void*)((char*)memblockPtr + totalBytesAllocated);
}

template<size_t T>
bool MemoryPool<T>::BelongsToPool(void* ptr) const
{
	return GetStart() < ptr && ptr < GetEnd();
}

template<size_t T>
size_t MemoryPool<T>::GetCurrentMemoryUnitsAllocated() const
{
	MemoryUnit* curUnitA = allocatedUnitsPtr;
	size_t allocatedCount = 0;
	while (curUnitA)
	{
		curUnitA = curUnitA->nextPtr;
		allocatedCount++;
	}
	return allocatedCount;
}

template<size_t T>
size_t MemoryPool<T>::GetCurrentMemoryUnitsUnAllocated() const
{
	MemoryUnit* curUnitA = freeUnitsPtr;
	size_t unAllocatedCount = 0;
	while (curUnitA)
	{
		curUnitA = curUnitA->nextPtr;
		unAllocatedCount++;
	}
	return unAllocatedCount;
}

template<size_t T>
size_t MemoryPool<T>::GetTotalMemoryAllocated() const
{
	return totalBytesAllocated;
}

template<size_t T>
void* MemoryPool<T>::Internal_Allocate()
{
	MemoryUnit* curUnitPtr = freeUnitsPtr;
	freeUnitsPtr = curUnitPtr->nextPtr;
	if (freeUnitsPtr)
	{
		freeUnitsPtr->prevPtr = nullptr;
	}

	curUnitPtr->nextPtr = allocatedUnitsPtr;
	if (allocatedUnitsPtr)
	{
		allocatedUnitsPtr->prevPtr = curUnitPtr;
	}
	allocatedUnitsPtr = curUnitPtr;

	return (void*)((char*)curUnitPtr + sizeof(MemoryUnit));
}

template<size_t T>
void MemoryPool<T>::Internal_Free(void* ptr)
{
	MemoryUnit* curUnitPtr = (MemoryUnit*)((char*)ptr - sizeof(MemoryUnit));
	MemoryUnit* prevPtr = curUnitPtr->prevPtr;
	MemoryUnit* nextPtr = curUnitPtr->nextPtr;

	if (prevPtr == nullptr)
	{
		allocatedUnitsPtr = curUnitPtr->nextPtr;
	}
	else
	{
		prevPtr->nextPtr = nextPtr;
	}

	if (nextPtr)
	{
		nextPtr->prevPtr = prevPtr;
	}

	curUnitPtr->nextPtr = freeUnitsPtr;
	if (freeUnitsPtr)
	{
		freeUnitsPtr->prevPtr = curUnitPtr;
	}

	freeUnitsPtr = curUnitPtr;
}

template MemoryPool<1>;
template MemoryPool<4>;
template MemoryPool<8>;
template MemoryPool<16>;
template MemoryPool<32>;
template MemoryPool<128>;
template MemoryPool<1024>;
template MemoryPool<4096>;