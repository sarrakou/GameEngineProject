#pragma once

#include "ThreadPool.h"
#include "../components/Transform.h"
#include "../components/Behavior.h"
#include "../core/Scene.h"
#include <vector>
#include <memory>

// UpdateSystem: Implements Data-Oriented Design with threading
// This system processes components in batches using the ThreadPool
class UpdateSystem {
private:
    std::unique_ptr<ThreadPool> threadPool;

    // Update frequency control
    float fixedUpdateInterval = 1.0f / 60.0f; // 60 FPS
    float fixedUpdateAccumulator = 0.0f;

    // Performance tracking
    struct PerformanceStats {
        float lastUpdateTime = 0.0f;
        float lastLateUpdateTime = 0.0f;
        float lastFixedUpdateTime = 0.0f;
        size_t transformsProcessed = 0;
        size_t behaviorsProcessed = 0;
        float averageFrameTime = 0.0f;
        int frameCount = 0;
    } stats;

    // System state
    bool enabled = true;
    bool useThreading = true;

public:
    // Constructor and destructor
    UpdateSystem(size_t numThreads = std::thread::hardware_concurrency());
    ~UpdateSystem() = default;

    // Delete copy operations
    UpdateSystem(const UpdateSystem&) = delete;
    UpdateSystem& operator=(const UpdateSystem&) = delete;

    // System control
    void SetEnabled(bool enable) { enabled = enable; }
    bool IsEnabled() const { return enabled; }

    void SetThreadingEnabled(bool enable) { useThreading = enable; }
    bool IsThreadingEnabled() const { return useThreading; }

    void SetFixedUpdateRate(float fps) { fixedUpdateInterval = 1.0f / fps; }
    float GetFixedUpdateRate() const { return 1.0f / fixedUpdateInterval; }

    // Main update methods (called by Engine)
    void Update(Scene* scene, float deltaTime);
    void LateUpdate(Scene* scene, float deltaTime);
    void FixedUpdate(Scene* scene, float deltaTime);

    // Data-Oriented batch processing (REQUIREMENT #3 & #5)
    void UpdateTransforms(std::vector<Transform*>& transforms, float deltaTime);
    void UpdateBehaviors(std::vector<Behavior*>& behaviors, float deltaTime);
    void LateUpdateBehaviors(std::vector<Behavior*>& behaviors, float deltaTime);
    void FixedUpdateBehaviors(std::vector<Behavior*>& behaviors, float deltaTime);

    // Specialized batch operations
    void TransformOperations(std::vector<Transform*>& transforms, const std::function<void(Transform*)>& operation);
    void BehaviorOperations(std::vector<Behavior*>& behaviors, const std::function<void(Behavior*)>& operation);

    // Parallel algorithms for common operations
    void ParallelTranslate(std::vector<Transform*>& transforms, const Vector3& translation);
    void ParallelRotate(std::vector<Transform*>& transforms, const Vector3& rotation);
    void ParallelScale(std::vector<Transform*>& transforms, float scale);

    // Distance calculations (useful for AI, physics)
    void CalculateDistances(std::vector<Transform*>& transforms, const Transform* target, std::vector<float>& outDistances);

    // Frustum culling for rendering optimization
    void FrustumCull(std::vector<Transform*>& transforms, std::vector<bool>& outVisibility);

    // Performance and diagnostics
    const PerformanceStats& GetStats() const { return stats; }
    void ResetStats();
    void PrintPerformanceInfo() const;

    // ThreadPool access
    ThreadPool& GetThreadPool() { return *threadPool; }

private:
    // Internal update methods
    void UpdateSingleThreaded(Scene* scene, float deltaTime);
    void UpdateMultiThreaded(Scene* scene, float deltaTime);

    void LateUpdateSingleThreaded(Scene* scene, float deltaTime);
    void LateUpdateMultiThreaded(Scene* scene, float deltaTime);

    void FixedUpdateSingleThreaded(Scene* scene, float deltaTime);
    void FixedUpdateMultiThreaded(Scene* scene, float deltaTime);

    // Performance tracking helpers
    void StartTimer();
    float EndTimer();
    void UpdateFrameTimeAverage(float frameTime);
};

// SystemManager: Manages all engine systems
class SystemManager {
private:
    std::unique_ptr<UpdateSystem> updateSystem;

    // System state
    bool initialized = false;

public:
    // Singleton pattern
    static SystemManager& GetInstance();

    // System lifecycle
    void Initialize(size_t numThreads = std::thread::hardware_concurrency());
    void Shutdown();

    // System access
    UpdateSystem& GetUpdateSystem();

    // Main system update (called by Engine)
    void UpdateSystems(Scene* scene, float deltaTime);
    void LateUpdateSystems(Scene* scene, float deltaTime);
    void FixedUpdateSystems(Scene* scene, float deltaTime);

    // System info
    bool IsInitialized() const { return initialized; }
    void PrintSystemInfo() const;
};

// Convenience macros for global system access
#define UPDATE_SYSTEM SystemManager::GetInstance().GetUpdateSystem()
#define UPDATE_ALL_SYSTEMS(scene, deltaTime) SystemManager::GetInstance().UpdateSystems(scene, deltaTime)