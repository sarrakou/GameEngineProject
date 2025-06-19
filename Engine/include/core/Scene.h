#pragma once

#include "GameObject.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>
#include <components/Behavior.h>

class Scene {
private:
    std::string name;
    std::vector<std::unique_ptr<GameObject>> objects;

    // Fast lookup maps for performance (Data-Oriented Design)
    std::unordered_map<std::string, std::vector<GameObject*>> objectsByTag;
    std::unordered_map<size_t, GameObject*> objectsById;

    // Component caches for batch processing
    mutable bool componentCachesDirty = true;
    mutable std::vector<Transform*> cachedTransforms;
    mutable std::vector<Behavior*> cachedBehaviors;

    // Scene state
    bool active = true;
    size_t nextObjectIndex = 0;

public:
    // Constructor and destructor
    Scene(const std::string& sceneName = "Scene");
    ~Scene() = default;

    // Delete copy operations (scenes are unique)
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // Move operations
    Scene(Scene&& other) noexcept;
    Scene& operator=(Scene&& other) noexcept;

    // Scene management
    const std::string& GetName() const { return name; }
    void SetName(const std::string& sceneName) { name = sceneName; }

    bool IsActive() const { return active; }
    void SetActive(bool isActive) { active = isActive; }

    // GameObject creation and management
    GameObject* CreateGameObject(const std::string& tag = "");
    GameObject* CreateGameObject(const std::string& name, const std::string& tag);

    // GameObject addition (for objects created elsewhere)
    void AddGameObject(std::unique_ptr<GameObject> gameObject);

    // GameObject removal and destruction
    bool DestroyGameObject(GameObject* gameObject);
    bool DestroyGameObject(size_t id);
    void DestroyGameObjectsWithTag(const std::string& tag);
    void DestroyAllGameObjects();

    // GameObject finding (REQUIREMENT: FindObjectsWithTag functionality)
    GameObject* FindGameObjectWithTag(const std::string& tag);
    std::vector<GameObject*> FindGameObjectsWithTag(const std::string& tag);
    GameObject* FindGameObjectById(size_t id);
    GameObject* FindGameObjectByName(const std::string& name); // If we add names later

    // Component finding (for Data-Oriented processing)
    template<typename T>
    std::vector<T*> FindComponentsOfType();

    template<typename T>
    T* FindComponentOfType();

    // Batch access for Data-Oriented Design
    const std::vector<Transform*>& GetAllTransforms() const;
    const std::vector<Behavior*>& GetAllBehaviors() const;
    void RefreshComponentCaches() const;

    // GameObject iteration
    const std::vector<std::unique_ptr<GameObject>>& GetAllGameObjects() const;
    std::vector<GameObject*> GetActiveGameObjects() const;

    // Scene statistics
    size_t GetGameObjectCount() const { return objects.size(); }
    size_t GetActiveGameObjectCount() const;
    size_t GetGameObjectCountWithTag(const std::string& tag) const;

    // Scene update (called by Engine)
    void Update(float deltaTime);
    void LateUpdate(float deltaTime);
    void FixedUpdate(float fixedDeltaTime);

    // Scene serialization (basic framework)
    void SaveToFile(const std::string& filepath) const;
    bool LoadFromFile(const std::string& filepath);

    // Utility functions
    void PrintSceneInfo() const;
    void PrintGameObjectHierarchy() const;

    // Event system (optional requirement #6)
    using GameObjectEvent = std::function<void(GameObject*)>;
    void OnGameObjectCreated(const GameObjectEvent& callback);
    void OnGameObjectDestroyed(const GameObjectEvent& callback);

private:
    // Internal management
    void UpdateLookupMaps(GameObject* gameObject);
    void RemoveFromLookupMaps(GameObject* gameObject);
    void MarkComponentCachesDirty() { componentCachesDirty = true; }

    // Event callbacks
    std::vector<GameObjectEvent> gameObjectCreatedCallbacks;
    std::vector<GameObjectEvent> gameObjectDestroyedCallbacks;

    void TriggerGameObjectCreated(GameObject* gameObject);
    void TriggerGameObjectDestroyed(GameObject* gameObject);
};

// Template implementations
template<typename T>
std::vector<T*> Scene::FindComponentsOfType() {
    std::vector<T*> result;

    for (const auto& gameObject : objects) {
        if (gameObject && gameObject->IsActive()) {
            if (T* component = gameObject->GetComponent<T>()) {
                result.push_back(component);
            }
        }
    }

    return result;
}

template<typename T>
T* Scene::FindComponentOfType() {
    for (const auto& gameObject : objects) {
        if (gameObject && gameObject->IsActive()) {
            if (T* component = gameObject->GetComponent<T>()) {
                return component;
            }
        }
    }
    return nullptr;
}