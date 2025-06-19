#include "../include/core/GameObject.h"
#include <iostream>

// Static member initialization
size_t GameObject::nextId = 0;

GameObject::GameObject(const std::string& objectTag)
    : id(nextId++), tag(objectTag) {
    components.reserve(8); // Reserve space for typical component count
}

GameObject::GameObject(GameObject&& other) noexcept
    : id(other.id)
    , tag(std::move(other.tag))
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
        components = std::move(other.components);
        active = other.active;

        // Update component owner references
        for (auto& component : components) {
            component->SetOwner(this);
        }
    }
    return *this;
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

void GameObject::PrintInfo() const {
    std::cout << "GameObject [ID: " << id << ", Tag: '" << tag
        << "', Active: " << (active ? "true" : "false")
        << ", Components: " << components.size() << "]\n";

    for (const auto& component : components) {
        std::cout << "  - " << typeid(*component).name()
            << " (Active: " << (component->IsActive() ? "true" : "false") << ")\n";
    }
}