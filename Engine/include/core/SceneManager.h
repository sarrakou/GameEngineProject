#pragma once

#include "Scene.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>

class SceneManager {
private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
    Scene* currentScene = nullptr;
    std::string currentSceneName;

    // Scene transition system
    bool isTransitioning = false;
    std::string nextSceneName;
    std::function<void()> transitionCallback;

    // Singleton pattern (typical for managers)
    static SceneManager* instance;

public:
    // Singleton access
    static SceneManager& GetInstance();
    static void DestroyInstance();

    // Constructor and destructor
    SceneManager() = default;
    ~SceneManager();

    // Delete copy operations (singleton)
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    // Scene creation and management
    Scene* CreateScene(const std::string& sceneName);
    bool AddScene(const std::string& sceneName, std::unique_ptr<Scene> scene);
    bool RemoveScene(const std::string& sceneName);
    void RemoveAllScenes();

    // Scene access
    Scene* GetScene(const std::string& sceneName);
    Scene* GetCurrentScene() { return currentScene; }
    const std::string& GetCurrentSceneName() const { return currentSceneName; }

    // Scene switching
    bool LoadScene(const std::string& sceneName);
    bool LoadSceneAsync(const std::string& sceneName, const std::function<void()>& callback = nullptr);
    void UnloadCurrentScene();

    // Scene existence checks
    bool HasScene(const std::string& sceneName) const;
    std::vector<std::string> GetAllSceneNames() const;
    size_t GetSceneCount() const { return scenes.size(); }

    // Scene file operations
    bool SaveScene(const std::string& sceneName, const std::string& filepath);
    bool LoadSceneFromFile(const std::string& sceneName, const std::string& filepath);
    bool SaveCurrentScene(const std::string& filepath);

    // Scene updates (called by Engine)
    void Update(float deltaTime);
    void LateUpdate(float deltaTime);
    void FixedUpdate(float fixedDeltaTime);

    // Global GameObject operations (operate on current scene)
    GameObject* CreateGameObject(const std::string& tag = "");
    GameObject* FindGameObjectWithTag(const std::string& tag);
    std::vector<GameObject*> FindGameObjectsWithTag(const std::string& tag);
    bool DestroyGameObject(GameObject* gameObject);

    // Global component operations (operate on current scene)
    template<typename T>
    std::vector<T*> FindComponentsOfType();

    template<typename T>
    T* FindComponentOfType();

    // Data-Oriented batch access (from current scene)
    std::vector<Transform*> GetAllTransforms();
    std::vector<Behavior*> GetAllBehaviors();

    // Scene transition management
    bool IsTransitioning() const { return isTransitioning; }
    void CompleteTransition();

    // Events for scene changes
    using SceneChangeEvent = std::function<void(const std::string& oldScene, const std::string& newScene)>;
    void OnSceneChanged(const SceneChangeEvent& callback);

    // Utility and debugging
    void PrintSceneInfo() const;
    void PrintAllScenesInfo() const;

private:
    // Internal scene switching
    void SwitchToScene(const std::string& sceneName);
    void TriggerSceneChanged(const std::string& oldScene, const std::string& newScene);

    // Event callbacks
    std::vector<SceneChangeEvent> sceneChangeCallbacks;

    // Helper methods
    bool IsValidSceneName(const std::string& sceneName) const;
};

// Template implementations
template<typename T>
std::vector<T*> SceneManager::FindComponentsOfType() {
    if (currentScene) {
        return currentScene->FindComponentsOfType<T>();
    }
    return std::vector<T*>();
}

template<typename T>
T* SceneManager::FindComponentOfType() {
    if (currentScene) {
        return currentScene->FindComponentOfType<T>();
    }
    return nullptr;
}

// Convenience macros for global access
#define SCENE_MANAGER SceneManager::GetInstance()
#define CURRENT_SCENE SceneManager::GetInstance().GetCurrentScene()
#define FIND_OBJECTS_WITH_TAG(tag) SceneManager::GetInstance().FindGameObjectsWithTag(tag)
#define CREATE_GAME_OBJECT(tag) SceneManager::GetInstance().CreateGameObject(tag)