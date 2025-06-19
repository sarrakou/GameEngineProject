#pragma once
#include <typeinfo>
#include <string>

// Forward declaration to avoid circular dependency
class GameObject;

class Component {
private:
    GameObject* owner = nullptr;
    bool active = true;

public:
    // Constructor  destructor
    Component() = default;
    virtual ~Component() = default; //virtual

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

    // ===== RTTI ENHANCEMENT METHODS =====

    // Get component type name using RTTI 
    std::string GetTypeName() const {
        return typeid(*this).name();
    }

    // Get a more readable type name
    virtual std::string GetDisplayName() const {
        return GetTypeName();
    }

    // RTTI helper methods for type checking
    template<typename T>
    bool IsOfType() const {
        return dynamic_cast<const T*>(this) != nullptr;
    }

    template<typename T>
    T* As() {
        return dynamic_cast<T*>(this);
    }

    template<typename T>
    const T* As() const {
        return dynamic_cast<const T*>(this);
    }

    // Check if this component is exactly a specific type (not derived)
    template<typename T>
    bool IsExactType() const {
        return typeid(*this) == typeid(T);
    }

    // Get type hash for fast comparisons
    size_t GetTypeHash() const {
        return typeid(*this).hash_code();
    }

    // Utility for debugging - print component info
    virtual void PrintDebugInfo() const {
#ifdef ENGINE_DEBUG_MODE
        std::cout << "Component Type: " << GetDisplayName()
            << ", Active: " << (active ? "true" : "false")
            << ", Owner: " << (owner ? "yes" : "no") << std::endl;
#endif
    }

    // ===== COMPARISON OPERATORS FOR TYPE CHECKING =====

    // Compare component types
    bool IsSameTypeAs(const Component& other) const {
        return typeid(*this) == typeid(other);
    }

    // Check if this component can be cast to the same type as another
    bool IsCompatibleWith(const Component& other) const {
        return GetTypeHash() == other.GetTypeHash();
    }
};

// ===== RTTI UTILITY FUNCTIONS =====

namespace ComponentUtils {
    // Helper function to get clean type name
    template<typename T>
    std::string GetCleanTypeName() {
        std::string name = typeid(T).name();
        return name;
    }

    inline bool AreSameType(const Component* a, const Component* b) {
        if (!a || !b) return false;
        return typeid(*a) == typeid(*b);
    }

    // Safe dynamic cast with null checking
    template<typename T>
    T* SafeCast(Component* component) {
        return component ? dynamic_cast<T*>(component) : nullptr;
    }

    template<typename T>
    const T* SafeCast(const Component* component) {
        return component ? dynamic_cast<const T*>(component) : nullptr;
    }
}