#pragma once


#include "../components/Component.h"

#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <atomic>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <typeindex>


// ObjectPool: Memory pool for efficient object allocation/deallocation
// REQUIREMENT #1: No allocation during main loop!
template<typename T>
class ObjectPool {
private:
    std::vector<std::unique_ptr<T>> pool;
    std::queue<T*> available;
    mutable std::mutex poolMutex;


    size_t capacity;
    std::atomic<size_t> inUse{ 0 };
    std::atomic<size_t> totalCreated{ 0 };

public:
    // Constructor
    explicit ObjectPool(size_t initialCapacity = 100) : capacity(initialCapacity) {
        pool.reserve(capacity);

        // Pre-allocate objects to avoid allocation during gameplay
        for (size_t i = 0; i < capacity; ++i) {
            auto obj = std::make_unique<T>();
            T* ptr = obj.get();
            pool.push_back(std::move(obj));
            available.push(ptr);
        }

        totalCreated = capacity;
    }

    // Destructor
    ~ObjectPool() = default;

    // Delete copy operations
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    // Move operations
    ObjectPool(ObjectPool&& other) noexcept
        : pool(std::move(other.pool))
        , available(std::move(other.available))
        , capacity(other.capacity)
        , inUse(other.inUse.load())
        , totalCreated(other.totalCreated.load()) {
    }

    ObjectPool& operator=(ObjectPool&& other) noexcept {
        if (this != &other) {
            pool = std::move(other.pool);
            available = std::move(other.available);
            capacity = other.capacity;
            inUse = other.inUse.load();
            totalCreated = other.totalCreated.load();
        }
        return *this;
    }

    // Get an object from the pool
    T* Get() {
        std::lock_guard<std::mutex> lock(poolMutex);

        if (available.empty()) {
            // Pool exhausted, create new object (should be rare)
            auto obj = std::make_unique<T>();
            T* ptr = obj.get();
            pool.push_back(std::move(obj));
            totalCreated++;

            inUse++;
            return ptr;
        }

        T* obj = available.front();
        available.pop();
        inUse++;

        return obj;
    }

    // Return an object to the pool
    void Return(T* obj) {
        if (!obj) return;

        std::lock_guard<std::mutex> lock(poolMutex);

        // Reset object state if needed
        ResetObject(obj);

        available.push(obj);
        inUse--;
    }

    // Pool state queries
    bool HasAvailable() const {
        std::lock_guard<std::mutex> lock(poolMutex);
        return !available.empty();
    }

    bool CanReturn() const {
        return true; // Always can return objects
    }

    size_t GetCapacity() const { return capacity; }
    size_t GetInUse() const { return inUse.load(); }
    size_t GetAvailable() const {
        std::lock_guard<std::mutex> lock(poolMutex);
        return available.size();
    }
    size_t GetTotalCreated() const { return totalCreated.load(); }

    // Pool management
    void Reserve(size_t newCapacity) {
        if (newCapacity <= capacity) return;

        std::lock_guard<std::mutex> lock(poolMutex);

        size_t toCreate = newCapacity - capacity;
        pool.reserve(newCapacity);

        for (size_t i = 0; i < toCreate; ++i) {
            auto obj = std::make_unique<T>();
            T* ptr = obj.get();
            pool.push_back(std::move(obj));
            available.push(ptr);
        }

        capacity = newCapacity;
        totalCreated += toCreate;
    }

    // Statistics
    float GetUtilization() const {
        return static_cast<float>(inUse.load()) / static_cast<float>(totalCreated.load());
    }

    void PrintStats() const {
        std::cout << "ObjectPool<" << typeid(T).name() << "> Stats:" << std::endl;
        std::cout << "  Capacity: " << capacity << std::endl;
        std::cout << "  In Use: " << inUse.load() << std::endl;
        std::cout << "  Available: " << GetAvailable() << std::endl;
        std::cout << "  Total Created: " << totalCreated.load() << std::endl;
        std::cout << "  Utilization: " << (GetUtilization() * 100.0f) << "%" << std::endl;
    }

private:
    // Reset object state (override for specific types)
    void ResetObject(T* obj) {
        // Default: do nothing
        // Specialized versions can reset object state
        (void)obj; // Suppress unused parameter warning
    }
};

// Specialized reset functions for common types
template<>
inline void ObjectPool<class Component>::ResetObject(Component* component) {
    if (component) {
        // Reset component state
        component->SetActive(true);
        component->SetOwner(nullptr);
        // Add other component-specific resets as needed
    }
}

// Pool manager for multiple object types
class PoolManager {
private:
    static PoolManager* instance;

    // Type-erased pools
    std::unordered_map<std::type_index, std::unique_ptr<void, std::function<void(void*)>>> pools;

public:
    static PoolManager& GetInstance() {
        if (!instance) {
            instance = new PoolManager();
        }
        return *instance;
    }

    static void DestroyInstance() {
        delete instance;
        instance = nullptr;
    }

    template<typename T>
    ObjectPool<T>* GetPool() {
        std::type_index typeIndex = std::type_index(typeid(T));

        auto it = pools.find(typeIndex);
        if (it == pools.end()) {
            // Create new pool
            auto pool = std::make_unique<ObjectPool<T>>();
            ObjectPool<T>* poolPtr = pool.get();

            // Store with custom deleter
            pools[typeIndex] = std::unique_ptr<void, std::function<void(void*)>>(
                pool.release(),
                [](void* ptr) { delete static_cast<ObjectPool<T>*>(ptr); }
            );

            return poolPtr;
        }

        return static_cast<ObjectPool<T>*>(it->second.get());
    }

    template<typename T>
    void CreatePool(size_t capacity) {
        std::type_index typeIndex = std::type_index(typeid(T));

        if (pools.find(typeIndex) == pools.end()) {
            auto pool = std::make_unique<ObjectPool<T>>(capacity);

            pools[typeIndex] = std::unique_ptr<void, std::function<void(void*)>>(
                pool.release(),
                [](void* ptr) { delete static_cast<ObjectPool<T>*>(ptr); }
            );
        }
    }

    void PrintAllPoolStats() const {
        std::cout << "\n=== Pool Manager Statistics ===" << std::endl;
        std::cout << "Active Pools: " << pools.size() << std::endl;
        // Individual pool stats would need type-specific handling
    }

private:
    PoolManager() = default;
    ~PoolManager() = default;
};

// Convenience macros
#define GET_POOL(Type) PoolManager::GetInstance().GetPool<Type>()
#define CREATE_POOL(Type, Capacity) PoolManager::GetInstance().CreatePool<Type>(Capacity)