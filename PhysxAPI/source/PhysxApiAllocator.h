//
// Created by zhangakai on 2026/4/13.
//

#ifndef PHYSXAPI_PHYSXAPIALLOCATOR_H
#define PHYSXAPI_PHYSXAPIALLOCATOR_H

#include "PhysxApiCommon.h"
#include <atomic>
#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <cstring>
// 针对不同平台的对齐分配函数封装
#if defined(_MSC_VER) // Windows
#define ALIGNED_ALLOC(size, alignment) _aligned_malloc(size, alignment)
#define ALIGNED_FREE(ptr) _aligned_free(ptr)
#else // Linux/Android/iOS
inline void* posix_alloc(size_t size, size_t alignment)
{
    if (alignment < alignof(void*))
    {
        std::cerr << "[PhysX] Invalid alignment (" << alignment
            << "): must be >= " << alignof(void*) << std::endl;
        return nullptr;
    }
    if ((alignment & (alignment - 1)) != 0)
    {
        std::cerr << "[PhysX] Invalid alignment (" << alignment
            << "): must be a power of two" << std::endl;
        return nullptr;
    }

    void* ptr = nullptr;
    const int rc = posix_memalign(&ptr, alignment, size);
    if (rc != 0)
    {
        std::cerr << "[PhysX] posix_memalign failed (alignment=" << alignment
            << ", size=" << size << "): " << std::strerror(rc) << std::endl;
        return nullptr;
    }
    return ptr;
}

#define ALIGNED_ALLOC(size, alignment) posix_alloc(size, alignment)
#define ALIGNED_FREE(ptr) free(ptr)
#endif
/**
 * @brief PhysX 内存分配器 具备内存追踪和对齐功能
 */
class PhysxApiAllocator : public physx::PxAllocatorCallback
{
public:
    PhysxApiAllocator() : mCurrentUsage(0), mTotalAllocations(0)
    {
    }

    virtual ~PhysxApiAllocator()
    {
        if (mCurrentUsage > 0)
        {
            std::cerr << "[PhysX] Warning: Memory leak detected! " << mCurrentUsage << " bytes still allocated." <<
                std::endl;
        }
    }

    /**
     * @param size 申请内存的大小
     * @param typeName 对象的类型名
     * @param filename 文件名
     * @param line 行号
     */
    virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) override
    {
        // 1. PhysX 要求至少 16 字节对齐
        const size_t alignment = 16;
        // 2. 为了追踪内存，我们需要在头部多分配一点空间来存储这块内存的大小
        // 我们分配 16 字节作为 Header，这样返回给 PhysX 的地址依然是 16 字节对齐的
        const size_t headerSize = 16;
        size_t totalSize = size + headerSize;
        void* rawPtr = ALIGNED_ALLOC(totalSize, alignment);
        if (!rawPtr)
        {
            std::cerr << "[PhysX] Out of memory! Request size: " << size << std::endl;
            return nullptr;
        }
        // 3. 在 Header 中存储大小
        *(size_t*)rawPtr = size;
        // 4. 更新统计数据 (原子操作，线程安全)
        mCurrentUsage += size;
        mTotalAllocations++;
        // 5. 返回 Header 之后的地址给 PhysX
        void* userPtr = (static_cast<char*>(rawPtr) + headerSize);
        // 调试日志 (仅在需要深度追踪时开启)
        // printf("Alloc: %zu bytes for %s at %s:%d\n", size, typeName, filename, line);
        return userPtr;
    }

    virtual void deallocate(void* ptr) override
    {
        if (!ptr) return;
        const size_t headerSize = 16;
        void* rawPtr = static_cast<char*>(ptr) - headerSize;
        // 1. 取出 Header 中存储的大小
        size_t size = *(size_t*)rawPtr;
        // 2. 更新统计数据
        mCurrentUsage -= size;
        // 3. 释放内存
        ALIGNED_FREE(rawPtr);
    }

    // --- 辅助功能，性能看板 ---
    size_t getCurrentMemoryUsage() const { return mCurrentUsage.load(); }
    size_t getTotalAllocationCount() const { return mTotalAllocations.load(); }

private:
    std::atomic<size_t> mCurrentUsage; // 当前物理引擎占用的总字节数
    std::atomic<size_t> mTotalAllocations; // 总分配次数
};

#endif //PHYSXAPI_PHYSXAPIALLOCATOR_H
