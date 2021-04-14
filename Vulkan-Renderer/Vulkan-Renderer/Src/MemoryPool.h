#pragma once
template<size_t T>
class MemoryPool
{
public:
	MemoryPool(size_t unitCount);
	~MemoryPool();
	void* Allocate();
	void Free(void* ptr);
	void* GetStart() const;
	void* GetEnd()const;
	bool BelongsToPool(void* ptr) const;
	size_t GetCurrentMemoryUnitsAllocated() const;
	size_t GetCurrentMemoryUnitsUnAllocated() const;
	size_t GetTotalMemoryAllocated() const;

protected:

	void* memblockPtr = nullptr;

	struct MemoryUnit
	{
		MemoryUnit* prevPtr = nullptr;
		MemoryUnit* nextPtr = nullptr;
	};

	MemoryUnit* allocatedUnitsPtr = nullptr;
	MemoryUnit* freeUnitsPtr = nullptr;

	size_t unitCount;
	size_t unitSize = T;
	size_t totalBytesAllocated = 0;

	void* Internal_Allocate();
	void Internal_Free(void* ptr);
};

using MemoryPool_1_Byte = MemoryPool<1>;
using MemoryPool_4_Bytes = MemoryPool<4>;
using MemoryPool_8_Bytes = MemoryPool<8>;
using MemoryPool_16_Bytes  = MemoryPool<16>;
using MemoryPool_32_Bytes = MemoryPool<32>;
using MemoryPool_128_Bytes = MemoryPool<128>;
using MemoryPool_1_KiloByte = MemoryPool<1024>;
using MemoryPool_4_KiloBytes = MemoryPool<4096>;