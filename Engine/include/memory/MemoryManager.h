#pragma once

#include "ObjectPool.h"
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <atomic>
#include <mutex>

// Forward declarations
class Component;
class GameObject;

// Memory allocation tracking
struct MemoryStats {
    std::atomic<size_t> totalAllocated{ 0 };
    std::atomic<size_t> totalDeallocated{ 0 };
    std::atomic<size_t> currentUsage{ 0 };
    std::atomic<size_t> peakUsage{ 0 };
    std::atomic<size_t> allocationCount{ 0 };
    std::atomic<size_t> deallocationCount{ 0 };

    void RecordAllocation(size_t size) {
        totalAllocated += size;
        currentUsage += size;
        allocationCount++;

        // Update peak usage
        size_t current = currentUsage.load();
        size_t peak = peakUsage.load();
        while (current > peak && !peakUsage.compare_exchange_weak(peak, current)) {
            // Retry if another thread updated peak
        }
    }

    void RecordDeallocation(size_t size) {
        totalDeallocated += size;
        currentUsage -= size;
        deallocationCount++;
    }

    void Reset() {
        totalAllocated = 0;
        totalDeallocated = 0;
        currentUsage = 0;
        peakUsage = 0;
        allocationCount = 0;
        deallocationCount = 0;
    }
};

// MemoryManager: Central memory management system
// REQUIREMENT #1: No allocation during main loop!
class MemoryManager {
private:
    // Object pools for different types
    std::unordered_map<std::type_index, std::unique_ptr<void, std::function<void(void*)>>> typePools;

    // Memory statistics
    MemoryStats stats;

    // Memory allocation tracking
    mutable std::mutex allocationMutex;
    std::unordered_map<void*, size_t> allocationSizes;

    // Singleton instance
    static MemoryManager* instance;

    // Configuration
    bool trackAllocations = true;
    bool useObjectPools = true;
    size_t defaultPoolSize = 100;

public:
    // Singleton access
    static MemoryManager& GetInstance();
    static void DestroyInstance();

    // Constructor and destructor
    MemoryManager();
    ~MemoryManager();

    // Delete copy operations
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // Object pool management
    template<typename T>
    ObjectPool<T>* GetOrCreatePool(size_t capacity = 0);

    template<typename T>
    void CreatePool(size_t capacity);

    template<typename T>
    T* GetFromPool();

    template<typename T>
    void ReturnToPool(T* object);

    // Memory allocation/deallocation with tracking
    void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    void Deallocate(void* ptr);

    template<typename T, typename... Args>
    T* New(Args&&... args);

    template<typename T>
    void Delete(T* object);

    // Bulk operations for Data-Oriented Design
    template<typename T>
    std::vector<T*> AllocateBatch(size_t count);

    template<typename T>
    void DeallocateBatch(std::vector<T*>& objects);

    // Memory statistics and monitoring
    const MemoryStats& GetStats() const { return stats; }
    size_t GetCurrentUsage() const { return stats.currentUsage.load(); }
    size_t GetPeakUsage() const { return stats.peakUsage.load(); }
    size_t getAllocationCount() const { return stats.allocationCount.load(); }

    // Memory management configuration
    void SetTrackAllocations(bool enable) { trackAllocations = enable; }
    void SetUseObjectPools(bool enable) { useObjectPools = enable; }
    void SetDefaultPoolSize(size_t size) { defaultPoolSize = size; }

    bool IsTrackingAllocations() const { return trackAllocations; }
    bool IsUsingObjectPools() const { return useObjectPools; }
    size_t GetDefaultPoolSize() const { return defaultPoolSize; }

    // Memory cleanup and optimization
    void DefragmentPools();
    void ShrinkPools();
    void ClearUnusedPools();

    // Pre-allocation for game engine objects
    void PreallocateGameObjects(size_t count);
    void PreallocateComponents(size_t count);

    // Memory pressure handling
    void OnLowMemory();
    void OnMemoryWarning();

    // Debug and diagnostics
    void PrintMemoryStats() const;
    void PrintPoolStats() const;
    void DumpMemoryReport() const;

    // Memory validation
    bool ValidateMemory() const;
    void CheckForLeaks() const;

private:
    // Internal helpers
    void InitializePools();
    void CleanupPools();
    void TrackAllocation(void* ptr, size_t size);
    void UntrackAllocation(void* ptr);

    template<typename T>
    void* GetTypeErasedPool();
};

// Template implementations
template<typename T>
ObjectPool<T>* MemoryManager::GetOrCreatePool(size_t capacity) {
    std::type_index typeIndex = std::type_index(typeid(T));

    auto it = typePools.find(typeIndex);
    if (it == typePools.end()) {
        // Create new pool
        size_t poolCapacity = (capacity > 0) ? capacity : defaultPoolSize;
        auto pool = std::make_unique<ObjectPool<T>>(poolCapacity);
        ObjectPool<T>* poolPtr = pool.get();

        // Store with type-erased deleter
        typePools[typeIndex] = std::unique_ptr<void, std::function<void(void*)>>(
            pool.release(),
            [](void* ptr) { delete static_cast<ObjectPool<T>*>(ptr); }
        );

        return poolPtr;
    }

    return static_cast<ObjectPool<T>*>(it->second.get());
}

template<typename T>
void MemoryManager::CreatePool(size_t capacity) {
    GetOrCreatePool<T>(capacity);
}

template<typename T>
T* MemoryManager::GetFromPool() {
    if (!useObjectPools) {
        return New<T>();
    }

    ObjectPool<T>* pool = GetOrCreatePool<T>();
    return pool->Get();
}

template<typename T>
void MemoryManager::ReturnToPool(T* object) {
    if (!object) return;

    if (!useObjectPools) {
        Delete(object);
        return;
    }

    ObjectPool<T>* pool = GetOrCreatePool<T>();
    pool->Return(object);
}

template<typename T, typename... Args>
T* MemoryManager::New(Args&&... args) {
    void* memory = Allocate(sizeof(T), alignof(T));
    if (!memory) {
        throw std::bad_alloc();
    }

    try {
        return new(memory) T(std::forward<Args>(args)...);
    }
    catch (...) {
        Deallocate(memory);
        throw;
    }
}

template<typename T>
void MemoryManager::Delete(T* object) {
    if (!object) return;

    object->~T();
    Deallocate(object);
}

template<typename T>
std::vector<T*> MemoryManager::AllocateBatch(size_t count) {
    std::vector<T*> result;
    result.reserve(count);

    if (useObjectPools) {
        ObjectPool<T>* pool = GetOrCreatePool<T>();
        for (size_t i = 0; i < count; ++i) {
            result.push_back(pool->Get());
        }
    }
    else {
        for (size_t i = 0; i < count; ++i) {
            result.push_back(New<T>());
        }
    }

    return result;
}

template<typename T>
void MemoryManager::DeallocateBatch(std::vector<T*>& objects) {
    if (useObjectPools) {
        ObjectPool<T>* pool = GetOrCreatePool<T>();
        for (T* obj : objects) {
            if (obj) {
                pool->Return(obj);
            }
        }
    }
    else {
        for (T* obj : objects) {
            Delete(obj);
        }
    }

    objects.clear();
}

// Global memory management functions
namespace Memory {
    // Convenience functions
    template<typename T, typename... Args>
    T* New(Args&&... args) {
        return MemoryManager::GetInstance().New<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    void Delete(T* object) {
        MemoryManager::GetInstance().Delete(object);
    }

    template<typename T>
    T* GetFromPool() {
        return MemoryManager::GetInstance().GetFromPool<T>();
    }

    template<typename T>
    void ReturnToPool(T* object) {
        MemoryManager::GetInstance().ReturnToPool(object);
    }

    // Memory stats - made inline to avoid multiple definition
    inline void PrintStats() {
        MemoryManager::GetInstance().PrintMemoryStats();
    }

    inline size_t GetCurrentUsage() {
        return MemoryManager::GetInstance().GetCurrentUsage();
    }
}

// Convenience macros
#define MEMORY_MANAGER MemoryManager::GetInstance()
#define NEW_OBJECT(Type, ...) Memory::New<Type>(__VA_ARGS__)
#define DELETE_OBJECT(ptr) Memory::Delete(ptr)
#define GET_FROM_POOL(Type) Memory::GetFromPool<Type>()
#define RETURN_TO_POOL(ptr) Memory::ReturnToPool(ptr)