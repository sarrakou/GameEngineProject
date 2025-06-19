#include "../include/core/GameObject.h"
#include "../include/components/Behavior.h"  // Include for Behavior type checking
#include <iostream>

// Static member initialization
size_t GameObject::nextId = 0;

// Updated constructor with name parameter
GameObject::GameObject(const std::string& objectTag, const std::string& objectName)
    : id(nextId++), tag(objectTag), name(objectName) {
    components.reserve(8); // Reserve space for typical component count
}

GameObject::GameObject(GameObject&& other) noexcept
    : id(other.id)
    , tag(std::move(other.tag))
    , name(std::move(other.name))  // Move name as well
    , components(std::move(other.components))
    , active(other.active) {

    // Update component owner references
    for (auto& component : components) {
        component->SetOwner(this);
    }
}

GameObject& GameObject::operator=(GameObject&& other) noexcept {
    if (this != &other) {
        id = other.id;
        tag = std::move(other.tag);
        name = std::move(other.name);  // Move name as well
        components = std::move(other.components);
        active = other.active;

        // Update component owner references
        for (auto& component : components) {
            component->SetOwner(this);
        }
    }
    return *this;
}

// Enhanced SetActive with component lifecycle management
void GameObject::SetActive(bool isActive) {
    if (active == isActive) return;  // No change needed

    bool wasActive = active;
    active = isActive;

    // Notify all components about the state change
    for (auto& component : components) {
        if (isActive && !wasActive) {
            // GameObject became active
            if (component->IsActive()) {
                component->OnEnable();
            }
        }
        else if (!isActive && wasActive) {
            // GameObject became inactive
            if (component->IsActive()) {
                component->OnDisable();
            }
        }
    }
}

void GameObject::Update(float deltaTime) {
    if (!active) return;

    // Update all components
    for (auto& component : components) {
        if (component->IsActive()) {
            component->Update(deltaTime);
        }
    }
}

// Enhanced PrintInfo using RTTI
void GameObject::PrintInfo() const {
    std::cout << "\n=== GameObject Info ===" << std::endl;
    std::cout << "ID: " << id << std::endl;
    std::cout << "Name: " << (name.empty() ? "Unnamed" : name) << std::endl;
    std::cout << "Tag: " << (tag.empty() ? "Untagged" : tag) << std::endl;
    std::cout << "Active: " << (active ? "true" : "false") << std::endl;
    std::cout << "Components (" << components.size() << "):" << std::endl;

    if (components.empty()) {
        std::cout << "  (No components)" << std::endl;
    }
    else {
        for (const auto& component : components) {
            std::cout << "  - " << component->GetDisplayName()
                << std::endl;
            std::cout << "    Type: " << component->GetTypeName()
                << std::endl;
            std::cout << "    Active: " << (component->IsActive() ? "true" : "false")
                << std::endl;

            // Show additional info for behaviors
            if (component->IsOfType<Behavior>()) {
                std::cout << "    Category: Behavior Component" << std::endl;
            }
            std::cout << std::endl;
        }
    }
    std::cout << "===================" << std::endl;
}

// Implementation of utility methods
std::vector<std::string> GameObject::GetComponentTypeNames() const {
    std::vector<std::string> names;
    names.reserve(components.size());

    for (const auto& component : components) {
        names.push_back(component->GetDisplayName());
    }
    return names;
}

std::vector<std::string> GameObject::GetComponentRTTINames() const {
    std::vector<std::string> names;
    names.reserve(components.size());

    for (const auto& component : components) {
        names.push_back(component->GetTypeName());
    }
    return names;
}

bool GameObject::HasConflictingComponents(const std::vector<std::string>& conflictingTypes) const {
    if (conflictingTypes.empty()) return false;

    std::vector<std::string> foundTypes;

    for (const auto& component : components) {
        std::string displayName = component->GetDisplayName();

        for (const std::string& conflictType : conflictingTypes) {
            if (displayName.find(conflictType) != std::string::npos) {
                foundTypes.push_back(displayName);
                break;  // Don't add the same component multiple times
            }
        }
    }

    return foundTypes.size() > 1;
}

bool GameObject::RemoveComponent(Component* component) {
    if (!component) return false;

    auto it = std::find_if(components.begin(), components.end(),
        [component](const std::unique_ptr<Component>& comp) {
            return comp.get() == component;
        });

    if (it != components.end()) {
        (*it)->OnDisable();  // Disable first
        (*it)->OnDestroy();  // Then destroy
        components.erase(it);
        return true;
    }
    return false;
}

// Implementation of behavior-specific methods
std::vector<Behavior*> GameObject::GetBehaviors() {
    return GetComponents<Behavior>();
}

std::vector<const Behavior*> GameObject::GetBehaviors() const {
    return GetComponents<Behavior>();
}

void GameObject::EnableAllComponents() {
    for (auto& component : components) {
        if (!component->IsActive()) {
            component->SetActive(true);
            if (active) {  // Only call OnEnable if GameObject is also active
                component->OnEnable();
            }
        }
    }
}

void GameObject::DisableAllComponents() {
    for (auto& component : components) {
        if (component->IsActive()) {
            component->OnDisable();
            component->SetActive(false);
        }
    }
}