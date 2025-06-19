#include "../include/core/Engine.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

// Static instance initialization
Engine* Engine::instance = nullptr;

Engine& Engine::GetInstance() {
    if (instance == nullptr) {
        instance = new Engine();
    }
    return *instance;
}

void Engine::DestroyInstance() {
    if (instance) {
        instance->Shutdown();
        delete instance;
        instance = nullptr;
    }
}

Engine::Engine()
    : sceneManager(SceneManager::GetInstance())
    , systemManager(SystemManager::GetInstance())
    , memoryManager(MemoryManager::GetInstance())
    , componentFactory(ComponentFactory::GetInstance())
    , gameObjectFactory(GameObjectFactory::GetInstance()) {

    frameTimeHistory.reserve(frameTimeHistorySize);
    std::cout << "Engine instance created" << std::endl;
}

Engine::~Engine() {
    if (state.load() != EngineState::Stopped && state.load() != EngineState::Uninitialized) {
        Shutdown();
    }
    std::cout << "Engine instance destroyed" << std::endl;
}

// Engine lifecycle
bool Engine::Initialize(const EngineConfig& engineConfig) {
    if (state.load() != EngineState::Uninitialized) {
        std::cerr << "Engine already initialized" << std::endl;
        return false;
    }

    state = EngineState::Initializing;
    config = engineConfig;

    std::cout << "\n=== Initializing Game Engine ===" << std::endl;

    // Initialize timing
    targetFrameTime = std::chrono::duration<float>(1.0f / config.targetFrameRate);
    fixedDeltaTime = 1.0f / config.fixedUpdateRate;

    // Initialize all systems
    if (!InitializeSystems()) {
        state = EngineState::Error;
        std::cerr << "Failed to initialize engine systems" << std::endl;
        return false;
    }

    // Initialize factories
    InitializeFactories();

    // Configure systems
    ConfigureSystems();

    // Initialize timing
    startTime = std::chrono::high_resolution_clock::now();
    lastFrameTime = startTime;

    state = EngineState::Stopped;
    std::cout << "Engine initialized successfully!" << std::endl;

    return true;
}

void Engine::Run() {
    if (state.load() != EngineState::Stopped) {
        std::cerr << "Engine must be stopped to run" << std::endl;
        return;
    }

    std::cout << "\n=== Starting Game Engine ===" << std::endl;
    state = EngineState::Running;

    TriggerStartCallbacks();

    // Reset timing
    lastFrameTime = std::chrono::high_resolution_clock::now();

    // Main game loop
    MainLoop();

    std::cout << "Engine stopped" << std::endl;
}

void Engine::Stop() {
    if (state.load() == EngineState::Running || state.load() == EngineState::Paused) {
        std::cout << "Stopping engine..." << std::endl;
        state = EngineState::Stopping;
    }
}

void Engine::Pause() {
    if (state.load() == EngineState::Running) {
        state = EngineState::Paused;
        std::cout << "Engine paused" << std::endl;
    }
}

void Engine::Resume() {
    if (state.load() == EngineState::Paused) {
        state = EngineState::Running;
        lastFrameTime = std::chrono::high_resolution_clock::now(); // Reset timing
        std::cout << "Engine resumed" << std::endl;
    }
}

void Engine::Shutdown() {
    if (state.load() == EngineState::Uninitialized) {
        return;
    }

    std::cout << "\n=== Shutting Down Game Engine ===" << std::endl;

    // Stop if running
    if (state.load() == EngineState::Running || state.load() == EngineState::Paused) {
        Stop();
        // Wait a moment for main loop to finish
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    TriggerStopCallbacks();

    // Shutdown systems
    ShutdownSystems();

    // Cleanup resources
    CleanupResources();

    state = EngineState::Uninitialized;
    std::cout << "Engine shutdown complete" << std::endl;
}

// Configuration management
void Engine::SetConfig(const EngineConfig& newConfig) {
    config = newConfig;

    // Update systems with new configuration
    if (IsInitialized()) {
        ConfigureSystems();

        // Update timing
        targetFrameTime = std::chrono::duration<float>(1.0f / config.targetFrameRate);
        fixedDeltaTime = 1.0f / config.fixedUpdateRate;
    }
}

// High-level game development API
Scene* Engine::CreateScene(const std::string& sceneName) {
    return sceneManager.CreateScene(sceneName);
}

bool Engine::LoadScene(const std::string& sceneName) {
    bool result = sceneManager.LoadScene(sceneName);
    if (result) {
        TriggerSceneChangeCallbacks();
    }
    return result;
}

Scene* Engine::GetCurrentScene() {
    return sceneManager.GetCurrentScene();
}

GameObject* Engine::CreateGameObject(const std::string& tag) {
    return sceneManager.CreateGameObject(tag);
}

GameObject* Engine::CreateGameObjectFromTemplate(const std::string& templateName) {
    auto result = gameObjectFactory.CreateGameObject(templateName);
    if (result.success && result.gameObject) {
        Scene* currentScene = GetCurrentScene();
        if (currentScene) {
            GameObject* ptr = result.gameObject.get();
            currentScene->AddGameObject(std::move(result.gameObject));
            return ptr;
        }
    }
    return nullptr;
}

std::vector<GameObject*> Engine::FindGameObjectsWithTag(const std::string& tag) {
    return sceneManager.FindGameObjectsWithTag(tag);
}

// Debug and diagnostics
void Engine::PrintEngineInfo() const {
    std::cout << "\n=== Engine Information ===" << std::endl;
    std::cout << "State: ";
    switch (state.load()) {
    case EngineState::Uninitialized: std::cout << "Uninitialized"; break;
    case EngineState::Initializing: std::cout << "Initializing"; break;
    case EngineState::Running: std::cout << "Running"; break;
    case EngineState::Paused: std::cout << "Paused"; break;
    case EngineState::Stopping: std::cout << "Stopping"; break;
    case EngineState::Stopped: std::cout << "Stopped"; break;
    case EngineState::Error: std::cout << "Error"; break;
    }
    std::cout << std::endl;

    std::cout << "Configuration:" << std::endl;
    std::cout << "  Thread Count: " << config.threadCount << std::endl;
    std::cout << "  Multi-Threading: " << (config.useMultiThreading ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  Target FPS: " << config.targetFrameRate << std::endl;
    std::cout << "  Fixed Update Rate: " << config.fixedUpdateRate << std::endl;
    std::cout << "  VSync: " << (config.enableVSync ? "Enabled" : "Disabled") << std::endl;
}

void Engine::PrintPerformanceStats() const {
    std::cout << "\n=== Performance Statistics ===" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Current FPS: " << stats.currentFPS << std::endl;
    std::cout << "Average FPS: " << stats.averageFPS << std::endl;
    std::cout << "Frame Time: " << stats.frameTime << "ms" << std::endl;
    std::cout << "Average Frame Time: " << stats.averageFrameTime << "ms" << std::endl;
    std::cout << "Update Time: " << stats.updateTime << "ms" << std::endl;
    std::cout << "Late Update Time: " << stats.lateUpdateTime << "ms" << std::endl;
    std::cout << "Fixed Update Time: " << stats.fixedUpdateTime << "ms" << std::endl;
    std::cout << "Total Frames: " << stats.totalFrames << std::endl;
    std::cout << "Total Run Time: " << stats.totalRunTime << "s" << std::endl;
}

void Engine::PrintMemoryStats() const {
    std::cout << "\n=== Memory Statistics ===" << std::endl;
    memoryManager.PrintMemoryStats();
}

void Engine::PrintSystemStats() const {
    std::cout << "\n=== System Statistics ===" << std::endl;
    std::cout << "Total GameObjects: " << stats.totalGameObjects << std::endl;
    std::cout << "Active GameObjects: " << stats.activeGameObjects << std::endl;
    std::cout << "Total Components: " << stats.totalComponents << std::endl;
    std::cout << "Active Components: " << stats.activeComponents << std::endl;
    std::cout << "Thread Count: " << stats.threadCount << std::endl;
    std::cout << "Active Tasks: " << stats.activeTasks << std::endl;
}

void Engine::DumpCompleteReport() const {
    PrintEngineInfo();
    PrintPerformanceStats();
    PrintSystemStats();
    PrintMemoryStats();

    std::cout << "\n=== Scene Information ===" << std::endl;
    sceneManager.PrintSceneInfo();

    std::cout << "\n=== Component Factory ===" << std::endl;
    componentFactory.PrintFactoryInfo();

    std::cout << "\n=== GameObject Factory ===" << std::endl;
    gameObjectFactory.PrintFactoryInfo();
}

// Private implementation
void Engine::MainLoop() {
    while (state.load() == EngineState::Running || state.load() == EngineState::Paused) {
        // Handle pause state
        if (state.load() == EngineState::Paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS even when paused
            continue;
        }

        // Calculate timing
        CalculateTiming();

        // Update frame
        UpdateFrame();

        // Update statistics
        UpdateStatistics();

        // Handle frame rate limiting
        HandleFrameRate();

        // Check for stop signal
        if (state.load() == EngineState::Stopping) {
            break;
        }
    }

    state = EngineState::Stopped;
    TriggerStopCallbacks();
}

void Engine::UpdateFrame() {
    auto frameStart = std::chrono::high_resolution_clock::now();

    Scene* currentScene = GetCurrentScene();
    if (!currentScene) {
        return; // No scene to update
    }

    // Update systems (MAIN REQUIREMENT #5: THREADED UPDATES!)
    auto updateStart = std::chrono::high_resolution_clock::now();
    systemManager.UpdateSystems(currentScene, deltaTime);
    auto updateEnd = std::chrono::high_resolution_clock::now();

    // Late update
    auto lateUpdateStart = std::chrono::high_resolution_clock::now();
    systemManager.LateUpdateSystems(currentScene, deltaTime);
    auto lateUpdateEnd = std::chrono::high_resolution_clock::now();

    // Fixed update
    auto fixedUpdateStart = std::chrono::high_resolution_clock::now();
    systemManager.FixedUpdateSystems(currentScene, fixedDeltaTime);
    auto fixedUpdateEnd = std::chrono::high_resolution_clock::now();

    // Calculate timing
    stats.updateTime = std::chrono::duration<float, std::milli>(updateEnd - updateStart).count();
    stats.lateUpdateTime = std::chrono::duration<float, std::milli>(lateUpdateEnd - lateUpdateStart).count();
    stats.fixedUpdateTime = std::chrono::duration<float, std::milli>(fixedUpdateEnd - fixedUpdateStart).count();

    auto frameEnd = std::chrono::high_resolution_clock::now();
    stats.frameTime = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();

    TrackFrameTime(stats.frameTime);
}

void Engine::CalculateTiming() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    stats.totalRunTime = std::chrono::duration<float>(currentTime - startTime).count();
}

void Engine::UpdateStatistics() {
    stats.totalFrames++;

    // Calculate FPS
    if (deltaTime > 0.0f) {
        stats.currentFPS = 1.0f / deltaTime;
    }

    // Update averages
    CalculateAverages();

    // Update system statistics
    Scene* currentScene = GetCurrentScene();
    if (currentScene) {
        stats.totalGameObjects = currentScene->GetGameObjectCount();
        stats.activeGameObjects = currentScene->GetActiveGameObjectCount();
    }

    // Update memory statistics
    stats.memoryUsage = memoryManager.GetCurrentUsage();
    stats.peakMemoryUsage = memoryManager.GetPeakUsage();

    // Update threading statistics
    stats.threadCount = config.threadCount;
    if (systemManager.IsInitialized()) {
        auto& updateSystem = systemManager.GetUpdateSystem();
        stats.activeTasks = updateSystem.GetThreadPool().GetActiveTaskCount();
    }

    // Performance logging
    if (config.enablePerformanceLogging && stats.totalFrames % 60 == 0) {
        std::cout << "[PERF] FPS: " << std::fixed << std::setprecision(1)
            << stats.currentFPS << " | Frame: " << std::setprecision(2)
            << stats.frameTime << "ms | Objects: " << stats.activeGameObjects << std::endl;
    }

    // Memory logging
    if (config.enableMemoryLogging && stats.totalFrames % 300 == 0) {
        std::cout << "[MEM] Usage: " << stats.memoryUsage
            << " bytes | Peak: " << stats.peakMemoryUsage << " bytes" << std::endl;
    }
}

void Engine::HandleFrameRate() {
    if (!config.enableVSync) {
        return;
    }

    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration = frameEnd - lastFrameTime;

    if (frameDuration < targetFrameTime) {
        auto sleepTime = targetFrameTime - frameDuration;
        std::this_thread::sleep_for(sleepTime);
    }
}

// Initialization helpers
bool Engine::InitializeSystems() {
    try {
        // Initialize memory manager first
        memoryManager.SetTrackAllocations(config.trackMemoryAllocations);
        memoryManager.SetDefaultPoolSize(config.defaultPoolSize);

        // Initialize system manager with threading configuration
        systemManager.Initialize(config.threadCount);

        std::cout << "All systems initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "System initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void Engine::InitializeFactories() {
    // Factories are already initialized via singleton pattern
    // Just print confirmation
    std::cout << "Factories initialized:" << std::endl;
    std::cout << "  - ComponentFactory: " << componentFactory.GetRegisteredComponentCount() << " types" << std::endl;
    std::cout << "  - GameObjectFactory: " << gameObjectFactory.GetTemplateCount() << " templates" << std::endl;
}

void Engine::ConfigureSystems() {
    // Configure update system
    if (systemManager.IsInitialized()) {
        auto& updateSystem = systemManager.GetUpdateSystem();
        updateSystem.SetThreadingEnabled(config.useMultiThreading);
        updateSystem.SetFixedUpdateRate(config.fixedUpdateRate);
    }
}

void Engine::ShutdownSystems() {
    systemManager.Shutdown();
}

void Engine::CleanupResources() {
    frameTimeHistory.clear();
    startCallbacks.clear();
    stopCallbacks.clear();
    sceneChangeCallbacks.clear();
}

// Performance tracking
void Engine::TrackFrameTime(float frameTime) {
    frameTimeHistory.push_back(frameTime);

    if (frameTimeHistory.size() > frameTimeHistorySize) {
        frameTimeHistory.erase(frameTimeHistory.begin());
    }
}

void Engine::CalculateAverages() {
    if (frameTimeHistory.empty()) return;

    float totalFrameTime = 0.0f;
    for (float time : frameTimeHistory) {
        totalFrameTime += time;
    }

    stats.averageFrameTime = totalFrameTime / frameTimeHistory.size();
    stats.averageFPS = 1000.0f / stats.averageFrameTime; // Convert ms to FPS
}

// Event handling
void Engine::OnEngineStart(const EngineEvent& callback) {
    startCallbacks.push_back(callback);
}

void Engine::OnEngineStop(const EngineEvent& callback) {
    stopCallbacks.push_back(callback);
}

void Engine::OnSceneChanged(const EngineEvent& callback) {
    sceneChangeCallbacks.push_back(callback);
}

void Engine::TriggerStartCallbacks() {
    for (const auto& callback : startCallbacks) {
        callback();
    }
}

void Engine::TriggerStopCallbacks() {
    for (const auto& callback : stopCallbacks) {
        callback();
    }
}

void Engine::TriggerSceneChangeCallbacks() {
    for (const auto& callback : sceneChangeCallbacks) {
        callback();
    }
}

// EngineUtils implementation
namespace EngineUtils {

    void QuickStart(const std::string& initialSceneName) {
        Engine& engine = Engine::GetInstance();

        // Initialize with default config
        EngineConfig config;
        engine.Initialize(config);

        // Create initial scene
        Scene* scene = engine.CreateScene(initialSceneName);
        engine.LoadScene(initialSceneName);

        std::cout << "Engine quick-started with scene: " << initialSceneName << std::endl;
    }

    void QuickShutdown() {
        Engine::DestroyInstance();
    }

    EngineConfig GetHighPerformanceConfig() {
        EngineConfig config;
        config.threadCount = std::thread::hardware_concurrency();
        config.useMultiThreading = true;
        config.targetFrameRate = 120.0f;
        config.fixedUpdateRate = 60.0f;
        config.enableVSync = false;
        config.trackMemoryAllocations = false;
        config.enableDebugOutput = false;
        return config;
    }

    EngineConfig GetDebugConfig() {
        EngineConfig config;
        config.threadCount = 1; // Single-threaded for easier debugging
        config.useMultiThreading = false;
        config.targetFrameRate = 30.0f;
        config.enablePerformanceLogging = true;
        config.enableMemoryLogging = true;
        config.enableDebugOutput = true;
        config.enableStatistics = true;
        return config;
    }

    EngineConfig GetLowMemoryConfig() {
        EngineConfig config;
        config.defaultPoolSize = 50; // Smaller pools
        config.trackMemoryAllocations = true;
        config.enableMemoryLogging = true;
        return config;
    }

    void RunFor(float seconds) {
        Engine& engine = Engine::GetInstance();

        auto startTime = std::chrono::high_resolution_clock::now();
        engine.Run();

        // This would require engine modification to support time-limited runs
        // For now, just print a message
        std::cout << "RunFor not fully implemented - would run for " << seconds << " seconds" << std::endl;
    }

    void RunFrames(size_t frameCount) {
        std::cout << "RunFrames not fully implemented - would run for " << frameCount << " frames" << std::endl;
    }

    void EnablePerformanceProfiling(bool enable) {
        Engine& engine = Engine::GetInstance();
        EngineConfig config = engine.GetConfig();
        config.enablePerformanceLogging = enable;
        config.enableStatistics = enable;
        engine.SetConfig(config);
    }

    void PrintQuickStats() {
        Engine& engine = Engine::GetInstance();
        const auto& stats = engine.GetStats();

        std::cout << "Quick Stats - FPS: " << std::fixed << std::setprecision(1)
            << stats.currentFPS << " | Objects: " << stats.activeGameObjects
            << " | Memory: " << stats.memoryUsage << " bytes" << std::endl;
    }
}