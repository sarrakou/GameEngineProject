#pragma once
#include "../components/Component.h"
#include <vector>
#include <memory>
#include <string>
#include <typeinfo>
#include <algorithm>
#include <iostream>

// Forward declaration to avoid circular dependency
class Behavior;

class GameObject {
private:
    static size_t nextId;
    size_t id;
    std::string tag;
    std::string name;  // Added name field
    std::vector<std::unique_ptr<Component>> components;
    bool active = true;

public:
    // Constructor - added name parameter
    GameObject(const std::string& objectTag = "", const std::string& objectName = "");

    // Destructor
    ~GameObject() = default;

    // Move constructor and assignment (for efficiency)
    GameObject(GameObject&& other) noexcept;
    GameObject& operator=(GameObject&& other) noexcept;

    // Delete copy constructor and assignment (unique ownership)
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

    // ===== ID, NAME, AND TAG MANAGEMENT =====
    size_t GetId() const { return id; }

    const std::string& GetTag() const { return tag; }
    void SetTag(const std::string& newTag) { tag = newTag; }

    // Added name management
    const std::string& GetName() const { return name; }
    void SetName(const std::string& newName) { name = newName; }

    // Active state
    bool IsActive() const { return active; }
    void SetActive(bool isActive);  // Move implementation to .cpp for component notifications

    // ===== ENHANCED COMPONENT MANAGEMENT =====

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

        // Optional: Check if component already exists
        if (GetComponent<T>() != nullptr) {
            return GetComponent<T>(); // Return existing component
        }

        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* componentPtr = component.get();

        // Set the owner reference
        component->SetOwner(this);
        components.push_back(std::move(component));

        // Call OnEnable if GameObject is active
        if (active) {
            componentPtr->OnEnable();
        }

        return componentPtr;
    }

    // Enhanced with Component's RTTI helpers
    template<typename T>
    T* GetComponent() {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        for (auto& component : components) {
            // Use Component's RTTI helper instead of raw dynamic_cast
            if (T* result = component->As<T>()) {
                return result;
            }
        }
        return nullptr;
    }

    template<typename T>
    const T* GetComponent() const {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        for (const auto& component : components) {
            // Use Component's RTTI helper
            if (const T* result = component->As<T>()) {
                return result;
            }
        }
        return nullptr;
    }

    // NEW: Get all components of a specific type
    template<typename T>
    std::vector<T*> GetComponents() {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        std::vector<T*> result;
        for (auto& component : components) {
            if (T* typedComp = component->As<T>()) {
                result.push_back(typedComp);
            }
        }
        return result;
    }

    template<typename T>
    std::vector<const T*> GetComponents() const {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        std::vector<const T*> result;
        for (const auto& component : components) {
            if (const T* typedComp = component->As<T>()) {
                result.push_back(typedComp);
            }
        }
        return result;
    }

    template<typename T>
    bool HasComponent() const {
        return GetComponent<T>() != nullptr;
    }

    // Enhanced RemoveComponent with proper cleanup
    template<typename T>
    bool RemoveComponent() {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        auto it = std::find_if(components.begin(), components.end(),
            [](const std::unique_ptr<Component>& component) {
                return component->IsOfType<T>();  // Use RTTI helper
            });

        if (it != components.end()) {
            (*it)->OnDestroy();  // Proper cleanup
            components.erase(it);
            return true;
        }
        return false;
    }

    // NEW: Remove all components of a specific type
    template<typename T>
    int RemoveComponents() {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        int removedCount = 0;
        auto it = components.begin();
        while (it != components.end()) {
            if ((*it)->IsOfType<T>()) {
                (*it)->OnDestroy();
                it = components.erase(it);
                removedCount++;
            }
            else {
                ++it;
            }
        }
        return removedCount;
    }

    // Remove component by pointer
    bool RemoveComponent(Component* component);

    // Get all components (useful for data-oriented processing)
    const std::vector<std::unique_ptr<Component>>& GetAllComponents() const {
        return components;
    }

    std::vector<std::unique_ptr<Component>>& GetAllComponents() {
        return components;
    }

    // Component count
    size_t GetComponentCount() const {
        return components.size();
    }

    // Update all components (called by systems)
    void Update(float deltaTime);

    // ===== NEW: RTTI DEBUG AND UTILITY FUNCTIONS =====

    // Enhanced debug function using RTTI
    void PrintInfo() const;

    // Get component type names for serialization/debugging
    std::vector<std::string> GetComponentTypeNames() const;

    // Get component RTTI type names
    std::vector<std::string> GetComponentRTTINames() const;

    // Check for component type conflicts
    bool HasConflictingComponents(const std::vector<std::string>& conflictingTypes) const;

    // Count components of a specific type
    template<typename T>
    size_t CountComponents() const {
        return GetComponents<T>().size();
    }

    // ===== BEHAVIOR-SPECIFIC METHODS (DECLARATIONS ONLY) =====
    // These are declared here but implemented in .cpp to avoid circular dependency

    // Check if GameObject has any behaviors
    bool HasBehavior() const;

    // Get all behaviors
    std::vector<Behavior*> GetBehaviors();
    std::vector<const Behavior*> GetBehaviors() const;

    // Enable/disable all components
    void EnableAllComponents();
    void DisableAllComponents();

    // Additional debug methods
    void PrintComponentHierarchy() const;
    void CheckForComponentConflicts() const;
};