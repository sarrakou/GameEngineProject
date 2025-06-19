#include "../include/core/SceneManager.h"
#include <iostream>
#include <algorithm>

// Static instance initialization
SceneManager* SceneManager::instance = nullptr;

// Singleton access
SceneManager& SceneManager::GetInstance() {
    if (instance == nullptr) {
        instance = new SceneManager();
    }
    return *instance;
}

void SceneManager::DestroyInstance() {
    delete instance;
    instance = nullptr;
}

SceneManager::~SceneManager() {
    RemoveAllScenes();
}

// Scene creation and management
Scene* SceneManager::CreateScene(const std::string& sceneName) {
    if (!IsValidSceneName(sceneName)) {
        std::cerr << "Invalid scene name: " << sceneName << std::endl;
        return nullptr;
    }

    if (HasScene(sceneName)) {
        std::cerr << "Scene already exists: " << sceneName << std::endl;
        return GetScene(sceneName);
    }

    auto scene = std::make_unique<Scene>(sceneName);
    Scene* scenePtr = scene.get();

    scenes[sceneName] = std::move(scene);

    std::cout << "Scene created: " << sceneName << std::endl;
    return scenePtr;
}

bool SceneManager::AddScene(const std::string& sceneName, std::unique_ptr<Scene> scene) {
    if (!scene || !IsValidSceneName(sceneName)) {
        return false;
    }

    if (HasScene(sceneName)) {
        std::cerr << "Scene already exists: " << sceneName << std::endl;
        return false;
    }

    scene->SetName(sceneName);
    scenes[sceneName] = std::move(scene);
    return true;
}

bool SceneManager::RemoveScene(const std::string& sceneName) {
    auto it = scenes.find(sceneName);
    if (it == scenes.end()) {
        return false;
    }

    // If this is the current scene, unload it first
    if (currentScene == it->second.get()) {
        UnloadCurrentScene();
    }

    scenes.erase(it);
    std::cout << "Scene removed: " << sceneName << std::endl;
    return true;
}

void SceneManager::RemoveAllScenes() {
    UnloadCurrentScene();
    scenes.clear();
    std::cout << "All scenes removed" << std::endl;
}

// Scene access
Scene* SceneManager::GetScene(const std::string& sceneName) {
    auto it = scenes.find(sceneName);
    if (it != scenes.end()) {
        return it->second.get();
    }
    return nullptr;
}

// Scene switching
bool SceneManager::LoadScene(const std::string& sceneName) {
    auto it = scenes.find(sceneName);
    if (it == scenes.end()) {
        std::cerr << "Scene not found: " << sceneName << std::endl;
        return false;
    }

    std::string oldSceneName = currentSceneName;
    SwitchToScene(sceneName);
    TriggerSceneChanged(oldSceneName, sceneName);

    return true;
}

bool SceneManager::LoadSceneAsync(const std::string& sceneName, const std::function<void()>& callback) {
    if (!HasScene(sceneName)) {
        std::cerr << "Scene not found for async load: " << sceneName << std::endl;
        return false;
    }

    if (isTransitioning) {
        std::cerr << "Already transitioning to scene: " << nextSceneName << std::endl;
        return false;
    }

    isTransitioning = true;
    nextSceneName = sceneName;
    transitionCallback = callback;

    // In a real implementation, this would happen over multiple frames
    // For now, we'll complete it immediately in the next update
    return true;
}

void SceneManager::UnloadCurrentScene() {
    if (currentScene) {
        currentScene->SetActive(false);
        std::cout << "Scene unloaded: " << currentSceneName << std::endl;
    }

    currentScene = nullptr;
    currentSceneName.clear();
}

// Scene existence checks
bool SceneManager::HasScene(const std::string& sceneName) const {
    return scenes.find(sceneName) != scenes.end();
}

std::vector<std::string> SceneManager::GetAllSceneNames() const {
    std::vector<std::string> names;
    names.reserve(scenes.size());

    for (const auto& pair : scenes) {
        names.push_back(pair.first);
    }

    return names;
}

// Scene file operations
bool SceneManager::SaveScene(const std::string& sceneName, const std::string& filepath) {
    Scene* scene = GetScene(sceneName);
    if (!scene) {
        std::cerr << "Cannot save scene: " << sceneName << " (not found)" << std::endl;
        return false;
    }

    scene->SaveToFile(filepath);
    return true;
}

bool SceneManager::LoadSceneFromFile(const std::string& sceneName, const std::string& filepath) {
    // Create scene if it doesn't exist
    Scene* scene = GetScene(sceneName);
    if (!scene) {
        scene = CreateScene(sceneName);
    }

    if (scene) {
        return scene->LoadFromFile(filepath);
    }

    return false;
}

bool SceneManager::SaveCurrentScene(const std::string& filepath) {
    if (currentScene) {
        currentScene->SaveToFile(filepath);
        return true;
    }
    return false;
}

// Scene updates
void SceneManager::Update(float deltaTime) {
    // Handle async scene transitions
    if (isTransitioning) {
        CompleteTransition();
    }

    // Update current scene
    if (currentScene && currentScene->IsActive()) {
        currentScene->Update(deltaTime);
    }
}

void SceneManager::LateUpdate(float deltaTime) {
    if (currentScene && currentScene->IsActive()) {
        currentScene->LateUpdate(deltaTime);
    }
}

void SceneManager::FixedUpdate(float fixedDeltaTime) {
    if (currentScene && currentScene->IsActive()) {
        currentScene->FixedUpdate(fixedDeltaTime);
    }
}

// Global GameObject operations
GameObject* SceneManager::CreateGameObject(const std::string& tag) {
    if (currentScene) {
        return currentScene->CreateGameObject(tag);
    }

    std::cerr << "Cannot create GameObject: No active scene" << std::endl;
    return nullptr;
}

GameObject* SceneManager::FindGameObjectWithTag(const std::string& tag) {
    if (currentScene) {
        return currentScene->FindGameObjectWithTag(tag);
    }
    return nullptr;
}

std::vector<GameObject*> SceneManager::FindGameObjectsWithTag(const std::string& tag) {
    if (currentScene) {
        return currentScene->FindGameObjectsWithTag(tag);
    }
    return std::vector<GameObject*>();
}

bool SceneManager::DestroyGameObject(GameObject* gameObject) {
    if (currentScene) {
        return currentScene->DestroyGameObject(gameObject);
    }
    return false;
}

// Data-Oriented batch access
std::vector<Transform*> SceneManager::GetAllTransforms() {
    if (currentScene) {
        return currentScene->GetAllTransforms();
    }
    return std::vector<Transform*>();
}

std::vector<Behavior*> SceneManager::GetAllBehaviors() {
    if (currentScene) {
        return currentScene->GetAllBehaviors();
    }
    return std::vector<Behavior*>();
}

// Scene transition management
void SceneManager::CompleteTransition() {
    if (!isTransitioning) return;

    std::string oldSceneName = currentSceneName;

    // Switch to the new scene
    SwitchToScene(nextSceneName);

    // Call the callback if provided
    if (transitionCallback) {
        transitionCallback();
        transitionCallback = nullptr;
    }

    // Trigger scene change event
    TriggerSceneChanged(oldSceneName, nextSceneName);

    // Clear transition state
    isTransitioning = false;
    nextSceneName.clear();

    std::cout << "Scene transition completed: " << oldSceneName << " -> " << currentSceneName << std::endl;
}

// Events
void SceneManager::OnSceneChanged(const SceneChangeEvent& callback) {
    sceneChangeCallbacks.push_back(callback);
}

// Utility and debugging
void SceneManager::PrintSceneInfo() const {
    std::cout << "\n=== SceneManager Info ===" << std::endl;
    std::cout << "Total Scenes: " << scenes.size() << std::endl;
    std::cout << "Current Scene: " << (currentScene ? currentSceneName : "None") << std::endl;
    std::cout << "Transitioning: " << (isTransitioning ? "Yes -> " + nextSceneName : "No") << std::endl;

    if (currentScene) {
        std::cout << "\nCurrent Scene Details:" << std::endl;
        currentScene->PrintSceneInfo();
    }
}

void SceneManager::PrintAllScenesInfo() const {
    std::cout << "\n=== All Scenes Info ===" << std::endl;

    for (const auto& pair : scenes) {
        std::cout << "\n--- Scene: " << pair.first << " ---" << std::endl;
        pair.second->PrintSceneInfo();
    }
}

// Private methods
void SceneManager::SwitchToScene(const std::string& sceneName) {
    auto it = scenes.find(sceneName);
    if (it == scenes.end()) {
        std::cerr << "Cannot switch to scene: " << sceneName << " (not found)" << std::endl;
        return;
    }

    // Deactivate current scene
    if (currentScene) {
        currentScene->SetActive(false);
    }

    // Switch to new scene
    currentScene = it->second.get();
    currentSceneName = sceneName;
    currentScene->SetActive(true);

    std::cout << "Switched to scene: " << sceneName << std::endl;
}

void SceneManager::TriggerSceneChanged(const std::string& oldScene, const std::string& newScene) {
    for (const auto& callback : sceneChangeCallbacks) {
        callback(oldScene, newScene);
    }
}

bool SceneManager::IsValidSceneName(const std::string& sceneName) const {
    return !sceneName.empty() && sceneName.find_first_not_of(" \t\n\r") != std::string::npos;
}