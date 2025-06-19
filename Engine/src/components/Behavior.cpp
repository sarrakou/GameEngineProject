#include "../include/components/Behavior.h"
#include "../include/core/GameObject.h"
#include <iostream>
#include <chrono>

// Static time tracking
static auto engineStartTime = std::chrono::high_resolution_clock::now();
static float lastFrameTime = 0.0f;
static float currentDeltaTime = 0.0f;

void Behavior::Update(float deltaTime) {
    // Update static time tracking
    currentDeltaTime = deltaTime;

    // Cache transform on first use
    if (!cachedTransform) {
        CacheTransform();
    }

    // Call Start() only once when first enabled
    if (IsActive() && !started) {
        Start();
        started = true;
    }

    // Call update methods if active
    if (IsActive()) {
        OnUpdate(deltaTime);
    }
}

void Behavior::OnEnable() {
    Component::OnEnable();
    // Reset started flag if component was disabled and re-enabled
    if (!started) {
        CacheTransform();
    }
}

void Behavior::OnDisable() {
    Component::OnDisable();
}

void Behavior::OnDestroy() {
    Component::OnDestroy();
    cachedTransform = nullptr;
}

Transform* Behavior::GetTransform() {
    if (!cachedTransform) {
        CacheTransform();
    }
    return cachedTransform;
}

float Behavior::GetTime() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - engineStartTime);
    return duration.count() / 1000000.0f; // Convert to seconds
}

float Behavior::GetDeltaTime() {
    return currentDeltaTime;
}

void Behavior::Log(const std::string& message) const {
    std::cout << "[LOG] " << GetTypeName() << ": " << message << std::endl;
}

void Behavior::LogWarning(const std::string& message) const {
    std::cout << "[WARNING] " << GetTypeName() << ": " << message << std::endl;
}

void Behavior::LogError(const std::string& message) const {
    std::cerr << "[ERROR] " << GetTypeName() << ": " << message << std::endl;
}

void Behavior::CacheTransform() {
    if (GetOwner()) {
        cachedTransform = GetOwner()->GetComponent<Transform>();
    }
}

// Template specializations and implementations for finding objects
// Note: These would normally be implemented using Scene/SceneManager
// For now, we'll provide basic implementations

template<>
Transform* Behavior::FindObjectOfType<Transform>() const {
    // This would normally search through the scene
    // For now, return this object's transform if it exists
    return GetOwner() ? GetOwner()->GetComponent<Transform>() : nullptr;
}

GameObject* Behavior::FindGameObjectWithTag(const std::string& tag) const {
    // This would normally search through the scene
    // For now, just check if our own GameObject has this tag
    GameObject* owner = GetOwner();
    if (owner && owner->GetTag() == tag) {
        return owner;
    }
    return nullptr;
}

std::vector<GameObject*> Behavior::FindGameObjectsWithTag(const std::string& tag) const {
    std::vector<GameObject*> result;

    // This would normally search through the scene
    // For now, just check our own GameObject
    GameObject* owner = GetOwner();
    if (owner && owner->GetTag() == tag) {
        result.push_back(owner);
    }

    return result;
}

// Advanced Behavior system for batch processing (Data-Oriented Design support)
class BehaviorSystem {
private:
    std::vector<Behavior*> behaviors;
    std::vector<Behavior*> lateUpdateBehaviors;
    std::vector<Behavior*> fixedUpdateBehaviors;

public:
    void RegisterBehavior(Behavior* behavior) {
        if (behavior && std::find(behaviors.begin(), behaviors.end(), behavior) == behaviors.end()) {
            behaviors.push_back(behavior);
        }
    }

    void UnregisterBehavior(Behavior* behavior) {
        auto it = std::find(behaviors.begin(), behaviors.end(), behavior);
        if (it != behaviors.end()) {
            behaviors.erase(it);
        }

        // Also remove from other lists
        auto lateIt = std::find(lateUpdateBehaviors.begin(), lateUpdateBehaviors.end(), behavior);
        if (lateIt != lateUpdateBehaviors.end()) {
            lateUpdateBehaviors.erase(lateIt);
        }

        auto fixedIt = std::find(fixedUpdateBehaviors.begin(), fixedUpdateBehaviors.end(), behavior);
        if (fixedIt != fixedUpdateBehaviors.end()) {
            fixedUpdateBehaviors.erase(fixedIt);
        }
    }

    // Batch processing methods for Data-Oriented Design
    void UpdateAllBehaviors(float deltaTime) {
        // Process all behaviors in batch
        for (Behavior* behavior : behaviors) {
            if (behavior && behavior->IsActive()) {
                behavior->Update(deltaTime);
            }
        }
    }

    void LateUpdateAllBehaviors(float deltaTime) {
        for (Behavior* behavior : lateUpdateBehaviors) {
            if (behavior && behavior->IsActive()) {
                behavior->OnLateUpdate(deltaTime);
            }
        }
    }

    void FixedUpdateAllBehaviors(float fixedDeltaTime) {
        for (Behavior* behavior : fixedUpdateBehaviors) {
            if (behavior && behavior->IsActive()) {
                behavior->OnFixedUpdate(fixedDeltaTime);
            }
        }
    }

    // Get behavior counts for debugging
    size_t GetBehaviorCount() const { return behaviors.size(); }
    const std::vector<Behavior*>& GetAllBehaviors() const { return behaviors; }
};

// Global behavior system instance (would normally be owned by Engine)
static BehaviorSystem g_behaviorSystem;