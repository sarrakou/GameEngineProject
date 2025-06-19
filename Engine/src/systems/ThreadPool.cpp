#include "../include/systems/ThreadPool.h"
#include "../include/components/Transform.h"
#include "../include/components/Behavior.h"
#include <iostream>
#include <algorithm>

ThreadPool::ThreadPool(size_t threads) : numThreads(threads) {
    // Ensure we have at least 1 thread
    if (numThreads == 0) {
        numThreads = 1;
    }

    // Create worker threads
    workers.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] { WorkerLoop(); });
    }

    std::cout << "ThreadPool initialized with " << numThreads << " threads" << std::endl;
}

ThreadPool::~ThreadPool() {
    // Signal all threads to stop
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }

    // Wake up all threads
    condition.notify_all();
    pauseCondition.notify_all();

    // Join all worker threads
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    std::cout << "ThreadPool destroyed" << std::endl;
}

void ThreadPool::EnqueueTask(Task task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);

        if (stop) {
            throw std::runtime_error("Enqueue on stopped ThreadPool");
        }

        tasks.emplace(std::move(task));
    }

    condition.notify_one();
}

// Specialized game engine batch processors
void ThreadPool::UpdateTransforms(std::vector<Transform*>& transforms, float deltaTime) {
    ProcessBatch<Transform>(transforms, [deltaTime](Transform* transform) {
        if (transform) {
            transform->Update(deltaTime);
        }
        });
}

void ThreadPool::UpdateBehaviors(std::vector<Behavior*>& behaviors, float deltaTime) {
    ProcessBatch<Behavior>(behaviors, [deltaTime](Behavior* behavior) {
        if (behavior && behavior->IsActive()) {
            behavior->Update(deltaTime);
        }
        });
}

void ThreadPool::UpdateComponents(std::vector<Component*>& components, float deltaTime) {
    ProcessBatch<Component>(components, [deltaTime](Component* component) {
        if (component && component->IsActive()) {
            component->Update(deltaTime);
        }
        });
}

void ThreadPool::WaitForCompletion() {
    while (activeTasks.load() > 0 || GetQueuedTaskCount() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

size_t ThreadPool::GetQueuedTaskCount() const {
    std::unique_lock<std::mutex> lock(queueMutex);
    return tasks.size();
}

void ThreadPool::Pause() {
    paused = true;
}

void ThreadPool::Resume() {
    paused = false;
    pauseCondition.notify_all();
}

// Private methods
void ThreadPool::WorkerLoop() {
    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            // Wait for a task or stop signal
            condition.wait(lock, [this] { return stop || !tasks.empty(); });

            if (stop && tasks.empty()) {
                return;
            }

            // Check if paused
            if (paused) {
                pauseCondition.wait(lock, [this] { return !paused || stop; });
                if (stop) return;
            }

            if (!tasks.empty()) {
                task = std::move(tasks.front());
                tasks.pop();
                activeTasks++;
            }
        }

        // Execute the task
        if (task) {
            try {
                task();
            }
            catch (const std::exception& e) {
                std::cerr << "ThreadPool task exception: " << e.what() << std::endl;
            }
            catch (...) {
                std::cerr << "ThreadPool task unknown exception" << std::endl;
            }

            activeTasks--;
        }
    }
}

size_t ThreadPool::CalculateOptimalBatchSize(size_t totalItems) const {
    if (totalItems == 0) return 1;

    // Rule of thumb: aim for 2-4 batches per thread to ensure good load balancing
    size_t targetBatches = numThreads * 3;
    size_t batchSize = std::max(static_cast < size_t>(1), totalItems / targetBatches);

    // Ensure batch size is reasonable (not too small, not too large)
    batchSize = std::max(batchSize, static_cast < size_t>(1));        // At least 1 item per batch
    batchSize = std::min(batchSize, static_cast < size_t>(1000));     // At most 1000 items per batch

    return batchSize;
}

// Specialized batch processing functions
namespace BatchProcessing {

    void UpdateTransformsBatch(Transform** transforms, size_t count, float deltaTime) {
        for (size_t i = 0; i < count; ++i) {
            if (transforms[i]) {
                transforms[i]->Update(deltaTime);
            }
        }
    }

    void TranslateTransformsBatch(Transform** transforms, size_t count, float x, float y, float z) {
        Vector3 translation(x, y, z);
        for (size_t i = 0; i < count; ++i) {
            if (transforms[i]) {
                transforms[i]->Translate(translation);
            }
        }
    }

    void RotateTransformsBatch(Transform** transforms, size_t count, float x, float y, float z) {
        Vector3 rotation(x, y, z);
        for (size_t i = 0; i < count; ++i) {
            if (transforms[i]) {
                transforms[i]->Rotate(rotation);
            }
        }
    }

    void ScaleTransformsBatch(Transform** transforms, size_t count, float scale) {
        for (size_t i = 0; i < count; ++i) {
            if (transforms[i]) {
                transforms[i]->SetScale(scale);
            }
        }
    }

    void UpdateBehaviorsBatch(Behavior** behaviors, size_t count, float deltaTime) {
        for (size_t i = 0; i < count; ++i) {
            if (behaviors[i] && behaviors[i]->IsActive()) {
                behaviors[i]->Update(deltaTime);
            }
        }
    }

    void StartBehaviorsBatch(Behavior** behaviors, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            if (behaviors[i] && behaviors[i]->IsActive()) {
                behaviors[i]->Start();
            }
        }
    }

    void CalculateDistancesBatch(Transform** transforms, size_t count, const Transform* target, std::vector<float>& distances) {
        if (!target || distances.size() < count) return;

        Vector3 targetPos = target->GetWorldPosition();

        for (size_t i = 0; i < count; ++i) {
            if (transforms[i]) {
                Vector3 pos = transforms[i]->GetWorldPosition();
                Vector3 diff = pos - targetPos;
                distances[i] = diff.Magnitude();
            }
            else {
                distances[i] = -1.0f; // Invalid distance
            }
        }
    }

    void FrustumCullBatch(Transform** transforms, size_t count, std::vector<bool>& visibility) {
        if (visibility.size() < count) return;

        // Simple frustum culling based on distance (placeholder implementation)
        // In a real engine, this would use camera frustum planes
        const float maxVisibleDistance = 100.0f;
        Vector3 cameraPos = Vector3::Zero; // Would get from camera

        for (size_t i = 0; i < count; ++i) {
            if (transforms[i]) {
                Vector3 pos = transforms[i]->GetWorldPosition();
                float distance = (pos - cameraPos).Magnitude();
                visibility[i] = distance <= maxVisibleDistance;
            }
            else {
                visibility[i] = false;
            }
        }
    }
}

// Global ThreadPool instance (would normally be owned by Engine)
static std::unique_ptr<ThreadPool> g_threadPool = nullptr;

// Global ThreadPool access functions
namespace ThreadPoolManager {
    void Initialize(size_t numThreads = std::thread::hardware_concurrency()) {
        if (!g_threadPool) {
            g_threadPool = std::make_unique<ThreadPool>(numThreads);
        }
    }

    void Shutdown() {
        g_threadPool.reset();
    }

    ThreadPool& GetInstance() {
        if (!g_threadPool) {
            Initialize();
        }
        return *g_threadPool;
    }

    // Convenience functions for common operations
    void UpdateAllTransforms(std::vector<Transform*>& transforms, float deltaTime) {
        GetInstance().UpdateTransforms(transforms, deltaTime);
    }

    void UpdateAllBehaviors(std::vector<Behavior*>& behaviors, float deltaTime) {
        GetInstance().UpdateBehaviors(behaviors, deltaTime);
    }

    template<typename T>
    void ProcessInParallel(std::vector<T*>& items, std::function<void(T*)> processor) {
        GetInstance().ProcessBatch(items, processor);
    }
}