#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>

// Forward declarations
class Transform;
class Behavior;
class Component;

// Task types for the thread pool
using Task = std::function<void()>;
using TaskQueue = std::queue<Task>;

class ThreadPool {
private:
    std::vector<std::thread> workers;
    TaskQueue tasks;

    // Synchronization
    mutable std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop{ false };
    std::atomic<int> activeTasks{ 0 };

    // Thread pool configuration
    size_t numThreads;

public:
    // Constructor and destructor
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Delete copy operations (thread pools should be unique)
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Task submission
    template<typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

    void EnqueueTask(Task task);

    // Batch processing for Data-Oriented Design
    template<typename T>
    void ProcessBatch(std::vector<T*>& items, std::function<void(T*)> processor, size_t batchSize = 0);

    template<typename T>
    void ProcessBatchRange(std::vector<T*>& items, std::function<void(T**, size_t, size_t)> processor, size_t batchSize = 0);

    // Specialized game engine batch processors
    void UpdateTransforms(std::vector<Transform*>& transforms, float deltaTime);
    void UpdateBehaviors(std::vector<Behavior*>& behaviors, float deltaTime);
    void UpdateComponents(std::vector<Component*>& components, float deltaTime);

    // Wait for all tasks to complete
    void WaitForCompletion();

    // Thread pool info
    size_t GetThreadCount() const { return numThreads; }
    size_t GetActiveTaskCount() const { return activeTasks.load(); }
    size_t GetQueuedTaskCount() const;

    // Thread pool control
    void Pause();
    void Resume();
    bool IsPaused() const { return paused.load(); }

private:
    std::atomic<bool> paused{ false };
    std::condition_variable pauseCondition;

    // Worker thread function
    void WorkerLoop();

    // Batch size calculation
    size_t CalculateOptimalBatchSize(size_t totalItems) const;
};

// Specialized batch processing functions for game engine components
namespace BatchProcessing {
    // Transform batch operations
    void UpdateTransformsBatch(Transform** transforms, size_t count, float deltaTime);
    void TranslateTransformsBatch(Transform** transforms, size_t count, float x, float y, float z);
    void RotateTransformsBatch(Transform** transforms, size_t count, float x, float y, float z);
    void ScaleTransformsBatch(Transform** transforms, size_t count, float scale);

    // Behavior batch operations  
    void UpdateBehaviorsBatch(Behavior** behaviors, size_t count, float deltaTime);
    void StartBehaviorsBatch(Behavior** behaviors, size_t count);

    // Distance calculations (useful for AI, physics, etc.)
    void CalculateDistancesBatch(Transform** transforms, size_t count, const Transform* target, std::vector<float>& distances);

    // Frustum culling batch (for rendering optimization)
    void FrustumCullBatch(Transform** transforms, size_t count, std::vector<bool>& visibility);
}

// Template implementations
template<typename F, typename... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> result = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queueMutex);

        if (stop) {
            throw std::runtime_error("Enqueue on stopped ThreadPool");
        }

        tasks.emplace([task]() { (*task)(); });
    }

    condition.notify_one();
    return result;
}

template<typename T>
void ThreadPool::ProcessBatch(std::vector<T*>& items, std::function<void(T*)> processor, size_t batchSize) {
    if (items.empty()) return;

    if (batchSize == 0) {
        batchSize = CalculateOptimalBatchSize(items.size());
    }

    std::vector<std::future<void>> futures;

    for (size_t i = 0; i < items.size(); i += batchSize) {
        size_t end = std::min(i + batchSize, items.size());

        auto future = Enqueue([&items, processor, i, end]() {
            for (size_t j = i; j < end; ++j) {
                if (items[j]) {
                    processor(items[j]);
                }
            }
            });

        futures.push_back(std::move(future));
    }

    // Wait for all batches to complete
    for (auto& future : futures) {
        future.wait();
    }
}

template<typename T>
void ThreadPool::ProcessBatchRange(std::vector<T*>& items, std::function<void(T**, size_t, size_t)> processor, size_t batchSize) {
    if (items.empty()) return;

    if (batchSize == 0) {
        batchSize = CalculateOptimalBatchSize(items.size());
    }

    std::vector<std::future<void>> futures;

    for (size_t i = 0; i < items.size(); i += batchSize) {
        size_t end = std::min(i + batchSize, items.size());

        auto future = Enqueue([&items, processor, i, end]() {
            processor(items.data(), i, end);
            });

        futures.push_back(std::move(future));
    }

    // Wait for all batches to complete
    for (auto& future : futures) {
        future.wait();
    }
}