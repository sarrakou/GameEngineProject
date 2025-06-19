#pragma once

#include "../components/Component.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>
#include <typeindex>
#include <vector>

// Forward declarations
class Transform;
class Behavior;

// Component creation configuration
struct ComponentConfig {
    std::string typeName;
    std::unordered_map<std::string, std::string> properties;

    // Default constructor
    ComponentConfig() = default;

    ComponentConfig(const std::string& type) : typeName(type) {}

    // Helper methods for setting properties
    ComponentConfig& SetProperty(const std::string& key, const std::string& value) {
        properties[key] = value;
        return *this;
    }

    ComponentConfig& SetFloat(const std::string& key, float value) {
        properties[key] = std::to_string(value);
        return *this;
    }

    ComponentConfig& SetInt(const std::string& key, int value) {
        properties[key] = std::to_string(value);
        return *this;
    }

    ComponentConfig& SetBool(const std::string& key, bool value) {
        properties[key] = value ? "true" : "false";
        return *this;
    }

    // Property getters
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const {
        auto it = properties.find(key);
        return (it != properties.end()) ? it->second : defaultValue;
    }

    float GetFloat(const std::string& key, float defaultValue = 0.0f) const {
        auto it = properties.find(key);
        return (it != properties.end()) ? std::stof(it->second) : defaultValue;
    }

    int GetInt(const std::string& key, int defaultValue = 0) const {
        auto it = properties.find(key);
        return (it != properties.end()) ? std::stoi(it->second) : defaultValue;
    }

    bool GetBool(const std::string& key, bool defaultValue = false) const {
        auto it = properties.find(key);
        if (it != properties.end()) {
            return it->second == "true" || it->second == "1";
        }
        return defaultValue;
    }
};

// Component factory registration info
struct ComponentFactoryInfo {
    std::string typeName;
    std::type_index typeIndex;
    std::function<std::unique_ptr<Component>()> defaultCreator;
    std::function<std::unique_ptr<Component>(const ComponentConfig&)> configCreator;

    ComponentFactoryInfo(const std::string& name, std::type_index index,
        std::function<std::unique_ptr<Component>()> defaultFunc,
        std::function<std::unique_ptr<Component>(const ComponentConfig&)> configFunc)
        : typeName(name), typeIndex(index), defaultCreator(defaultFunc), configCreator(configFunc) {
    }
};

// ComponentFactory: Data-driven component creation (REQUIREMENT #4)
class ComponentFactory {
private:
    // Factory registry
    std::unordered_map<std::string, ComponentFactoryInfo> componentFactories;
    std::unordered_map<std::type_index, std::string> typeToName;

    // Component ID system for data-driven creation
    std::unordered_map<std::string, size_t> componentIds;
    std::unordered_map<size_t, std::string> idToName;
    size_t nextId = 1;

    // Singleton instance
    static ComponentFactory* instance;

public:
    // Singleton access
    static ComponentFactory& GetInstance();
    static void DestroyInstance();

    // Constructor and destructor
    ComponentFactory();
    ~ComponentFactory() = default;

    // Delete copy operations
    ComponentFactory(const ComponentFactory&) = delete;
    ComponentFactory& operator=(const ComponentFactory&) = delete;

    // Component type registration
    template<typename T>
    void RegisterComponent(const std::string& typeName);

    template<typename T>
    void RegisterComponentWithConfig(const std::string& typeName,
        std::function<std::unique_ptr<T>(const ComponentConfig&)> configCreator);

    bool IsComponentRegistered(const std::string& typeName) const;
    bool IsComponentRegistered(size_t componentId) const;

    // Component creation by name
    std::unique_ptr<Component> CreateComponent(const std::string& typeName);
    std::unique_ptr<Component> CreateComponent(const std::string& typeName, const ComponentConfig& config);

    // Component creation by ID
    std::unique_ptr<Component> CreateComponent(size_t componentId);
    std::unique_ptr<Component> CreateComponent(size_t componentId, const ComponentConfig& config);

    // Batch component creation
    std::vector<std::unique_ptr<Component>> CreateComponents(const std::vector<std::string>& typeNames);
    std::vector<std::unique_ptr<Component>> CreateComponents(const std::vector<ComponentConfig>& configs);

    // Component ID management
    size_t GetComponentId(const std::string& typeName) const;
    std::string GetComponentName(size_t componentId) const;

    // Factory information
    std::vector<std::string> GetRegisteredComponentNames() const;
    std::vector<size_t> GetRegisteredComponentIds() const;
    size_t GetRegisteredComponentCount() const { return componentFactories.size(); }

    // Data-driven creation from strings/files
    std::unique_ptr<Component> CreateFromString(const std::string& componentData);
    std::vector<std::unique_ptr<Component>> CreateFromFile(const std::string& filepath);

    // Component presets/templates
    void RegisterPreset(const std::string& presetName, const ComponentConfig& config);
    std::unique_ptr<Component> CreateFromPreset(const std::string& presetName);
    bool HasPreset(const std::string& presetName) const;

    // Debug and utilities
    void PrintRegisteredComponents() const;
    void PrintFactoryInfo() const;

private:
    // Internal helpers
    void InitializeBuiltinComponents();
    size_t AssignComponentId(const std::string& typeName);

    // Preset storage
    std::unordered_map<std::string, ComponentConfig> presets;
};

// Template implementations
template<typename T>
void ComponentFactory::RegisterComponent(const std::string& typeName) {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

    if (IsComponentRegistered(typeName)) {
        std::cout << "Component already registered: " << typeName << std::endl;
        return;
    }

    std::type_index typeIndex = std::type_index(typeid(T));

    // Default creator function
    auto defaultCreator = []() -> std::unique_ptr<Component> {
        return std::make_unique<T>();
        };

    // Config-based creator function (default implementation)
    auto configCreator = [](const ComponentConfig& config) -> std::unique_ptr<Component> {
        auto component = std::make_unique<T>();
        // Apply configuration properties if the component supports it
        // This would be overridden for specific component types
        return component;
        };

    ComponentFactoryInfo info(typeName, typeIndex, defaultCreator, configCreator);
    componentFactories.emplace(typeName, info);
    typeToName.emplace(typeIndex, typeName);

    // Assign unique ID
    size_t id = AssignComponentId(typeName);

    std::cout << "Registered component: " << typeName << " (ID: " << id << ")" << std::endl;
}

template<typename T>
void ComponentFactory::RegisterComponentWithConfig(const std::string& typeName,
    std::function<std::unique_ptr<T>(const ComponentConfig&)> configCreator) {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

    if (IsComponentRegistered(typeName)) {
        std::cout << "Component already registered: " << typeName << std::endl;
        return;
    }

    std::type_index typeIndex = std::type_index(typeid(T));

    // Default creator
    auto defaultCreator = []() -> std::unique_ptr<Component> {
        return std::make_unique<T>();
        };

    // Wrap the typed config creator to return Component*
    auto wrappedConfigCreator = [configCreator](const ComponentConfig& config) -> std::unique_ptr<Component> {
        return configCreator(config);
        };

    ComponentFactoryInfo info(typeName, typeIndex, defaultCreator, wrappedConfigCreator);
    componentFactories.emplace(typeName, info);
    typeToName.emplace(typeIndex, typeName);

    // Assign unique ID
    size_t id = AssignComponentId(typeName);

    std::cout << "Registered component with config: " << typeName << " (ID: " << id << ")" << std::endl;
}

// Convenience macros for factory usage
#define COMPONENT_FACTORY ComponentFactory::GetInstance()
#define REGISTER_COMPONENT_FACTORY(Type) ComponentFactory::GetInstance().RegisterComponent<Type>(#Type)
#define CREATE_COMPONENT_BY_NAME(name) ComponentFactory::GetInstance().CreateComponent(name)
#define CREATE_COMPONENT_BY_ID(id) ComponentFactory::GetInstance().CreateComponent(id)