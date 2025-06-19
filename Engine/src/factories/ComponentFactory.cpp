#include "../include/factories/ComponentFactory.h"
#include "../include/components/Transform.h"
#include "../include/components/Behavior.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// Static instance initialization
ComponentFactory* ComponentFactory::instance = nullptr;

ComponentFactory& ComponentFactory::GetInstance() {
    if (instance == nullptr) {
        instance = new ComponentFactory();
    }
    return *instance;
}

void ComponentFactory::DestroyInstance() {
    delete instance;
    instance = nullptr;
}

ComponentFactory::ComponentFactory() {
    InitializeBuiltinComponents();
    std::cout << "ComponentFactory initialized" << std::endl;
}

// Component registration checks
bool ComponentFactory::IsComponentRegistered(const std::string& typeName) const {
    return componentFactories.find(typeName) != componentFactories.end();
}

bool ComponentFactory::IsComponentRegistered(size_t componentId) const {
    return idToName.find(componentId) != idToName.end();
}

// Component creation by name
std::unique_ptr<Component> ComponentFactory::CreateComponent(const std::string& typeName) {
    auto it = componentFactories.find(typeName);
    if (it == componentFactories.end()) {
        std::cerr << "Component not registered: " << typeName << std::endl;
        return nullptr;
    }

    return it->second.defaultCreator();
}

std::unique_ptr<Component> ComponentFactory::CreateComponent(const std::string& typeName, const ComponentConfig& config) {
    auto it = componentFactories.find(typeName);
    if (it == componentFactories.end()) {
        std::cerr << "Component not registered: " << typeName << std::endl;
        return nullptr;
    }

    return it->second.configCreator(config);
}

// Component creation by ID
std::unique_ptr<Component> ComponentFactory::CreateComponent(size_t componentId) {
    auto it = idToName.find(componentId);
    if (it == idToName.end()) {
        std::cerr << "Component ID not found: " << componentId << std::endl;
        return nullptr;
    }

    return CreateComponent(it->second);
}

std::unique_ptr<Component> ComponentFactory::CreateComponent(size_t componentId, const ComponentConfig& config) {
    auto it = idToName.find(componentId);
    if (it == idToName.end()) {
        std::cerr << "Component ID not found: " << componentId << std::endl;
        return nullptr;
    }

    return CreateComponent(it->second, config);
}

// Batch component creation
std::vector<std::unique_ptr<Component>> ComponentFactory::CreateComponents(const std::vector<std::string>& typeNames) {
    std::vector<std::unique_ptr<Component>> components;
    components.reserve(typeNames.size());

    for (const std::string& typeName : typeNames) {
        auto component = CreateComponent(typeName);
        if (component) {
            components.push_back(std::move(component));
        }
    }

    return components;
}

std::vector<std::unique_ptr<Component>> ComponentFactory::CreateComponents(const std::vector<ComponentConfig>& configs) {
    std::vector<std::unique_ptr<Component>> components;
    components.reserve(configs.size());

    for (const ComponentConfig& config : configs) {
        auto component = CreateComponent(config.typeName, config);
        if (component) {
            components.push_back(std::move(component));
        }
    }

    return components;
}

// Component ID management
size_t ComponentFactory::GetComponentId(const std::string& typeName) const {
    auto it = componentIds.find(typeName);
    if (it != componentIds.end()) {
        return it->second;
    }
    return 0; // Invalid ID
}

std::string ComponentFactory::GetComponentName(size_t componentId) const {
    auto it = idToName.find(componentId);
    if (it != idToName.end()) {
        return it->second;
    }
    return ""; // Invalid name
}

// Factory information
std::vector<std::string> ComponentFactory::GetRegisteredComponentNames() const {
    std::vector<std::string> names;
    names.reserve(componentFactories.size());

    for (const auto& pair : componentFactories) {
        names.push_back(pair.first);
    }

    return names;
}

std::vector<size_t> ComponentFactory::GetRegisteredComponentIds() const {
    std::vector<size_t> ids;
    ids.reserve(componentIds.size());

    for (const auto& pair : componentIds) {
        ids.push_back(pair.second);
    }

    return ids;
}

// Data-driven creation from strings/files
std::unique_ptr<Component> ComponentFactory::CreateFromString(const std::string& componentData) {
    // Simple format: "ComponentType:property1=value1,property2=value2"
    std::stringstream ss(componentData);
    std::string typeName;

    if (!std::getline(ss, typeName, ':')) {
        std::cerr << "Invalid component data format" << std::endl;
        return nullptr;
    }

    ComponentConfig config(typeName);

    std::string propertiesStr;
    if (std::getline(ss, propertiesStr)) {
        std::stringstream propStream(propertiesStr);
        std::string property;

        while (std::getline(propStream, property, ',')) {
            size_t equalPos = property.find('=');
            if (equalPos != std::string::npos) {
                std::string key = property.substr(0, equalPos);
                std::string value = property.substr(equalPos + 1);
                config.SetProperty(key, value);
            }
        }
    }

    return CreateComponent(typeName, config);
}

std::vector<std::unique_ptr<Component>> ComponentFactory::CreateFromFile(const std::string& filepath) {
    std::vector<std::unique_ptr<Component>> components;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Failed to open component file: " << filepath << std::endl;
        return components;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#') { // Skip empty lines and comments
            auto component = CreateFromString(line);
            if (component) {
                components.push_back(std::move(component));
            }
        }
    }

    file.close();
    std::cout << "Loaded " << components.size() << " components from " << filepath << std::endl;
    return components;
}

// Component presets/templates
void ComponentFactory::RegisterPreset(const std::string& presetName, const ComponentConfig& config) {
    presets[presetName] = config;
    std::cout << "Registered component preset: " << presetName << std::endl;
}

std::unique_ptr<Component> ComponentFactory::CreateFromPreset(const std::string& presetName) {
    auto it = presets.find(presetName);
    if (it == presets.end()) {
        std::cerr << "Component preset not found: " << presetName << std::endl;
        return nullptr;
    }

    return CreateComponent(it->second.typeName, it->second);
}

bool ComponentFactory::HasPreset(const std::string& presetName) const {
    return presets.find(presetName) != presets.end();
}

// Debug and utilities
void ComponentFactory::PrintRegisteredComponents() const {
    std::cout << "\n=== Registered Components ===" << std::endl;

    for (const auto& pair : componentFactories) {
        size_t id = GetComponentId(pair.first);
        std::cout << "- " << pair.first << " (ID: " << id << ")" << std::endl;
    }
}

void ComponentFactory::PrintFactoryInfo() const {
    std::cout << "\n=== ComponentFactory Info ===" << std::endl;
    std::cout << "Registered Components: " << componentFactories.size() << std::endl;
    std::cout << "Registered Presets: " << presets.size() << std::endl;
    std::cout << "Next Component ID: " << nextId << std::endl;
}

// Private helpers
void ComponentFactory::InitializeBuiltinComponents() {
    // Register Transform with configuration support
    RegisterComponentWithConfig<Transform>("Transform",
        [](const ComponentConfig& config) -> std::unique_ptr<Transform> {
            float x = config.GetFloat("x", 0.0f);
            float y = config.GetFloat("y", 0.0f);
            float z = config.GetFloat("z", 0.0f);

            auto transform = std::make_unique<Transform>(x, y, z);

            // Set rotation if provided
            float rx = config.GetFloat("rotX", 0.0f);
            float ry = config.GetFloat("rotY", 0.0f);
            float rz = config.GetFloat("rotZ", 0.0f);
            if (rx != 0.0f || ry != 0.0f || rz != 0.0f) {
                transform->SetRotation(rx, ry, rz);
            }

            // Set scale if provided
            float scale = config.GetFloat("scale", 1.0f);
            if (scale != 1.0f) {
                transform->SetScale(scale);
            }

            return transform;
        });

    // Register basic Behavior
    RegisterComponent<Behavior>("Behavior");

    // Register some useful presets
    ComponentConfig playerTransformPreset("Transform");
    playerTransformPreset.SetFloat("x", 0.0f).SetFloat("y", 1.0f).SetFloat("z", 0.0f).SetFloat("scale", 1.5f);
    RegisterPreset("PlayerTransform", playerTransformPreset);

    ComponentConfig enemyTransformPreset("Transform");
    enemyTransformPreset.SetFloat("x", 10.0f).SetFloat("y", 0.0f).SetFloat("z", 5.0f).SetFloat("scale", 0.8f);
    RegisterPreset("EnemyTransform", enemyTransformPreset);

    std::cout << "Built-in components and presets registered" << std::endl;
}

size_t ComponentFactory::AssignComponentId(const std::string& typeName) {
    size_t id = nextId++;
    componentIds[typeName] = id;
    idToName[id] = typeName;
    return id;
}