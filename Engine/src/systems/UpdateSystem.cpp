#include "../include/systems/UpdateSystem.h"
#include <iostream>
#include <chrono>
#include <algorithm>

UpdateSystem::UpdateSystem(size_t numThreads) {
    threadPool = std::make_unique<ThreadPool>(numThreads);
    std::cout << "UpdateSystem initialized with " << numThreads << " threads" << std::endl;
}

// Main update methods
void UpdateSystem::Update(Scene* scene, float deltaTime) {
    if (!enabled || !scene) return;

    auto start = std::chrono::high_resolution_clock::now();

    if (useThreading) {
        UpdateMultiThreaded(scene, deltaTime);
    }
    else {
        UpdateSingleThreaded(scene, deltaTime);
    }

    auto end = std::chrono::high_resolution_clock::now();
    stats.lastUpdateTime = std::chrono::duration<float, std::milli>(end - start).count();

    UpdateFrameTimeAverage(stats.lastUpdateTime);
}

void UpdateSystem::LateUpdate(Scene* scene, float deltaTime) {
    if (!enabled || !scene) return;

    auto start = std::chrono::high_resolution_clock::now();

    if (useThreading) {
        LateUpdateMultiThreaded(scene, deltaTime);
    }
    else {
        LateUpdateSingleThreaded(scene, deltaTime);
    }

    auto end = std::chrono::high_resolution_clock::now();
    stats.lastLateUpdateTime = std::chrono::duration<float, std::milli>(end - start).count();
}

void UpdateSystem::FixedUpdate(Scene* scene, float deltaTime) {
    if (!enabled || !scene) return;

    fixedUpdateAccumulator += deltaTime;

    auto start = std::chrono::high_resolution_clock::now();

    while (fixedUpdateAccumulator >= fixedUpdateInterval) {
        if (useThreading) {
            FixedUpdateMultiThreaded(scene, fixedUpdateInterval);
        }
        else {
            FixedUpdateSingleThreaded(scene, fixedUpdateInterval);
        }

        fixedUpdateAccumulator -= fixedUpdateInterval;
    }

    auto end = std::chrono::high_resolution_clock::now();
    stats.lastFixedUpdateTime = std::chrono::duration<float, std::milli>(end - start).count();
}

// Data-Oriented batch processing (MAIN REQUIREMENT!)
void UpdateSystem::UpdateTransforms(std::vector<Transform*>& transforms, float deltaTime) {
    if (transforms.empty()) return;

    if (useThreading) {
        threadPool->UpdateTransforms(transforms, deltaTime);
    }
    else {
        for (Transform* transform : transforms) {
            if (transform) {
                transform->Update(deltaTime);
            }
        }
    }

    stats.transformsProcessed = transforms.size();
}

void UpdateSystem::UpdateBehaviors(std::vector<Behavior*>& behaviors, float deltaTime) {
    if (behaviors.empty()) return;

    if (useThreading) {
        threadPool->UpdateBehaviors(behaviors, deltaTime);
    }
    else {
        for (Behavior* behavior : behaviors) {
            if (behavior && behavior->IsActive()) {
                behavior->Update(deltaTime);
            }
        }
    }

    stats.behaviorsProcessed = behaviors.size();
}

void UpdateSystem::LateUpdateBehaviors(std::vector<Behavior*>& behaviors, float deltaTime) {
    if (behaviors.empty()) return;

    if (useThreading) {
        threadPool->ProcessBatch<Behavior>(behaviors, [deltaTime](Behavior* behavior) {
            if (behavior && behavior->IsActive()) {
                behavior->OnLateUpdate(deltaTime);
            }
            });
    }
    else {
        for (Behavior* behavior : behaviors) {
            if (behavior && behavior->IsActive()) {
                behavior->OnLateUpdate(deltaTime);
            }
        }
    }
}

void UpdateSystem::FixedUpdateBehaviors(std::vector<Behavior*>& behaviors, float deltaTime) {
    if (behaviors.empty()) return;

    if (useThreading) {
        threadPool->ProcessBatch<Behavior>(behaviors, [deltaTime](Behavior* behavior) {
            if (behavior && behavior->IsActive()) {
                behavior->OnFixedUpdate(deltaTime);
            }
            });
    }
    else {
        for (Behavior* behavior : behaviors) {
            if (behavior && behavior->IsActive()) {
                behavior->OnFixedUpdate(deltaTime);
            }
        }
    }
}

// Specialized batch operations
void UpdateSystem::TransformOperations(std::vector<Transform*>& transforms, const std::function<void(Transform*)>& operation) {
    if (transforms.empty()) return;

    if (useThreading) {
        threadPool->ProcessBatch<Transform>(transforms, operation);
    }
    else {
        for (Transform* transform : transforms) {
            if (transform) {
                operation(transform);
            }
        }
    }
}

void UpdateSystem::BehaviorOperations(std::vector<Behavior*>& behaviors, const std::function<void(Behavior*)>& operation) {
    if (behaviors.empty()) return;

    if (useThreading) {
        threadPool->ProcessBatch<Behavior>(behaviors, operation);
    }
    else {
        for (Behavior* behavior : behaviors) {
            if (behavior) {
                operation(behavior);
            }
        }
    }
}

// Parallel algorithms for common operations
void UpdateSystem::ParallelTranslate(std::vector<Transform*>& transforms, const Vector3& translation) {
    TransformOperations(transforms, [&translation](Transform* transform) {
        transform->Translate(translation);
        });
}

void UpdateSystem::ParallelRotate(std::vector<Transform*>& transforms, const Vector3& rotation) {
    TransformOperations(transforms, [&rotation](Transform* transform) {
        transform->Rotate(rotation);
        });
}

void UpdateSystem::ParallelScale(std::vector<Transform*>& transforms, float scale) {
    TransformOperations(transforms, [scale](Transform* transform) {
        transform->SetScale(scale);
        });
}

void UpdateSystem::CalculateDistances(std::vector<Transform*>& transforms, const Transform* target, std::vector<float>& outDistances) {
    if (!target || transforms.empty()) return;

    outDistances.resize(transforms.size());
    Vector3 targetPos = target->GetWorldPosition();

    if (useThreading) {
        threadPool->ProcessBatchRange<Transform>(transforms,
            [&targetPos, &outDistances](Transform** transformArray, size_t start, size_t end) {
                for (size_t i = start; i < end; ++i) {
                    if (transformArray[i]) {
                        Vector3 pos = transformArray[i]->GetWorldPosition();
                        Vector3 diff = pos - targetPos;
                        outDistances[i] = diff.Magnitude();
                    }
                    else {
                        outDistances[i] = -1.0f;
                    }
                }
            });
    }
    else {
        for (size_t i = 0; i < transforms.size(); ++i) {
            if (transforms[i]) {
                Vector3 pos = transforms[i]->GetWorldPosition();
                Vector3 diff = pos - targetPos;
                outDistances[i] = diff.Magnitude();
            }
            else {
                outDistances[i] = -1.0f;
            }
        }
    }
}

void UpdateSystem::FrustumCull(std::vector<Transform*>& transforms, std::vector<bool>& outVisibility) {
    if (transforms.empty()) return;

    outVisibility.resize(transforms.size());

    // Simple frustum culling implementation
    const float maxVisibleDistance = 100.0f;
    Vector3 cameraPos = Vector3::Zero; // Would get from camera system

    if (useThreading) {
        threadPool->ProcessBatchRange<Transform>(transforms,
            [&cameraPos, maxVisibleDistance, &outVisibility](Transform** transformArray, size_t start, size_t end) {
                for (size_t i = start; i < end; ++i) {
                    if (transformArray[i]) {
                        Vector3 pos = transformArray[i]->GetWorldPosition();
                        float distance = (pos - cameraPos).Magnitude();
                        outVisibility[i] = distance <= maxVisibleDistance;
                    }
                    else {
                        outVisibility[i] = false;
                    }
                }
            });
    }
    else {
        for (size_t i = 0; i < transforms.size(); ++i) {
            if (transforms[i]) {
                Vector3 pos = transforms[i]->GetWorldPosition();
                float distance = (pos - cameraPos).Magnitude();
                outVisibility[i] = distance <= maxVisibleDistance;
            }
            else {
                outVisibility[i] = false;
            }
        }
    }
}

// Performance tracking
void UpdateSystem::ResetStats() {
    stats = PerformanceStats();
}

void UpdateSystem::PrintPerformanceInfo() const {
    std::cout << "\n=== UpdateSystem Performance ===" << std::endl;
    std::cout << "Threading Enabled: " << (useThreading ? "Yes" : "No") << std::endl;
    std::cout << "Thread Count: " << threadPool->GetThreadCount() << std::endl;
    std::cout << "Last Update Time: " << stats.lastUpdateTime << "ms" << std::endl;
    std::cout << "Last LateUpdate Time: " << stats.lastLateUpdateTime << "ms" << std::endl;
    std::cout << "Last FixedUpdate Time: " << stats.lastFixedUpdateTime << "ms" << std::endl;
    std::cout << "Transforms Processed: " << stats.transformsProcessed << std::endl;
    std::cout << "Behaviors Processed: " << stats.behaviorsProcessed << std::endl;
    std::cout << "Average Frame Time: " << stats.averageFrameTime << "ms" << std::endl;
    std::cout << "Frame Count: " << stats.frameCount << std::endl;
}

void UpdateSystem::UpdateFrameTimeAverage(float frameTime) {
    stats.frameCount++;
    stats.averageFrameTime = (stats.averageFrameTime * (stats.frameCount - 1) + frameTime) / stats.frameCount;
}

// Internal update methods
void UpdateSystem::UpdateSingleThreaded(Scene* scene, float deltaTime) {
    // Traditional single-threaded update
    auto transforms = scene->GetAllTransforms();
    auto behaviors = scene->GetAllBehaviors();

    UpdateTransforms(transforms, deltaTime);
    UpdateBehaviors(behaviors, deltaTime);
}

void UpdateSystem::UpdateMultiThreaded(Scene* scene, float deltaTime) {
    // Data-Oriented multi-threaded update
    auto transforms = scene->GetAllTransforms();
    auto behaviors = scene->GetAllBehaviors();

    // Process transforms and behaviors in parallel
    auto transformFuture = threadPool->Enqueue([this, transforms = std::move(transforms), deltaTime]() mutable {
        UpdateTransforms(transforms, deltaTime);
        });

    auto behaviorFuture = threadPool->Enqueue([this, behaviors = std::move(behaviors), deltaTime]() mutable {
        UpdateBehaviors(behaviors, deltaTime);
        });

    // Wait for both to complete
    transformFuture.wait();
    behaviorFuture.wait();
}

void UpdateSystem::LateUpdateSingleThreaded(Scene* scene, float deltaTime) {
    auto behaviors = scene->GetAllBehaviors();
    LateUpdateBehaviors(behaviors, deltaTime);
}

void UpdateSystem::LateUpdateMultiThreaded(Scene* scene, float deltaTime) {
    auto behaviors = scene->GetAllBehaviors();
    LateUpdateBehaviors(behaviors, deltaTime);
}

void UpdateSystem::FixedUpdateSingleThreaded(Scene* scene, float deltaTime) {
    auto behaviors = scene->GetAllBehaviors();
    FixedUpdateBehaviors(behaviors, deltaTime);
}

void UpdateSystem::FixedUpdateMultiThreaded(Scene* scene, float deltaTime) {
    auto behaviors = scene->GetAllBehaviors();
    FixedUpdateBehaviors(behaviors, deltaTime);
}

// SystemManager implementation
static std::unique_ptr<SystemManager> g_systemManager = nullptr;

SystemManager& SystemManager::GetInstance() {
    if (!g_systemManager) {
        g_systemManager = std::make_unique<SystemManager>();
    }
    return *g_systemManager;
}

void SystemManager::Initialize(size_t numThreads) {
    if (initialized) return;

    updateSystem = std::make_unique<UpdateSystem>(numThreads);
    initialized = true;

    std::cout << "SystemManager initialized" << std::endl;
}

void SystemManager::Shutdown() {
    updateSystem.reset();
    initialized = false;
    std::cout << "SystemManager shut down" << std::endl;
}

UpdateSystem& SystemManager::GetUpdateSystem() {
    if (!initialized) {
        Initialize();
    }
    return *updateSystem;
}

void SystemManager::UpdateSystems(Scene* scene, float deltaTime) {
    if (initialized && updateSystem) {
        updateSystem->Update(scene, deltaTime);
    }
}

void SystemManager::LateUpdateSystems(Scene* scene, float deltaTime) {
    if (initialized && updateSystem) {
        updateSystem->LateUpdate(scene, deltaTime);
    }
}

void SystemManager::FixedUpdateSystems(Scene* scene, float deltaTime) {
    if (initialized && updateSystem) {
        updateSystem->FixedUpdate(scene, deltaTime);
    }
}

void SystemManager::PrintSystemInfo() const {
    std::cout << "\n=== SystemManager Info ===" << std::endl;
    std::cout << "Initialized: " << (initialized ? "Yes" : "No") << std::endl;

    if (initialized && updateSystem) {
        updateSystem->PrintPerformanceInfo();
    }
}