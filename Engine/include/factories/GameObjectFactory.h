#pragma once

#include "../core/GameObject.h"
#include "ComponentFactory.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

#include <iostream>

// Forward declarations
class Scene;

// GameObject template/blueprint definition
struct GameObjectTemplate {
    std::string name;
    std::string tag;
    std::vector<ComponentConfig> components;
    bool active = true;

    // Default constructor
    GameObjectTemplate() = default;

    GameObjectTemplate(const std::string& templateName, const std::string& objectTag = "")
        : name(templateName), tag(objectTag) {
    }

    // Component addition
    GameObjectTemplate& AddComponent(const ComponentConfig& config) {
        components.push_back(config);
        return *this;
    }

    GameObjectTemplate& AddComponent(const std::string& componentType) {
        components.emplace_back(componentType);
        return *this;
    }

    // Convenience methods for common components
    GameObjectTemplate& AddTransform(float x = 0.0f, float y = 0.0f, float z = 0.0f) {
        ComponentConfig config("Transform");
        config.SetFloat("x", x).SetFloat("y", y).SetFloat("z", z);
        return AddComponent(config);
    }

    GameObjectTemplate& AddBehavior() {
        return AddComponent("Behavior");
    }

    // Template configuration
    GameObjectTemplate& SetActive(bool isActive) {
        active = isActive;
        return *this;
    }

    GameObjectTemplate& SetTag(const std::string& newTag) {
        tag = newTag;
        return *this;
    }

    // Template information
    size_t GetComponentCount() const { return components.size(); }
    bool HasComponent(const std::string& componentType) const {
        for (const auto& config : components) {
            if (config.typeName == componentType) {
                return true;
            }
        }
        return false;
    }
};

// GameObject factory creation result
struct GameObjectCreationResult {
    std::unique_ptr<GameObject> gameObject;
    std::vector<std::string> errors;
    bool success = false;

    GameObjectCreationResult() = default;
    GameObjectCreationResult(std::unique_ptr<GameObject> obj)
        : gameObject(std::move(obj)), success(true) {
    }

    void AddError(const std::string& error) {
        errors.push_back(error);
        success = false;
    }

    bool HasErrors() const { return !errors.empty(); }
    void PrintErrors() const {
        for (const std::string& error : errors) {
            std::cerr << "GameObject creation error: " << error << std::endl;
        }
    }
};

// GameObjectFactory: Data-driven GameObject creation (REQUIREMENT #4)
class GameObjectFactory {
private:
    // Template registry
    std::unordered_map<std::string, GameObjectTemplate> templates;

    // Component factory reference
    ComponentFactory& componentFactory;

    // Factory statistics
    size_t objectsCreated = 0;
    size_t templatesRegistered = 0;

    // Singleton instance
    static GameObjectFactory* instance;

public:
    // Singleton access
    static GameObjectFactory& GetInstance();
    static void DestroyInstance();
    // Constructor and destructor
    GameObjectFactory();
    ~GameObjectFactory() = default;

    // Delete copy operations
    GameObjectFactory(const GameObjectFactory&) = delete;
    GameObjectFactory& operator=(const GameObjectFactory&) = delete;

    // Template registration
    void RegisterTemplate(const GameObjectTemplate& gameObjectTemplate);
    void RegisterTemplate(const std::string& name, const std::string& tag,
        const std::vector<ComponentConfig>& components);

    bool HasTemplate(const std::string& templateName) const;
    const GameObjectTemplate* GetTemplate(const std::string& templateName) const;

    // GameObject creation from templates
    GameObjectCreationResult CreateGameObject(const std::string& templateName);
    GameObjectCreationResult CreateGameObject(const GameObjectTemplate& gameObjectTemplate);

    // Direct GameObject creation with components
    GameObjectCreationResult CreateGameObject(const std::string& tag,
        const std::vector<ComponentConfig>& components);

    // Batch GameObject creation
    std::vector<GameObjectCreationResult> CreateGameObjects(const std::string& templateName, size_t count);
    std::vector<GameObjectCreationResult> CreateGameObjectsFromFile(const std::string& filepath);

    // Specialized creation methods
    std::unique_ptr<GameObject> CreatePlayer(float x = 0.0f, float y = 0.0f, float z = 0.0f);
    std::unique_ptr<GameObject> CreateEnemy(float x = 0.0f, float y = 0.0f, float z = 0.0f);
    std::unique_ptr<GameObject> CreateStaticObject(const std::string& tag, float x, float y, float z);

    // Template management
    void RemoveTemplate(const std::string& templateName);
    void ClearTemplates();
    std::vector<std::string> GetTemplateNames() const;
    size_t GetTemplateCount() const { return templates.size(); }

    // Template serialization
    bool SaveTemplate(const std::string& templateName, const std::string& filepath) const;
    bool LoadTemplate(const std::string& filepath);
    bool LoadTemplatesFromDirectory(const std::string& directory);

    // Data-driven creation from strings
    GameObjectCreationResult CreateFromString(const std::string& objectData);

    // Scene population
    void PopulateScene(Scene* scene, const std::string& templateName, size_t count);
    void PopulateSceneFromFile(Scene* scene, const std::string& filepath);

    // Factory statistics and info
    size_t GetObjectsCreated() const { return objectsCreated; }
    size_t GetTemplatesRegistered() const { return templatesRegistered; }
    void ResetStatistics();

    // Debug and utilities
    void PrintTemplates() const;
    void PrintFactoryInfo() const;
    void PrintTemplate(const std::string& templateName) const;

private:
    // Internal creation helpers
    std::unique_ptr<GameObject> CreateGameObjectInternal(const GameObjectTemplate& gameObjectTemplate,
        GameObjectCreationResult& result);

    void ApplyComponentsToGameObject(GameObject* gameObject,
        const std::vector<ComponentConfig>& components,
        GameObjectCreationResult& result);

    // Built-in template initialization
    void InitializeBuiltinTemplates();

    // File parsing helpers
    GameObjectTemplate ParseTemplateFromString(const std::string& data) const;
    std::vector<GameObjectTemplate> ParseTemplatesFromFile(const std::string& filepath) const;
};

// Template builder helper class
class GameObjectTemplateBuilder {
private:
    GameObjectTemplate gameObjectTemplate;

public:
    GameObjectTemplateBuilder(const std::string& name, const std::string& tag = "")
        : gameObjectTemplate(name, tag) {
    }

    GameObjectTemplateBuilder& WithComponent(const ComponentConfig& config) {
        gameObjectTemplate.AddComponent(config);
        return *this;
    }

    GameObjectTemplateBuilder& WithComponent(const std::string& componentType) {
        gameObjectTemplate.AddComponent(componentType);
        return *this;
    }

    GameObjectTemplateBuilder& WithTransform(float x = 0.0f, float y = 0.0f, float z = 0.0f) {
        gameObjectTemplate.AddTransform(x, y, z);
        return *this;
    }

    GameObjectTemplateBuilder& WithBehavior() {
        gameObjectTemplate.AddBehavior();
        return *this;
    }

    GameObjectTemplateBuilder& WithTag(const std::string& tag) {
        gameObjectTemplate.SetTag(tag);
        return *this;
    }

    GameObjectTemplateBuilder& SetActive(bool active) {
        gameObjectTemplate.SetActive(active);
        return *this;
    }

    GameObjectTemplate Build() {
        return std::move(gameObjectTemplate);
    }

    // Register the template directly
    void Register() {
        GameObjectFactory::GetInstance().RegisterTemplate(gameObjectTemplate);
    }
};

// Convenience macros
#define GAMEOBJECT_FACTORY GameObjectFactory::GetInstance()
#define CREATE_GAMEOBJECT(template) GameObjectFactory::GetInstance().CreateGameObject(template)
#define REGISTER_TEMPLATE(template) GameObjectFactory::GetInstance().RegisterTemplate(template)

// Template builder macro
#define BUILD_TEMPLATE(name, tag) GameObjectTemplateBuilder(name, tag)