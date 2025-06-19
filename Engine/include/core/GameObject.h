#pragma once

#include "../components/Component.h"
#include <vector>
#include <memory>
#include <string>
#include <typeinfo>
#include <algorithm>

class GameObject {
private:
    static size_t nextId;
    size_t id;
    std::string tag;
    std::vector<std::unique_ptr<Component>> components;
    bool active = true;

public:
    // Constructor
    GameObject(const std::string& objectTag = "");

    // Destructor
    ~GameObject() = default;

    // Move constructor and assignment (for efficiency)
    GameObject(GameObject&& other) noexcept;
    GameObject& operator=(GameObject&& other) noexcept;

    // Delete copy constructor and assignment (unique ownership)
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

    // ID and Tag management
    size_t GetId() const { return id; }
    const std::string& GetTag() const { return tag; }
    void SetTag(const std::string& newTag) { tag = newTag; }

    // Active state
    bool IsActive() const { return active; }
    void SetActive(bool isActive) { active = isActive; }

    // Component management
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

        // Check if component already exists (optional: enforce single component per type)
        if (GetComponent<T>() != nullptr) {
            return GetComponent<T>(); // Return existing component
        }

        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* componentPtr = component.get();

        // Set the owner reference
        component->SetOwner(this);

        components.push_back(std::move(component));
        return componentPtr;
    }

    template<typename T>
    T* GetComponent() {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

        for (auto& component : components) {
            if (T* castedComponent = dynamic_cast<T*>(component.get())) {
                return castedComponent;
            }
        }
        return nullptr;
    }

    template<typename T>
    const T* GetComponent() const {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

        for (const auto& component : components) {
            if (const T* castedComponent = dynamic_cast<const T*>(component.get())) {
                return castedComponent;
            }
        }
        return nullptr;
    }

    template<typename T>
    bool HasComponent() const {
        return GetComponent<T>() != nullptr;
    }

    template<typename T>
    bool RemoveComponent() {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

        auto it = std::find_if(components.begin(), components.end(),
            [](const std::unique_ptr<Component>& component) {
                return dynamic_cast<T*>(component.get()) != nullptr;
            });

        if (it != components.end()) {
            components.erase(it);
            return true;
        }
        return false;
    }

    // Get all components (useful for data-oriented processing)
    const std::vector<std::unique_ptr<Component>>& GetAllComponents() const {
        return components;
    }

    // Component count
    size_t GetComponentCount() const {
        return components.size();
    }

    // Update all components (called by systems)
    void Update(float deltaTime);

    // Debug/utility functions
    void PrintInfo() const;
};