#include "../include/memory/MemoryManager.h"
#include "../include/components/Component.h"
#include "../include/core/GameObject.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>

// Static instance initialization
MemoryManager* MemoryManager::instance = nullptr;

MemoryManager& MemoryManager::GetInstance() {
    if (instance == nullptr) {
        instance = new MemoryManager();
    }
    return *instance;
}

void MemoryManager::DestroyInstance() {
    delete instance;
    instance = nullptr;
}

MemoryManager::MemoryManager() {
    InitializePools();
    std::cout << "MemoryManager initialized" << std::endl;
}

MemoryManager::~MemoryManager() {
    CleanupPools();

    // Check for memory leaks
    if (trackAllocations) {
        CheckForLeaks();
    }

    std::cout << "MemoryManager destroyed" << std::endl;
}

// Memory allocation/deallocation with tracking
void* MemoryManager::Allocate(size_t size, size_t alignment) {
    // Use aligned allocation if needed
    void* ptr = nullptr;

#ifdef _WIN32
    ptr = _aligned_malloc(size, alignment);
#else
    if (posix_memalign(&ptr, alignment, size) != 0) {
        ptr = nullptr;
    }
#endif

    if (!ptr) {
        throw std::bad_alloc();
    }

    if (trackAllocations) {
        TrackAllocation(ptr, size);
    }

    stats.RecordAllocation(size);
    return ptr;
}

void MemoryManager::Deallocate(void* ptr) {
    if (!ptr) return;

    if (trackAllocations) {
        UntrackAllocation(ptr);
    }

#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

// Memory cleanup and optimization
void MemoryManager::DefragmentPools() {
    // Pool defragmentation would be complex to implement
    // For now, just print a message
    std::cout << "Pool defragmentation not implemented" << std::endl;
}

void MemoryManager::ShrinkPools() {
    // Shrink pools by removing unused capacity
    std::cout << "Pool shrinking not implemented" << std::endl;
}

void MemoryManager::ClearUnusedPools() {
    // Remove pools that haven't been used
    std::cout << "Clearing unused pools not implemented" << std::endl;
}

// Pre-allocation for game engine objects
void MemoryManager::PreallocateGameObjects(size_t count) {
    CreatePool<GameObject>(count);
    std::cout << "Pre-allocated " << count << " GameObjects" << std::endl;
}

void MemoryManager::PreallocateComponents(size_t count) {
    CreatePool<Component>(count);
    std::cout << "Pre-allocated " << count << " Components" << std::endl;
}

// Memory pressure handling
void MemoryManager::OnLowMemory() {
    std::cout << "Low memory warning - attempting cleanup" << std::endl;
    ShrinkPools();
    ClearUnusedPools();
}

void MemoryManager::OnMemoryWarning() {
    std::cout << "Memory warning - current usage: " << GetCurrentUsage() << " bytes" << std::endl;
}

// Debug and diagnostics
void MemoryManager::PrintMemoryStats() const {
    std::cout << "\n=== Memory Manager Statistics ===" << std::endl;
    std::cout << "Current Usage: " << std::setw(10) << stats.currentUsage.load() << " bytes" << std::endl;
    std::cout << "Peak Usage: " << std::setw(13) << stats.peakUsage.load() << " bytes" << std::endl;
    std::cout << "Total Allocated: " << std::setw(8) << stats.totalAllocated.load() << " bytes" << std::endl;
    std::cout << "Total Deallocated: " << std::setw(6) << stats.totalDeallocated.load() << " bytes" << std::endl;
    std::cout << "Allocation Count: " << std::setw(9) << stats.allocationCount.load() << std::endl;
    std::cout << "Deallocation Count: " << std::setw(7) << stats.deallocationCount.load() << std::endl;
    std::cout << "Active Pools: " << std::setw(13) << typePools.size() << std::endl;
    std::cout << "Tracking Enabled: " << std::setw(9) << (trackAllocations ? "Yes" : "No") << std::endl;
    std::cout << "Object Pools Enabled: " << std::setw(5) << (useObjectPools ? "Yes" : "No") << std::endl;
}

void MemoryManager::PrintPoolStats() const {
    std::cout << "\n=== Object Pool Statistics ===" << std::endl;
    std::cout << "Number of Active Pools: " << typePools.size() << std::endl;

    // Individual pool stats would require type-specific handling
    // This is a placeholder implementation
    for (const auto& pair : typePools) {
        std::cout << "Pool for type index: " << pair.first.name() << std::endl;
    }
}

void MemoryManager::DumpMemoryReport() const {
    std::cout << "\n=== Complete Memory Report ===" << std::endl;
    PrintMemoryStats();
    PrintPoolStats();

    if (trackAllocations) {
        std::cout << "\n=== Active Allocations ===" << std::endl;
        std::lock_guard<std::mutex> lock(allocationMutex);
        std::cout << "Tracked Allocations: " << allocationSizes.size() << std::endl;

        size_t totalTracked = 0;
        for (const auto& pair : allocationSizes) {
            totalTracked += pair.second;
        }
        std::cout << "Total Tracked Size: " << totalTracked << " bytes" << std::endl;
    }
}

// Memory validation
bool MemoryManager::ValidateMemory() const {
    // Basic memory validation
    bool valid = true;

    if (stats.currentUsage.load() > stats.peakUsage.load()) {
        std::cerr << "Memory validation error: current usage exceeds peak" << std::endl;
        valid = false;
    }

    if (stats.allocationCount.load() < stats.deallocationCount.load()) {
        std::cerr << "Memory validation error: more deallocations than allocations" << std::endl;
        valid = false;
    }

    return valid;
}

void MemoryManager::CheckForLeaks() const {
    if (!trackAllocations) {
        std::cout << "Memory leak checking disabled (tracking not enabled)" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(allocationMutex);

    if (allocationSizes.empty()) {
        std::cout << "No memory leaks detected" << std::endl;
    }
    else {
        std::cout << "Memory leaks detected!" << std::endl;
        std::cout << "Leaked allocations: " << allocationSizes.size() << std::endl;

        size_t totalLeaked = 0;
        for (const auto& pair : allocationSizes) {
            totalLeaked += pair.second;
        }
        std::cout << "Total leaked memory: " << totalLeaked << " bytes" << std::endl;
    }
}

// Private helpers
void MemoryManager::InitializePools() {
    // Create pools for common game engine types
    if (useObjectPools) {
        CreatePool<Component>(defaultPoolSize);
        CreatePool<GameObject>(defaultPoolSize / 2); // Fewer GameObjects than Components
    }
}

void MemoryManager::CleanupPools() {
    typePools.clear();
}

void MemoryManager::TrackAllocation(void* ptr, size_t size) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(allocationMutex);
    allocationSizes[ptr] = size;
}

void MemoryManager::UntrackAllocation(void* ptr) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(allocationMutex);
    auto it = allocationSizes.find(ptr);
    if (it != allocationSizes.end()) {
        stats.RecordDeallocation(it->second);
        allocationSizes.erase(it);
    }
}