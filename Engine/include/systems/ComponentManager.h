#pragma once

#include "../components/Component.h"
#include "../memory/ObjectPool.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <typeinfo>
#include <typeindex>
#include <functional>
#include <string>

// Forward declarations
class GameObject;
class Transform;
class Behavior;

// Component type information
struct ComponentTypeInfo {
    std::type_index typeIndex;
    std::string typeName;
    size_t typeSize;
    std::function<std::unique_ptr<Component>()> creator;

    ComponentTypeInfo(std::type_index index, const std::string& name, size_t size,
        std::function<std::unique_ptr<Component>()> creatorFunc)
        : typeIndex(index), typeName(name), typeSize(size), creator(creatorFunc) {
    }
};

// ComponentManager: Manages all component types and instances
class ComponentManager {
private:
    // Component type registry
    std::unordered_map<std::type_index, ComponentTypeInfo> componentTypes;
    std::unordered_map<std::string, std::type_index> nameToType;

    // Component storage for Data-Oriented Design
    std::unordered_map<std::type_index, std::vector<Component*>> componentsByType;

    // Component pools for memory management (no allocation during gameplay)
    std::unordered_map<std::type_index, std::unique_ptr<ObjectPool<Component>>> componentPools;

    // Active components tracking
    std::vector<Component*> allActiveComponents;
    bool componentsDirty = true;

    // Singleton instance
    static ComponentManager* instance;

public:
    // Singleton access
    static ComponentManager& GetInstance();
    static void DestroyInstance();

    // Constructor and destructor
    ComponentManager();
    ~ComponentManager();

    // Delete copy operations
    ComponentManager(const ComponentManager&) = delete;
    ComponentManager& operator=(const ComponentManager&) = delete;

    // Component type registration
    template<typename T>
    void RegisterComponentType(const std::string& typeName = "");

    bool IsComponentTypeRegistered(const std::type_index& typeIndex) const;
    bool IsComponentTypeRegistered(const std::string& typeName) const;

    template<typename T>
    bool IsComponentTypeRegistered() const;

    // Component creation (Factory pattern - Requirement #4)
    template<typename T, typename... Args>
    T* CreateComponent(Args&&... args);

    std::unique_ptr<Component> CreateComponentByName(const std::string& typeName);
    std::unique_ptr<Component> CreateComponentByType(const std::type_index& typeIndex);

    // Component destruction
    template<typename T>
    void DestroyComponent(T* component);

    void DestroyComponent(Component* component);

    // Component queries (Data-Oriented Design support)
    template<typename T>
    std::vector<T*> GetComponentsOfType();

    template<typename T>
    T* GetFirstComponentOfType();

    std::vector<Component*> GetComponentsOfType(const std::type_index& typeIndex);
    std::vector<Component*> GetComponentsOfType(const std::string& typeName);

    // Component registration for tracking
    void RegisterComponentInstance(Component* component);
    void UnregisterComponentInstance(Component* component);

    // Batch processing support
    const std::vector<Component*>& GetAllActiveComponents();
    void RefreshComponentCache();

    // Component statistics
    size_t GetComponentTypeCount() const { return componentTypes.size(); }
    size_t GetActiveComponentCount() const;
    size_t GetComponentCountOfType(const std::type_index& typeIndex) const;

    template<typename T>
    size_t GetComponentCountOfType() const;

    // Memory management
    void SetComponentPoolSize(const std::type_index& typeIndex, size_t poolSize);

    template<typename T>
    void SetComponentPoolSize(size_t poolSize);

    size_t GetComponentPoolSize(const std::type_index& typeIndex) const;

    // Component type information
    std::vector<std::string> GetAllComponentTypeNames() const;
    std::vector<std::type_index> GetAllComponentTypes() const;
    const ComponentTypeInfo* GetComponentTypeInfo(const std::type_index& typeIndex) const;
    const ComponentTypeInfo* GetComponentTypeInfo(const std::string& typeName) const;

    // Utility and debugging
    void PrintComponentInfo() const;
    void PrintComponentTypeRegistry() const;
    void PrintComponentStatistics() const;

    // Component system update
    void UpdateAllComponents(float deltaTime);

private:
    // Internal helpers
    std::type_index GetTypeIndexFromName(const std::string& typeName) const;
    void InitializeBuiltinComponents();
    void MarkComponentsDirty() { componentsDirty = true; }

    // Component pool management
    template<typename T>
    ObjectPool<Component>* GetOrCreatePool();
};

// Template implementations
template<typename T>
void ComponentManager::RegisterComponentType(const std::string& typeName) {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

    std::type_index typeIndex = std::type_index(typeid(T));

    // Don't register twice
    if (IsComponentTypeRegistered(typeIndex)) {
        return;
    }

    std::string name = typeName.empty() ? typeid(T).name() : typeName;

    // Create component factory function
    auto creator = []() -> std::unique_ptr<Component> {
        return std::make_unique<T>();
        };

    ComponentTypeInfo info(typeIndex, name, sizeof(T), creator);
    componentTypes.emplace(typeIndex, info);
    nameToType.emplace(name, typeIndex);

    // Initialize component storage
    componentsByType[typeIndex] = std::vector<Component*>();

    std::cout << "Registered component type: " << name << std::endl;
}

template<typename T>
bool ComponentManager::IsComponentTypeRegistered() const {
    return IsComponentTypeRegistered(std::type_index(typeid(T)));
}

template<typename T, typename... Args>
T* ComponentManager::CreateComponent(Args&&... args) {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

    std::type_index typeIndex = std::type_index(typeid(T));

    // Ensure type is registered
    if (!IsComponentTypeRegistered(typeIndex)) {
        RegisterComponentType<T>();
    }

    // Try to get from pool first (memory management)
    ObjectPool<Component>* pool = GetOrCreatePool<T>();
    if (pool && pool->HasAvailable()) {
        Component* component = pool->Get();
        T* typedComponent = static_cast<T*>(component);

        // Re-initialize with new parameters if needed
        // For now, just return the pooled component
        RegisterComponentInstance(component);
        return typedComponent;
    }

    // Create new component if pool is empty
    auto component = std::make_unique<T>(std::forward<Args>(args)...);
    T* componentPtr = component.release(); // Transfer ownership

    RegisterComponentInstance(componentPtr);
    return componentPtr;
}

template<typename T>
void ComponentManager::DestroyComponent(T* component) {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    DestroyComponent(static_cast<Component*>(component));
}

template<typename T>
std::vector<T*> ComponentManager::GetComponentsOfType() {
    std::type_index typeIndex = std::type_index(typeid(T));
    std::vector<T*> result;

    auto it = componentsByType.find(typeIndex);
    if (it != componentsByType.end()) {
        result.reserve(it->second.size());
        for (Component* component : it->second) {
            if (T* typedComponent = dynamic_cast<T*>(component)) {
                result.push_back(typedComponent);
            }
        }
    }

    return result;
}

template<typename T>
T* ComponentManager::GetFirstComponentOfType() {
    auto components = GetComponentsOfType<T>();
    return components.empty() ? nullptr : components[0];
}

template<typename T>
size_t ComponentManager::GetComponentCountOfType() const {
    return GetComponentCountOfType(std::type_index(typeid(T)));
}

template<typename T>
void ComponentManager::SetComponentPoolSize(size_t poolSize) {
    SetComponentPoolSize(std::type_index(typeid(T)), poolSize);
}

template<typename T>
ObjectPool<Component>* ComponentManager::GetOrCreatePool() {
    std::type_index typeIndex = std::type_index(typeid(T));

    auto it = componentPools.find(typeIndex);
    if (it == componentPools.end()) {
        // Create new pool
        auto pool = std::make_unique<ObjectPool<Component>>(10); // Default size
        ObjectPool<Component>* poolPtr = pool.get();
        componentPools[typeIndex] = std::move(pool);
        return poolPtr;
    }

    return it->second.get();
}

// Convenience macros
#define COMPONENT_MANAGER ComponentManager::GetInstance()
#define REGISTER_COMPONENT(Type) ComponentManager::GetInstance().RegisterComponentType<Type>(#Type)
#define CREATE_COMPONENT(Type, ...) ComponentManager::GetInstance().CreateComponent<Type>(__VA_ARGS__)
#define GET_COMPONENTS(Type) ComponentManager::GetInstance().GetComponentsOfType<Type>()