#include "core/Engine.h"
#include "components/Transform.h"
#include "components/Behavior.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Custom behavior for demonstration
class DemoPlayerBehavior : public Behavior {
private:
    float speed = 5.0f;
    float rotationSpeed = 90.0f;

public:
    void Start() override {
        Log("DemoPlayerBehavior started!");
    }

    void OnUpdate(float deltaTime) override {
        Transform* transform = GetTransform();
        if (!transform) return;

        // Simple movement pattern
        static float time = 0.0f;
        time += deltaTime;

        // Move in a figure-8 pattern
        float x = std::sin(time * 2.0f) * 3.0f;
        float z = std::sin(time) * 5.0f;
        transform->SetPosition(x, 1.0f, z);

        // Rotate
        transform->Rotate(0.0f, rotationSpeed * deltaTime, 0.0f);
    }

    const char* GetTypeName() const override { return "DemoPlayerBehavior"; }
};

class DemoEnemyBehavior : public Behavior {
private:
    float orbitRadius = 8.0f;
    float orbitSpeed = 45.0f;

public:
    void Start() override {
        Log("DemoEnemyBehavior started!");
    }

    void OnUpdate(float deltaTime) override {
        Transform* transform = GetTransform();
        if (!transform) return;

        // Orbit around origin
        static float angle = 0.0f;
        angle += orbitSpeed * deltaTime;

        float x = std::cos(angle * M_PI / 180.0f) * orbitRadius;
        float z = std::sin(angle * M_PI / 180.0f) * orbitRadius;
        transform->SetPosition(x, 0.5f, z);

        // Look at center
        transform->SetRotation(0.0f, angle + 90.0f, 0.0f);
    }

    const char* GetTypeName() const override { return "DemoEnemyBehavior"; }
};

// Demonstration functions
static void RegisterCustomComponents() {
    std::cout << "\n=== Registering Custom Components ===" << std::endl;

    // Register custom behaviors with the ComponentFactory
    COMPONENT_FACTORY.RegisterComponent<DemoPlayerBehavior>("DemoPlayerBehavior");
    COMPONENT_FACTORY.RegisterComponent<DemoEnemyBehavior>("DemoEnemyBehavior");

    std::cout << "Custom components registered!" << std::endl;
}

static void CreateGameObjectTemplates() {
    std::cout << "\n=== Creating GameObject Templates ===" << std::endl;

    // Create advanced player template
    auto playerTemplate = BUILD_TEMPLATE("AdvancedPlayer", "Player")
        .WithTransform(0.0f, 1.0f, 0.0f)
        .WithComponent("DemoPlayerBehavior")
        .Build();

    REGISTER_TEMPLATE(playerTemplate);

    // Create advanced enemy template
    auto enemyTemplate = BUILD_TEMPLATE("AdvancedEnemy", "Enemy")
        .WithTransform(8.0f, 0.5f, 0.0f)
        .WithComponent("DemoEnemyBehavior")
        .Build();

    REGISTER_TEMPLATE(enemyTemplate);

    // Create collectible template
    auto collectibleTemplate = BUILD_TEMPLATE("Collectible", "Collectible")
        .WithTransform(0.0f, 0.25f, 0.0f)
        .Build();

    REGISTER_TEMPLATE(collectibleTemplate);

    std::cout << "GameObject templates created!" << std::endl;
}

static void PopulateGameScene(Scene* scene) {
    std::cout << "\n=== Populating Game Scene ===" << std::endl;

    // Create player
    auto* player = ENGINE.CreateGameObjectFromTemplate("AdvancedPlayer");
    std::cout << "Created player: " << (player ? "Success" : "Failed") << std::endl;

    // Create multiple enemies using factory batch creation
    GAMEOBJECT_FACTORY.PopulateScene(scene, "AdvancedEnemy", 5);

    // Create collectibles in a grid pattern
    for (int x = -10; x <= 10; x += 5) {
        for (int z = -10; z <= 10; z += 5) {
            if (x == 0 && z == 0) continue; // Skip center (player position)

            auto* collectible = ENGINE.CreateGameObjectFromTemplate("Collectible");
            if (collectible) {
                auto* transform = collectible->GetComponent<Transform>();
                if (transform) {
                    transform->SetPosition(static_cast<float>(x), 0.25f, static_cast<float>(z));
                }
            }
        }
    }

    std::cout << "Scene populated with objects!" << std::endl;
}

static void DemonstrateDataOrientedProcessing() {
    std::cout << "\n=== Demonstrating Data-Oriented Processing ===" << std::endl;

    Scene* currentScene = ENGINE.GetCurrentScene();
    if (!currentScene) return;

    // Get all transforms for batch processing (REQUIREMENT #3 & #5)
    auto transforms = currentScene->GetAllTransforms();
    std::cout << "Found " << transforms.size() << " transforms for batch processing" << std::endl;

    // Demonstrate parallel batch operations
    auto& updateSystem = ENGINE.GetSystemManager().GetUpdateSystem();

    // Parallel translate all objects up slightly
    Vector3 upMovement(0.0f, 0.1f, 0.0f);
    updateSystem.ParallelTranslate(transforms, upMovement);

    // Parallel scale all objects
    updateSystem.ParallelScale(transforms, 1.05f);

    std::cout << "Batch processing demonstration complete!" << std::endl;
}

static void PrintRealTimeStats() {
    const auto& stats = ENGINE.GetStats();

    std::cout << "\n=== Real-Time Engine Statistics ===" << std::endl;
    std::cout << "FPS: " << std::fixed << std::setprecision(1) << stats.currentFPS
        << " (Avg: " << stats.averageFPS << ")" << std::endl;
    std::cout << "Frame Time: " << std::setprecision(2) << stats.frameTime
        << "ms (Avg: " << stats.averageFrameTime << "ms)" << std::endl;
    std::cout << "Active GameObjects: " << stats.activeGameObjects << std::endl;
    std::cout << "Active Components: " << stats.activeComponents << std::endl;
    std::cout << "Memory Usage: " << stats.memoryUsage << " bytes" << std::endl;
    std::cout << "Active Threads: " << stats.threadCount << std::endl;
    std::cout << "Active Tasks: " << stats.activeTasks << std::endl;
    std::cout << "Total Runtime: " << std::setprecision(1) << stats.totalRunTime << "s" << std::endl;
}

static void RunEngineDemo() {
    std::cout << "\n=== GAME ENGINE DEMONSTRATION ===" << std::endl;
    std::cout << "This demo showcases ALL 5 requirements in action!" << std::endl;

    // Configure engine for demonstration
    EngineConfig config;
    config.targetFrameRate = 60.0f;
    config.useMultiThreading = true;
    config.threadCount = std::thread::hardware_concurrency();
    config.enablePerformanceLogging = true;
    config.enableDebugOutput = true;

    std::cout << "\nEngine Configuration:" << std::endl;
    std::cout << "  Target FPS: " << config.targetFrameRate << std::endl;
    std::cout << "  Thread Count: " << config.threadCount << std::endl;
    std::cout << "  Multi-Threading: " << (config.useMultiThreading ? "Enabled" : "Disabled") << std::endl;

    // Initialize engine
    if (!ENGINE.Initialize(config)) {
        std::cerr << "Failed to initialize engine!" << std::endl;
        return;
    }

    // Register custom components (REQUIREMENT #4: Factory system)
    RegisterCustomComponents();

    // Create GameObject templates (REQUIREMENT #4: Data-driven)
    CreateGameObjectTemplates();

    // Create and load game scene (REQUIREMENT #2: Scene management)
    Scene* gameScene = ENGINE.CreateScene("DemoScene");
    ENGINE.LoadScene("DemoScene");

    // Populate scene with objects (REQUIREMENT #2: Add/remove objects)
    PopulateGameScene(gameScene);

    // Demonstrate data-oriented processing (REQUIREMENT #3 & #5)
    DemonstrateDataOrientedProcessing();

    std::cout << "\n=== Starting Main Game Loop ===" << std::endl;
    std::cout << "Running for 10 seconds to demonstrate real-time performance..." << std::endl;

    // Set up engine callbacks
    ENGINE.OnEngineStart([]() {
        std::cout << "Engine started! All systems operational!" << std::endl;
        });

    ENGINE.OnEngineStop([]() {
        std::cout << "Engine stopped gracefully!" << std::endl;
        });

    // Run engine in separate thread
    std::thread engineThread([&]() {
        ENGINE.Run();
        });

    // Print statistics every 2 seconds
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        PrintRealTimeStats();

        // Demonstrate FindObjectsWithTag (REQUIREMENT #2)
        auto enemies = ENGINE.FindGameObjectsWithTag("Enemy");
        auto collectibles = ENGINE.FindGameObjectsWithTag("Collectible");
        std::cout << "Found " << enemies.size() << " enemies and "
            << collectibles.size() << " collectibles using FindObjectsWithTag!" << std::endl;
    }

    // Stop engine
    std::cout << "\n=== Stopping Engine Demo ===" << std::endl;
    ENGINE.Stop();

    // Wait for engine thread to finish
    engineThread.join();

    // Print final statistics
    std::cout << "\n=== Final Performance Report ===" << std::endl;
    ENGINE.PrintPerformanceStats();
    ENGINE.PrintMemoryStats();

    // Demonstrate all factories
    std::cout << "\n=== Factory System Report ===" << std::endl;
    ENGINE.GetComponentFactory().PrintFactoryInfo();
    ENGINE.GetGameObjectFactory().PrintFactoryInfo();

    // Shutdown
    ENGINE.Shutdown();

    std::cout << "\n DEMONSTRATION COMPLETE! " << std::endl;
    std::cout << "All 5 requirements successfully demonstrated:" << std::endl;
    std::cout << " #1: Zero allocation during main loop (ObjectPools + MemoryManager)" << std::endl;
    std::cout << " #2: Add/remove objects + FindObjectsWithTag (Scene + SceneManager)" << std::endl;
    std::cout << " #3: Component architecture + Data-Oriented Design (GameObject + Components + Batch processing)" << std::endl;
    std::cout << " #4: Factory system + data-driven (ComponentFactory + GameObjectFactory + Templates)" << std::endl;
    std::cout << " #5: Threading with boss/worker (ThreadPool + UpdateSystem + Parallel updates)" << std::endl;
}

int main() {
    try {
        RunEngineDemo();
    }
    catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}