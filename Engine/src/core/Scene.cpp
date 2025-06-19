#include "../include/core/Scene.h"
#include "../include/components/Transform.h"
#include "../include/components/Behavior.h"
#include <iostream>
#include <algorithm>
#include <fstream>

Scene::Scene(const std::string& sceneName) : name(sceneName) {
    // Reserve space for common scenarios to avoid reallocations
    objects.reserve(100);
    cachedTransforms.reserve(100);
    cachedBehaviors.reserve(100);
}

Scene::Scene(Scene&& other) noexcept
    : name(std::move(other.name))
    , objects(std::move(other.objects))
    , objectsByTag(std::move(other.objectsByTag))
    , objectsById(std::move(other.objectsById))
    , componentCachesDirty(other.componentCachesDirty)
    , cachedTransforms(std::move(other.cachedTransforms))
    , cachedBehaviors(std::move(other.cachedBehaviors))
    , active(other.active)
    , nextObjectIndex(other.nextObjectIndex)
    , gameObjectCreatedCallbacks(std::move(other.gameObjectCreatedCallbacks))
    , gameObjectDestroyedCallbacks(std::move(other.gameObjectDestroyedCallbacks)) {
}

Scene& Scene::operator=(Scene&& other) noexcept {
    if (this != &other) {
        name = std::move(other.name);
        objects = std::move(other.objects);
        objectsByTag = std::move(other.objectsByTag);
        objectsById = std::move(other.objectsById);
        componentCachesDirty = other.componentCachesDirty;
        cachedTransforms = std::move(other.cachedTransforms);
        cachedBehaviors = std::move(other.cachedBehaviors);
        active = other.active;
        nextObjectIndex = other.nextObjectIndex;
        gameObjectCreatedCallbacks = std::move(other.gameObjectCreatedCallbacks);
        gameObjectDestroyedCallbacks = std::move(other.gameObjectDestroyedCallbacks);
    }
    return *this;
}

// GameObject creation
GameObject* Scene::CreateGameObject(const std::string& tag) {
    auto gameObject = std::make_unique<GameObject>(tag);
    GameObject* ptr = gameObject.get();

    AddGameObject(std::move(gameObject));
    return ptr;
}

GameObject* Scene::CreateGameObject(const std::string& name, const std::string& tag) {
    GameObject* obj = CreateGameObject(tag);
    // Note: Would need to add name field to GameObject for this to work fully
    return obj;
}

void Scene::AddGameObject(std::unique_ptr<GameObject> gameObject) {
    if (!gameObject) return;

    GameObject* ptr = gameObject.get();
    objects.push_back(std::move(gameObject));

    UpdateLookupMaps(ptr);
    MarkComponentCachesDirty();
    TriggerGameObjectCreated(ptr);
}

// GameObject destruction
bool Scene::DestroyGameObject(GameObject* gameObject) {
    if (!gameObject) return false;

    auto it = std::find_if(objects.begin(), objects.end(),
        [gameObject](const std::unique_ptr<GameObject>& obj) {
            return obj.get() == gameObject;
        });

    if (it != objects.end()) {
        TriggerGameObjectDestroyed(gameObject);
        RemoveFromLookupMaps(gameObject);
        objects.erase(it);
        MarkComponentCachesDirty();
        return true;
    }

    return false;
}

bool Scene::DestroyGameObject(size_t id) {
    auto it = objectsById.find(id);
    if (it != objectsById.end()) {
        return DestroyGameObject(it->second);
    }
    return false;
}

void Scene::DestroyGameObjectsWithTag(const std::string& tag) {
    auto objectsWithTag = FindGameObjectsWithTag(tag);
    for (GameObject* obj : objectsWithTag) {
        DestroyGameObject(obj);
    }
}

void Scene::DestroyAllGameObjects() {
    // Trigger destroyed events for all objects
    for (const auto& obj : objects) {
        TriggerGameObjectDestroyed(obj.get());
    }

    objects.clear();
    objectsByTag.clear();
    objectsById.clear();
    MarkComponentCachesDirty();
}

// GameObject finding (MAIN REQUIREMENT!)
GameObject* Scene::FindGameObjectWithTag(const std::string& tag) {
    auto it = objectsByTag.find(tag);
    if (it != objectsByTag.end() && !it->second.empty()) {
        return it->second[0]; // Return first object with this tag
    }
    return nullptr;
}

std::vector<GameObject*> Scene::FindGameObjectsWithTag(const std::string& tag) {
    auto it = objectsByTag.find(tag);
    if (it != objectsByTag.end()) {
        return it->second;
    }
    return std::vector<GameObject*>(); // Return empty vector
}

GameObject* Scene::FindGameObjectById(size_t id) {
    auto it = objectsById.find(id);
    if (it != objectsById.end()) {
        return it->second;
    }
    return nullptr;
}

// Batch access for Data-Oriented Design
const std::vector<Transform*>& Scene::GetAllTransforms() const {
    if (componentCachesDirty) {
        RefreshComponentCaches();
    }
    return cachedTransforms;
}

const std::vector<Behavior*>& Scene::GetAllBehaviors() const {
    if (componentCachesDirty) {
        RefreshComponentCaches();
    }
    return cachedBehaviors;
}

void Scene::RefreshComponentCaches() const {
    cachedTransforms.clear();
    cachedBehaviors.clear();

    for (const auto& gameObject : objects) {
        if (gameObject && gameObject->IsActive()) {
            // Cache Transform components
            if (Transform* transform = gameObject->GetComponent<Transform>()) {
                cachedTransforms.push_back(transform);
            }

            // Cache Behavior components (check all components for Behavior base class)
            for (const auto& component : gameObject->GetAllComponents()) {
                if (Behavior* behavior = dynamic_cast<Behavior*>(component.get())) {
                    cachedBehaviors.push_back(behavior);
                }
            }
        }
    }

    componentCachesDirty = false;
}

std::vector<GameObject*> Scene::GetActiveGameObjects() const {
    std::vector<GameObject*> activeObjects;
    for (const auto& obj : objects) {
        if (obj && obj->IsActive()) {
            activeObjects.push_back(obj.get());
        }
    }
    return activeObjects;
}

// Scene statistics
size_t Scene::GetActiveGameObjectCount() const {
    return std::count_if(objects.begin(), objects.end(),
        [](const std::unique_ptr<GameObject>& obj) {
            return obj && obj->IsActive();
        });
}

size_t Scene::GetGameObjectCountWithTag(const std::string& tag) const {
    auto it = objectsByTag.find(tag);
    return (it != objectsByTag.end()) ? it->second.size() : 0;
}

// Scene update
void Scene::Update(float deltaTime) {
    if (!active) return;

    // Update all active GameObjects
    for (const auto& gameObject : objects) {
        if (gameObject && gameObject->IsActive()) {
            gameObject->Update(deltaTime);
        }
    }
}

void Scene::LateUpdate(float deltaTime) {
    if (!active) return;

    // Process late updates for behaviors
    const auto& behaviors = GetAllBehaviors();
    for (Behavior* behavior : behaviors) {
        if (behavior && behavior->IsActive()) {
            behavior->OnLateUpdate(deltaTime);
        }
    }
}

void Scene::FixedUpdate(float fixedDeltaTime) {
    if (!active) return;

    // Process fixed updates for behaviors
    const auto& behaviors = GetAllBehaviors();
    for (Behavior* behavior : behaviors) {
        if (behavior && behavior->IsActive()) {
            behavior->OnFixedUpdate(fixedDeltaTime);
        }
    }
}

// Basic serialization framework
void Scene::SaveToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to save scene to: " << filepath << std::endl;
        return;
    }

    file << "Scene: " << name << std::endl;
    file << "GameObjects: " << objects.size() << std::endl;

    for (const auto& obj : objects) {
        file << "GameObject ID: " << obj->GetId()
            << " Tag: " << obj->GetTag()
            << " Active: " << obj->IsActive()
            << " Components: " << obj->GetComponentCount() << std::endl;
    }

    file.close();
    std::cout << "Scene saved to: " << filepath << std::endl;
}

bool Scene::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to load scene from: " << filepath << std::endl;
        return false;
    }

    // Basic loading (would need more sophisticated parsing for real implementation)
    std::string line;
    while (std::getline(file, line)) {
        // Parse scene data...
        // This is a placeholder for a more complete implementation
    }

    file.close();
    std::cout << "Scene loaded from: " << filepath << std::endl;
    return true;
}

// Utility functions
void Scene::PrintSceneInfo() const {
    std::cout << "\n=== Scene Info: " << name << " ===\n";
    std::cout << "Active: " << (active ? "Yes" : "No") << std::endl;
    std::cout << "Total GameObjects: " << objects.size() << std::endl;
    std::cout << "Active GameObjects: " << GetActiveGameObjectCount() << std::endl;
    std::cout << "Cached Transforms: " << GetAllTransforms().size() << std::endl;
    std::cout << "Cached Behaviors: " << GetAllBehaviors().size() << std::endl;

    // Tag distribution
    std::cout << "\nTag Distribution:\n";
    for (const auto& tagPair : objectsByTag) {
        std::cout << "  '" << tagPair.first << "': " << tagPair.second.size() << " objects\n";
    }
    std::cout << std::endl;
}

// Private helper methods
void Scene::UpdateLookupMaps(GameObject* gameObject) {
    if (!gameObject) return;

    // Add to ID map
    objectsById[gameObject->GetId()] = gameObject;

    // Add to tag map
    const std::string& tag = gameObject->GetTag();
    objectsByTag[tag].push_back(gameObject);
}

void Scene::RemoveFromLookupMaps(GameObject* gameObject) {
    if (!gameObject) return;

    // Remove from ID map
    objectsById.erase(gameObject->GetId());

    // Remove from tag map
    const std::string& tag = gameObject->GetTag();
    auto& tagVector = objectsByTag[tag];
    auto it = std::find(tagVector.begin(), tagVector.end(), gameObject);
    if (it != tagVector.end()) {
        tagVector.erase(it);

        // Remove empty tag entries
        if (tagVector.empty()) {
            objectsByTag.erase(tag);
        }
    }
}

// Event system
void Scene::OnGameObjectCreated(const GameObjectEvent& callback) {
    gameObjectCreatedCallbacks.push_back(callback);
}

void Scene::OnGameObjectDestroyed(const GameObjectEvent& callback) {
    gameObjectDestroyedCallbacks.push_back(callback);
}

void Scene::TriggerGameObjectCreated(GameObject* gameObject) {
    for (const auto& callback : gameObjectCreatedCallbacks) {
        callback(gameObject);
    }
}

void Scene::TriggerGameObjectDestroyed(GameObject* gameObject) {
    for (const auto& callback : gameObjectDestroyedCallbacks) {
        callback(gameObject);
    }
}