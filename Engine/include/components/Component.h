#pragma once

// Forward declaration to avoid circular dependency
class GameObject;

class Component {
private:
    GameObject* owner = nullptr;
    bool active = true;

public:
    // Constructor and destructor
    Component() = default;
    virtual ~Component() = default;

    // Delete copy operations (components are unique to their GameObject)
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    // Move operations
    Component(Component&& other) noexcept : owner(other.owner), active(other.active) {
        other.owner = nullptr;
    }

    Component& operator=(Component&& other) noexcept {
        if (this != &other) {
            owner = other.owner;
            active = other.active;
            other.owner = nullptr;
        }
        return *this;
    }

    // Owner management (called by GameObject)
    void SetOwner(GameObject* gameObject) { owner = gameObject; }
    GameObject* GetOwner() const { return owner; }

    // Active state
    bool IsActive() const { return active; }
    void SetActive(bool isActive) { active = isActive; }

    // Virtual update method - override in derived components
    virtual void Update(float deltaTime) {}

    // Virtual methods for component lifecycle
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnDestroy() {}

    // Get component type name (useful for debugging/serialization)
    virtual const char* GetTypeName() const { return "Component"; }
};