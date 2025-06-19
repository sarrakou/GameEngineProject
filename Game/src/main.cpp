#include "core/Engine.h"
#include "components/Transform.h"
#include "components/Behavior.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <cmath>
#include <typeinfo>

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
        // RTTI: Show what type this behavior actually is
        std::cout << "[RTTI] Behavior type: " << typeid(*this).name() << std::endl;
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

    std::string GetDisplayName() const override { return "Demo Player Behavior"; }
};

class DemoEnemyBehavior : public Behavior {
private:
    float orbitRadius = 8.0f;
    float orbitSpeed = 45.0f;

public:
    void Start() override {
        Log("DemoEnemyBehavior started!");
        std::cout << "[RTTI] Behavior type: " << typeid(*this).name() << std::endl;
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

    std::string GetDisplayName() const override { return "Demo Enemy Behavior"; }
};

// Demonstration functions
static void RegisterCustomComponents() {
    std::cout << "\n=== Registering Custom Components (RTTI Demo) ===" << std::endl;

    std::cout << "[RTTI] Registering DemoPlayerBehavior: " << typeid(DemoPlayerBehavior).name() << std::endl;
    std::cout << "[RTTI] Registering DemoEnemyBehavior: " << typeid(DemoEnemyBehavior).name() << std::endl;

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

    if (player) {
        std::cout << "[RTTI] Player GameObject type: " << typeid(*player).name() << std::endl;
    }

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

                    std::cout << "[RTTI] Transform component type: " << typeid(*transform).name() << std::endl;
                }
            }
        }
    }

    std::cout << "Scene populated with objects!" << std::endl;
}

static void DemonstrateRTTIComponentSearch() {
    std::cout << "\n=== RTTI Component Search Demo ===" << std::endl;

    Scene* currentScene = ENGINE.GetCurrentScene();
    if (!currentScene) return;

    // RTTI: Find all components of specific types using template methods
    auto transforms = ENGINE.GetAllComponentsOfType<Transform>();
    auto behaviors = ENGINE.GetAllComponentsOfType<Behavior>();

    std::cout << "[RTTI] Found " << transforms.size() << " Transform components" << std::endl;
    std::cout << "[RTTI] Found " << behaviors.size() << " Behavior components" << std::endl;

    std::cout << "\n[RTTI] Behavior component types:" << std::endl;
    for (size_t i = 0; i < behaviors.size() && i < 3; ++i) {
        if (behaviors[i]) {
            std::cout << "  " << i << ": " << typeid(*behaviors[i]).name() << std::endl;

            // RTTI: Try dynamic casting to specific behavior types
            if (dynamic_cast<DemoPlayerBehavior*>(behaviors[i])) {
                std::cout << "    -> This is a DemoPlayerBehavior!" << std::endl;
            }
            else if (dynamic_cast<DemoEnemyBehavior*>(behaviors[i])) {
                std::cout << "    -> This is a DemoEnemyBehavior!" << std::endl;
            }
        }
    }
}

static void DemonstrateDataOrientedProcessing() {
    std::cout << "\n=== Demonstrating Data-Oriented Processing ===" << std::endl;

    Scene* currentScene = ENGINE.GetCurrentScene();
    if (!currentScene) return;

    // Get all transforms for batch processing (REQUIREMENT #3 & #5)
    auto transforms = currentScene->GetAllTransforms();
    std::cout << "Found " << transforms.size() << " transforms for batch processing" << std::endl;

    std::cout << "[RTTI] Transform types in batch:" << std::endl;
    for (size_t i = 0; i < transforms.size() && i < 3; ++i) {
        if (transforms[i]) {
            std::cout << "  " << typeid(*transforms[i]).name() << std::endl;
        }
    }

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

static void DemonstrateRTTITypeComparison() {
    std::cout << "\n=== RTTI Type Comparison Demo ===" << std::endl;

    // RTTI: Compare engine instance types
    Engine& engine1 = ENGINE;
    Engine& engine2 = Engine::GetInstance();

    std::cout << "[RTTI] Engine type comparison:" << std::endl;
    std::cout << "  Engine1 type: " << typeid(engine1).name() << std::endl;
    std::cout << "  Engine2 type: " << typeid(engine2).name() << std::endl;
    std::cout << "  Same type? " << (typeid(engine1) == typeid(engine2) ? "YES" : "NO") << std::endl;
    std::cout << "  Same instance? " << (&engine1 == &engine2 ? "YES" : "NO") << std::endl;

    // RTTI: Create different component types and compare
    auto transform = std::make_unique<Transform>();
    auto playerBehavior = std::make_unique<DemoPlayerBehavior>();

    std::cout << "\n[RTTI] Component type comparison:" << std::endl;
    std::cout << "  Transform type: " << typeid(*transform).name() << std::endl;
    std::cout << "  PlayerBehavior type: " << typeid(*playerBehavior).name() << std::endl;
    std::cout << "  Same type? " << (typeid(*transform) == typeid(*playerBehavior) ? "YES" : "NO") << std::endl;

    // RTTI: Test inheritance hierarchy
    Component* basePtr1 = transform.get();
    Component* basePtr2 = playerBehavior.get();

    std::cout << "\n[RTTI] Inheritance testing:" << std::endl;
    std::cout << "  Transform as Component: " << typeid(*basePtr1).name() << std::endl;
    std::cout << "  PlayerBehavior as Component: " << typeid(*basePtr2).name() << std::endl;

    // RTTI: dynamic_cast testing
    if (dynamic_cast<Transform*>(basePtr1)) {
        std::cout << "  dynamic_cast to Transform: SUCCESS" << std::endl;
    }
    if (dynamic_cast<Behavior*>(basePtr2)) {
        std::cout << "  dynamic_cast to Behavior: SUCCESS" << std::endl;
    }
}

static void RunEngineDemo() {
    std::cout << "\n=== GAME ENGINE DEMO ===" << std::endl;

    Engine& engine = ENGINE;
    std::cout << "\n[RTTI] Engine type: " << typeid(engine).name() << std::endl;
    std::cout << "[RTTI] Engine hash: " << typeid(engine).hash_code() << std::endl;

    // Configure engine 
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

    // RTTI: Demonstrate type comparison
    DemonstrateRTTITypeComparison();

    // RTTI: Demonstrate component searching with RTTI
    DemonstrateRTTIComponentSearch();

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

        // RTTI: Show first enemy's type if found
        if (!enemies.empty() && enemies[0]) {
            std::cout << "[RTTI] First enemy type: " << typeid(*enemies[0]).name() << std::endl;
        }
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

    std::cout << "\n DEMO COMPLETE! " << std::endl;
}

int main() {
    try {
        // RTTI: Show what we're running
        std::cout << "[RTTI] Starting main() - type: " << typeid(main).name() << std::endl;

        RunEngineDemo();
    }
    catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;

        // RTTI: Show exception type
        std::cerr << "[RTTI] Exception type: " << typeid(e).name() << std::endl;
        return 1;
    }

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}