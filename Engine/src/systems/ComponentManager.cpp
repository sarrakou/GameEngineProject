#include "../include/systems/ComponentManager.h"
#include "../include/components/Transform.h"
#include "../include/components/Behavior.h"
#include <iostream>
#include <algorithm>

// Static instance initialization
ComponentManager* ComponentManager::instance = nullptr;

ComponentManager& ComponentManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ComponentManager();
    }
    return *instance;
}

void ComponentManager::DestroyInstance() {
    delete instance;
    instance = nullptr;
}

ComponentManager::ComponentManager() {
    // Reserve space for common components
    allActiveComponents.reserve(1000);

    // Initialize built-in component types
    InitializeBuiltinComponents();

    std::cout << "ComponentManager initialized" << std::endl;
}

ComponentManager::~ComponentManager() {
    // Clean up all components
    for (auto& pair : componentsByType) {
        for (Component* component : pair.second) {
            delete component;
        }
    }

    componentsByType.clear();
    allActiveComponents.clear();
    componentPools.clear();

    std::cout << "ComponentManager destroyed" << std::endl;
}

// Component type registration checks
bool ComponentManager::IsComponentTypeRegistered(const std::type_index& typeIndex) const {
    return componentTypes.find(typeIndex) != componentTypes.end();
}

bool ComponentManager::IsComponentTypeRegistered(const std::string& typeName) const {
    return nameToType.find(typeName) != nameToType.end();
}

// Component creation by name/type
std::unique_ptr<Component> ComponentManager::CreateComponentByName(const std::string& typeName) {
    auto it = nameToType.find(typeName);
    if (it == nameToType.end()) {
        std::cerr << "Component type not registered: " << typeName << std::endl;
        return nullptr;
    }

    return CreateComponentByType(it->second);
}

std::unique_ptr<Component> ComponentManager::CreateComponentByType(const std::type_index& typeIndex) {
    auto it = componentTypes.find(typeIndex);
    if (it == componentTypes.end()) {
        std::cerr << "Component type not registered" << std::endl;
        return nullptr;
    }

    auto component = it->second.creator();
    if (component) {
        RegisterComponentInstance(component.get());
    }

    return component;
}

// Component destruction
void ComponentManager::DestroyComponent(Component* component) {
    if (!component) return;

    UnregisterComponentInstance(component);

    // Try to return to pool instead of deleting
    std::type_index typeIndex = std::type_index(typeid(*component));
    auto poolIt = componentPools.find(typeIndex);
    if (poolIt != componentPools.end() && poolIt->second->CanReturn()) {
        poolIt->second->Return(component);
    }
    else {
        delete component;
    }
}

// Component queries
std::vector<Component*> ComponentManager::GetComponentsOfType(const std::type_index& typeIndex) {
    auto it = componentsByType.find(typeIndex);
    if (it != componentsByType.end()) {
        return it->second;
    }
    return std::vector<Component*>();
}

std::vector<Component*> ComponentManager::GetComponentsOfType(const std::string& typeName) {
    auto it = nameToType.find(typeName);
    if (it != nameToType.end()) {
        return GetComponentsOfType(it->second);
    }
    return std::vector<Component*>();
}

// Component registration for tracking
void ComponentManager::RegisterComponentInstance(Component* component) {
    if (!component) return;

    std::type_index typeIndex = std::type_index(typeid(*component));

    // Add to type-specific storage
    componentsByType[typeIndex].push_back(component);

    // Add to global active components list
    allActiveComponents.push_back(component);

    MarkComponentsDirty();
}

void ComponentManager::UnregisterComponentInstance(Component* component) {
    if (!component) return;

    std::type_index typeIndex = std::type_index(typeid(*component));

    // Remove from type-specific storage
    auto& typeComponents = componentsByType[typeIndex];
    auto it = std::find(typeComponents.begin(), typeComponents.end(), component);
    if (it != typeComponents.end()) {
        typeComponents.erase(it);
    }

    // Remove from global active components list
    auto globalIt = std::find(allActiveComponents.begin(), allActiveComponents.end(), component);
    if (globalIt != allActiveComponents.end()) {
        allActiveComponents.erase(globalIt);
    }

    MarkComponentsDirty();
}

// Batch processing support
const std::vector<Component*>& ComponentManager::GetAllActiveComponents() {
    if (componentsDirty) {
        RefreshComponentCache();
    }
    return allActiveComponents;
}

void ComponentManager::RefreshComponentCache() {
    allActiveComponents.clear();

    for (const auto& pair : componentsByType) {
        for (Component* component : pair.second) {
            if (component && component->IsActive()) {
                allActiveComponents.push_back(component);
            }
        }
    }

    componentsDirty = false;
}

// Component statistics
size_t ComponentManager::GetActiveComponentCount() const {
    size_t count = 0;
    for (const auto& pair : componentsByType) {
        for (Component* component : pair.second) {
            if (component && component->IsActive()) {
                count++;
            }
        }
    }
    return count;
}

size_t ComponentManager::GetComponentCountOfType(const std::type_index& typeIndex) const {
    auto it = componentsByType.find(typeIndex);
    if (it != componentsByType.end()) {
        return it->second.size();
    }
    return 0;
}

// Memory management
void ComponentManager::SetComponentPoolSize(const std::type_index& typeIndex, size_t poolSize) {
    auto it = componentPools.find(typeIndex);
    if (it != componentPools.end()) {
        // Pool already exists, resize it
        // Note: ObjectPool would need a resize method for this to work fully
        std::cout << "Resizing component pool for type (not implemented)" << std::endl;
    }
    else {
        // Create new pool with specified size
        auto pool = std::make_unique<ObjectPool<Component>>(poolSize);
        componentPools[typeIndex] = std::move(pool);
    }
}

size_t ComponentManager::GetComponentPoolSize(const std::type_index& typeIndex) const {
    auto it = componentPools.find(typeIndex);
    if (it != componentPools.end()) {
        return it->second->GetCapacity();
    }
    return 0;
}

// Component type information
std::vector<std::string> ComponentManager::GetAllComponentTypeNames() const {
    std::vector<std::string> names;
    names.reserve(componentTypes.size());

    for (const auto& pair : componentTypes) {
        names.push_back(pair.second.typeName);
    }

    return names;
}

std::vector<std::type_index> ComponentManager::GetAllComponentTypes() const {
    std::vector<std::type_index> types;
    types.reserve(componentTypes.size());

    for (const auto& pair : componentTypes) {
        types.push_back(pair.first);
    }

    return types;
}

const ComponentTypeInfo* ComponentManager::GetComponentTypeInfo(const std::type_index& typeIndex) const {
    auto it = componentTypes.find(typeIndex);
    if (it != componentTypes.end()) {
        return &it->second;
    }
    return nullptr;
}

const ComponentTypeInfo* ComponentManager::GetComponentTypeInfo(const std::string& typeName) const {
    auto it = nameToType.find(typeName);
    if (it != nameToType.end()) {
        return GetComponentTypeInfo(it->second);
    }
    return nullptr;
}

// Utility and debugging
void ComponentManager::PrintComponentInfo() const {
    std::cout << "\n=== ComponentManager Info ===" << std::endl;
    std::cout << "Registered Component Types: " << componentTypes.size() << std::endl;
    std::cout << "Active Components: " << GetActiveComponentCount() << std::endl;
    std::cout << "Component Pools: " << componentPools.size() << std::endl;
}

void ComponentManager::PrintComponentTypeRegistry() const {
    std::cout << "\n=== Component Type Registry ===" << std::endl;

    for (const auto& pair : componentTypes) {
        const ComponentTypeInfo& info = pair.second;
        size_t instanceCount = GetComponentCountOfType(pair.first);

        std::cout << "Type: " << info.typeName
            << " | Size: " << info.typeSize << " bytes"
            << " | Instances: " << instanceCount << std::endl;
    }
}

void ComponentManager::PrintComponentStatistics() const {
    std::cout << "\n=== Component Statistics ===" << std::endl;

    for (const auto& pair : componentsByType) {
        auto typeInfo = GetComponentTypeInfo(pair.first);
        if (typeInfo) {
            size_t activeCount = 0;
            for (Component* comp : pair.second) {
                if (comp && comp->IsActive()) {
                    activeCount++;
                }
            }

            std::cout << typeInfo->typeName
                << " - Total: " << pair.second.size()
                << " | Active: " << activeCount
                << " | Memory: " << (pair.second.size() * typeInfo->typeSize) << " bytes"
                << std::endl;
        }
    }
}

// Component system update
void ComponentManager::UpdateAllComponents(float deltaTime) {
    const auto& components = GetAllActiveComponents();

    for (Component* component : components) {
        if (component && component->IsActive()) {
            component->Update(deltaTime);
        }
    }
}

// Private helpers
std::type_index ComponentManager::GetTypeIndexFromName(const std::string& typeName) const {
    auto it = nameToType.find(typeName);
    if (it != nameToType.end()) {
        return it->second;
    }
    throw std::runtime_error("Component type not found: " + typeName);
}

void ComponentManager::InitializeBuiltinComponents() {
    // Register built-in component types
    RegisterComponentType<Transform>("Transform");
    RegisterComponentType<Behavior>("Behavior");

    // You can add more built-in types here
    // RegisterComponentType<Renderer>("Renderer");
    // RegisterComponentType<Collider>("Collider");
    // RegisterComponentType<RigidBody>("RigidBody");

    std::cout << "Built-in component types registered" << std::endl;
}

// Component factory helper functions
namespace ComponentFactory {

    template<typename T>
    void RegisterComponent(const std::string& name = "") {
        ComponentManager::GetInstance().RegisterComponentType<T>(name);
    }

    template<typename T, typename... Args>
    T* Create(Args&&... args) {
        return ComponentManager::GetInstance().CreateComponent<T>(std::forward<Args>(args)...);
    }

    std::unique_ptr<Component> CreateByName(const std::string& typeName) {
        return ComponentManager::GetInstance().CreateComponentByName(typeName);
    }

    template<typename T>
    std::vector<T*> GetAll() {
        return ComponentManager::GetInstance().GetComponentsOfType<T>();
    }

    // Batch operations
    template<typename T>
    void UpdateAllOfType(float deltaTime) {
        auto components = ComponentManager::GetInstance().GetComponentsOfType<T>();
        for (T* component : components) {
            if (component && component->IsActive()) {
                component->Update(deltaTime);
            }
        }
    }
}