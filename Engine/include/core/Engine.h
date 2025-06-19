#pragma once

#include "SceneManager.h"
#include "../systems/UpdateSystem.h"
#include "../systems/ComponentManager.h"
#include "../memory/MemoryManager.h"
#include "../factories/ComponentFactory.h"
#include "../factories/GameObjectFactory.h"
#include <chrono>
#include <thread>
#include <atomic>
#include <string>

// Engine configuration
struct EngineConfig {
    // Threading configuration
    size_t threadCount = std::thread::hardware_concurrency();
    bool useMultiThreading = true;

    // Memory configuration
    size_t defaultPoolSize = 100;
    bool trackMemoryAllocations = true;

    // Performance configuration
    float targetFrameRate = 60.0f;
    float fixedUpdateRate = 60.0f;
    bool enableVSync = true;

    // Engine behavior
    bool pauseWhenUnfocused = true;
    bool enablePerformanceLogging = false;
    bool enableMemoryLogging = false;

    // Debug configuration
    bool enableDebugOutput = true;
    bool enableStatistics = true;

    EngineConfig() = default;
};

// Engine statistics
struct EngineStats {
    // Frame timing
    float currentFPS = 0.0f;
    float averageFPS = 0.0f;
    float frameTime = 0.0f;
    float averageFrameTime = 0.0f;

    // Update timing
    float updateTime = 0.0f;
    float lateUpdateTime = 0.0f;
    float fixedUpdateTime = 0.0f;

    // System statistics
    size_t totalGameObjects = 0;
    size_t activeGameObjects = 0;
    size_t totalComponents = 0;
    size_t activeComponents = 0;

    // Memory statistics
    size_t memoryUsage = 0;
    size_t peakMemoryUsage = 0;

    // Threading statistics
    size_t threadCount = 0;
    size_t activeTasks = 0;

    // Engine uptime
    float totalRunTime = 0.0f;
    size_t totalFrames = 0;

    void Reset() {
        *this = EngineStats();
    }
};

enum class EngineState {
    Uninitialized,
    Initializing,
    Running,
    Paused,
    Stopping,
    Stopped,
    Error
};

class Engine {
private:
    // Core systems
    SceneManager& sceneManager;
    SystemManager& systemManager; //defined in UpdateSystem.h/.cpp
    MemoryManager& memoryManager;
    ComponentFactory& componentFactory;
    GameObjectFactory& gameObjectFactory;

    // Engine state
    std::atomic<EngineState> state{ EngineState::Uninitialized };
    EngineConfig config;
    EngineStats stats;

    // Timing
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    float deltaTime = 0.0f;
    float fixedDeltaTime = 0.0f;

    // Frame rate control
    std::chrono::duration<float> targetFrameTime;

    // Performance tracking
    std::vector<float> frameTimeHistory;
    size_t frameTimeHistorySize = 60; // Track last 60 frames

    // Singleton instance
    static Engine* instance;

public:
    // Singleton access
    static Engine& GetInstance();
    static void DestroyInstance();

    // Constructor and destructor
    Engine();
    ~Engine();

    // Delete copy operations
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Engine lifecycle
    bool Initialize(const EngineConfig& engineConfig = EngineConfig());
    void Run();
    void Stop();
    void Pause();
    void Resume();
    void Shutdown();

    // Engine state queries
    EngineState GetState() const { return state.load(); }
    bool IsRunning() const { return state.load() == EngineState::Running; }
    bool IsPaused() const { return state.load() == EngineState::Paused; }
    bool IsInitialized() const { return state.load() != EngineState::Uninitialized; }

    // Configuration management
    void SetConfig(const EngineConfig& newConfig);
    const EngineConfig& GetConfig() const { return config; }

    // System access
    SceneManager& GetSceneManager() { return sceneManager; }
    SystemManager& GetSystemManager() { return systemManager; }
    MemoryManager& GetMemoryManager() { return memoryManager; }
    ComponentFactory& GetComponentFactory() { return componentFactory; }
    GameObjectFactory& GetGameObjectFactory() { return gameObjectFactory; }

    // High-level game development API
    Scene* CreateScene(const std::string& sceneName);
    bool LoadScene(const std::string& sceneName);
    Scene* GetCurrentScene();

    GameObject* CreateGameObject(const std::string& tag = "");
    GameObject* CreateGameObjectFromTemplate(const std::string& templateName);
    std::vector<GameObject*> FindGameObjectsWithTag(const std::string& tag);

    // Component management
    template<typename T>
    T* CreateComponent();

    template<typename T>
    std::vector<T*> GetAllComponentsOfType();

    // Performance and statistics
    const EngineStats& GetStats() const { return stats; }
    float GetDeltaTime() const { return deltaTime; }
    float GetFixedDeltaTime() const { return fixedDeltaTime; }
    float GetFPS() const { return stats.currentFPS; }
    float GetRunTime() const { return stats.totalRunTime; }

    // Debug and diagnostics
    void PrintEngineInfo() const;
    void PrintPerformanceStats() const;
    void PrintMemoryStats() const;
    void PrintSystemStats() const;
    void DumpCompleteReport() const;

    // Engine events (basic framework)
    using EngineEvent = std::function<void()>;
    void OnEngineStart(const EngineEvent& callback);
    void OnEngineStop(const EngineEvent& callback);
    void OnSceneChanged(const EngineEvent& callback);

private:
    // Main loop components
    void MainLoop();
    void UpdateFrame();
    void CalculateTiming();
    void UpdateStatistics();
    void HandleFrameRate();

    // Initialization helpers
    bool InitializeSystems();
    void InitializeFactories();
    void ConfigureSystems();

    // Shutdown helpers
    void ShutdownSystems();
    void CleanupResources();

    // Performance tracking
    void TrackFrameTime(float frameTime);
    void CalculateAverages();

    // Event handling
    std::vector<EngineEvent> startCallbacks;
    std::vector<EngineEvent> stopCallbacks;
    std::vector<EngineEvent> sceneChangeCallbacks;

    void TriggerStartCallbacks();
    void TriggerStopCallbacks();
    void TriggerSceneChangeCallbacks();
};

// Template implementations
template<typename T>
T* Engine::CreateComponent() {
    return componentFactory.CreateComponent<T>();
}

template<typename T>
std::vector<T*> Engine::GetAllComponentsOfType() {
    Scene* currentScene = GetCurrentScene();
    if (currentScene) {
        return currentScene->FindComponentsOfType<T>();
    }
    return std::vector<T*>();
}

// Global engine access macros
#define ENGINE Engine::GetInstance()
#define CURRENT_SCENE ENGINE.GetCurrentScene()
#define CREATE_GAMEOBJECT(tag) ENGINE.CreateGameObject(tag)
#define FIND_OBJECTS_WITH_TAG(tag) ENGINE.FindGameObjectsWithTag(tag)
#define DELTA_TIME ENGINE.GetDeltaTime()
#define ENGINE_FPS ENGINE.GetFPS()

// Engine helper functions
namespace EngineUtils {
    // Quick engine setup
    void QuickStart(const std::string& initialSceneName = "MainScene");
    void QuickShutdown();

    // Common configurations
    EngineConfig GetHighPerformanceConfig();
    EngineConfig GetDebugConfig();
    EngineConfig GetLowMemoryConfig();

    // Convenience functions
    void RunFor(float seconds);
    void RunFrames(size_t frameCount);

    // Performance helpers
    void EnablePerformanceProfiling(bool enable = true);
    void PrintQuickStats();
}