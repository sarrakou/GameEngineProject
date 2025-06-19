#include "../include/factories/GameObjectFactory.h"
#include "../include/core/Scene.h"
#include "../include/components/Transform.h"
#include "../include/components/Behavior.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Static instance initialization
GameObjectFactory* GameObjectFactory::instance = nullptr;

GameObjectFactory& GameObjectFactory::GetInstance() {
    if (instance == nullptr) {
        instance = new GameObjectFactory();
    }
    return *instance;
}

void GameObjectFactory::DestroyInstance() {
    delete instance;
    instance = nullptr;
}

GameObjectFactory::GameObjectFactory() : componentFactory(ComponentFactory::GetInstance()) {
    InitializeBuiltinTemplates();
    std::cout << "GameObjectFactory initialized" << std::endl;
}

// Template registration
void GameObjectFactory::RegisterTemplate(const GameObjectTemplate& gameObjectTemplate) {
    templates[gameObjectTemplate.name] = gameObjectTemplate;
    templatesRegistered++;
    std::cout << "Registered GameObject template: " << gameObjectTemplate.name << std::endl;
}

void GameObjectFactory::RegisterTemplate(const std::string& name, const std::string& tag,
    const std::vector<ComponentConfig>& components) {
    GameObjectTemplate gameObjectTemplate(name, tag);
    for (const auto& config : components) {
        gameObjectTemplate.AddComponent(config);
    }
    RegisterTemplate(gameObjectTemplate);
}

bool GameObjectFactory::HasTemplate(const std::string& templateName) const {
    return templates.find(templateName) != templates.end();
}

const GameObjectTemplate* GameObjectFactory::GetTemplate(const std::string& templateName) const {
    auto it = templates.find(templateName);
    if (it != templates.end()) {
        return &it->second;
    }
    return nullptr;
}

// GameObject creation from templates
GameObjectCreationResult GameObjectFactory::CreateGameObject(const std::string& templateName) {
    auto it = templates.find(templateName);
    if (it == templates.end()) {
        GameObjectCreationResult result;
        result.AddError("Template not found: " + templateName);
        return result;
    }

    return CreateGameObject(it->second);
}

GameObjectCreationResult GameObjectFactory::CreateGameObject(const GameObjectTemplate& gameObjectTemplate) {
    GameObjectCreationResult result;

    auto gameObject = CreateGameObjectInternal(gameObjectTemplate, result);
    if (gameObject) {
        result.gameObject = std::move(gameObject);
        result.success = true;
        objectsCreated++;
    }

    return result;
}

// Direct GameObject creation with components
GameObjectCreationResult GameObjectFactory::CreateGameObject(const std::string& tag,
    const std::vector<ComponentConfig>& components) {
    GameObjectTemplate temp("Temporary", tag);
    for (const auto& config : components) {
        temp.AddComponent(config);
    }

    return CreateGameObject(temp);
}

// Batch GameObject creation
std::vector<GameObjectCreationResult> GameObjectFactory::CreateGameObjects(const std::string& templateName, size_t count) {
    std::vector<GameObjectCreationResult> results;
    results.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        results.push_back(CreateGameObject(templateName));
    }

    return results;
}

std::vector<GameObjectCreationResult> GameObjectFactory::CreateGameObjectsFromFile(const std::string& filepath) {
    std::vector<GameObjectCreationResult> results;

    auto fileTemplates = ParseTemplatesFromFile(filepath);
    for (const auto& gameObjectTemplate : fileTemplates) {
        results.push_back(CreateGameObject(gameObjectTemplate));
    }

    std::cout << "Created " << results.size() << " GameObjects from " << filepath << std::endl;
    return results;
}

// Specialized creation methods
std::unique_ptr<GameObject> GameObjectFactory::CreatePlayer(float x, float y, float z) {
    auto result = CreateGameObject("Player");
    if (result.success && result.gameObject) {
        // Set custom position
        if (auto* transform = result.gameObject->GetComponent<Transform>()) {
            transform->SetPosition(x, y, z);
        }
        return std::move(result.gameObject);
    }

    // Fallback: create basic player
    auto gameObject = std::make_unique<GameObject>("Player");
    gameObject->AddComponent<Transform>(x, y, z);
    return gameObject;
}

std::unique_ptr<GameObject> GameObjectFactory::CreateEnemy(float x, float y, float z) {
    auto result = CreateGameObject("Enemy");
    if (result.success && result.gameObject) {
        // Set custom position
        if (auto* transform = result.gameObject->GetComponent<Transform>()) {
            transform->SetPosition(x, y, z);
        }
        return std::move(result.gameObject);
    }

    // Fallback: create basic enemy
    auto gameObject = std::make_unique<GameObject>("Enemy");
    gameObject->AddComponent<Transform>(x, y, z);
    return gameObject;
}

std::unique_ptr<GameObject> GameObjectFactory::CreateStaticObject(const std::string& tag, float x, float y, float z) {
    auto gameObject = std::make_unique<GameObject>(tag);
    gameObject->AddComponent<Transform>(x, y, z);
    objectsCreated++;
    return gameObject;
}

// Template management
void GameObjectFactory::RemoveTemplate(const std::string& templateName) {
    auto it = templates.find(templateName);
    if (it != templates.end()) {
        templates.erase(it);
        std::cout << "Removed template: " << templateName << std::endl;
    }
}

void GameObjectFactory::ClearTemplates() {
    size_t count = templates.size();
    templates.clear();
    std::cout << "Cleared " << count << " templates" << std::endl;
}

std::vector<std::string> GameObjectFactory::GetTemplateNames() const {
    std::vector<std::string> names;
    names.reserve(templates.size());

    for (const auto& pair : templates) {
        names.push_back(pair.first);
    }

    return names;
}

// Template serialization
bool GameObjectFactory::SaveTemplate(const std::string& templateName, const std::string& filepath) const {
    auto it = templates.find(templateName);
    if (it == templates.end()) {
        std::cerr << "Template not found: " << templateName << std::endl;
        return false;
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filepath << std::endl;
        return false;
    }

    const GameObjectTemplate& temp = it->second;
    file << "# GameObject Template: " << temp.name << std::endl;
    file << "Name:" << temp.name << std::endl;
    file << "Tag:" << temp.tag << std::endl;
    file << "Active:" << (temp.active ? "true" : "false") << std::endl;
    file << "Components:" << std::endl;

    for (const auto& config : temp.components) {
        file << "  - Type:" << config.typeName << std::endl;
        for (const auto& prop : config.properties) {
            file << "    " << prop.first << ":" << prop.second << std::endl;
        }
    }

    file.close();
    std::cout << "Saved template " << templateName << " to " << filepath << std::endl;
    return true;
}

bool GameObjectFactory::LoadTemplate(const std::string& filepath) {
    // Simple template loading implementation
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open template file: " << filepath << std::endl;
        return false;
    }

    // Basic parsing (could be improved with JSON/XML parsing)
    std::string line;
    std::string name, tag;
    bool active = true;
    std::vector<ComponentConfig> components;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line.find("Name:") == 0) {
            name = line.substr(5);
        }
        else if (line.find("Tag:") == 0) {
            tag = line.substr(4);
        }
        else if (line.find("Active:") == 0) {
            active = line.substr(7) == "true";
        }
        // Component parsing would go here...
    }

    if (!name.empty()) {
        GameObjectTemplate temp(name, tag);
        temp.SetActive(active);
        for (const auto& config : components) {
            temp.AddComponent(config);
        }
        RegisterTemplate(temp);
        return true;
    }

    return false;
}

bool GameObjectFactory::LoadTemplatesFromDirectory(const std::string& directory) {
    // Directory traversal would require platform-specific code
    // For now, just load a few common template files
    std::vector<std::string> templateFiles = {
        directory + "/player.template",
        directory + "/enemy.template",
        directory + "/objects.template"
    };

    bool anyLoaded = false;
    for (const std::string& file : templateFiles) {
        if (LoadTemplate(file)) {
            anyLoaded = true;
        }
    }

    return anyLoaded;
}

// Data-driven creation from strings
GameObjectCreationResult GameObjectFactory::CreateFromString(const std::string& objectData) {
    GameObjectTemplate temp = ParseTemplateFromString(objectData);
    return CreateGameObject(temp);
}

// Scene population
void GameObjectFactory::PopulateScene(Scene* scene, const std::string& templateName, size_t count) {
    if (!scene) return;

    auto results = CreateGameObjects(templateName, count);
    for (auto& result : results) {
        if (result.success && result.gameObject) {
            scene->AddGameObject(std::move(result.gameObject));
        }
        else {
            result.PrintErrors();
        }
    }

    std::cout << "Populated scene with " << count << " objects of type " << templateName << std::endl;
}

void GameObjectFactory::PopulateSceneFromFile(Scene* scene, const std::string& filepath) {
    if (!scene) return;

    auto results = CreateGameObjectsFromFile(filepath);
    for (auto& result : results) {
        if (result.success && result.gameObject) {
            scene->AddGameObject(std::move(result.gameObject));
        }
        else {
            result.PrintErrors();
        }
    }
}

// Factory statistics
void GameObjectFactory::ResetStatistics() {
    objectsCreated = 0;
    templatesRegistered = 0;
}

// Debug and utilities
void GameObjectFactory::PrintTemplates() const {
    std::cout << "\n=== Registered GameObject Templates ===" << std::endl;

    for (const auto& pair : templates) {
        const GameObjectTemplate& temp = pair.second;
        std::cout << "- " << temp.name << " (Tag: '" << temp.tag
            << "', Components: " << temp.components.size() << ")" << std::endl;
    }
}

void GameObjectFactory::PrintFactoryInfo() const {
    std::cout << "\n=== GameObjectFactory Info ===" << std::endl;
    std::cout << "Registered Templates: " << templates.size() << std::endl;
    std::cout << "Objects Created: " << objectsCreated << std::endl;
    std::cout << "Templates Registered: " << templatesRegistered << std::endl;
}

void GameObjectFactory::PrintTemplate(const std::string& templateName) const {
    auto it = templates.find(templateName);
    if (it == templates.end()) {
        std::cout << "Template not found: " << templateName << std::endl;
        return;
    }

    const GameObjectTemplate& temp = it->second;
    std::cout << "\n=== Template: " << temp.name << " ===" << std::endl;
    std::cout << "Tag: " << temp.tag << std::endl;
    std::cout << "Active: " << (temp.active ? "true" : "false") << std::endl;
    std::cout << "Components (" << temp.components.size() << "):" << std::endl;

    for (const auto& config : temp.components) {
        std::cout << "  - " << config.typeName << std::endl;
        for (const auto& prop : config.properties) {
            std::cout << "    " << prop.first << ": " << prop.second << std::endl;
        }
    }
}

// Private helpers
std::unique_ptr<GameObject> GameObjectFactory::CreateGameObjectInternal(const GameObjectTemplate& gameObjectTemplate,
    GameObjectCreationResult& result) {
    auto gameObject = std::make_unique<GameObject>(gameObjectTemplate.tag);
    gameObject->SetActive(gameObjectTemplate.active);

    ApplyComponentsToGameObject(gameObject.get(), gameObjectTemplate.components, result);

    return gameObject;
}

void GameObjectFactory::ApplyComponentsToGameObject(GameObject* gameObject,
    const std::vector<ComponentConfig>& components,
    GameObjectCreationResult& result) {
    for (const auto& config : components) {
        auto component = componentFactory.CreateComponent(config.typeName, config);
        if (component) {
            // Add component to GameObject
            // Note: This would require GameObject to have a method to add existing components
            // For now, we'll use the template-based AddComponent method

            if (config.typeName == "Transform") {
                auto* transform = gameObject->AddComponent<Transform>(
                    config.GetFloat("x", 0.0f),
                    config.GetFloat("y", 0.0f),
                    config.GetFloat("z", 0.0f)
                );

                // Apply additional properties
                float rx = config.GetFloat("rotX", 0.0f);
                float ry = config.GetFloat("rotY", 0.0f);
                float rz = config.GetFloat("rotZ", 0.0f);
                if (rx != 0.0f || ry != 0.0f || rz != 0.0f) {
                    transform->SetRotation(rx, ry, rz);
                }

                float scale = config.GetFloat("scale", 1.0f);
                if (scale != 1.0f) {
                    transform->SetScale(scale);
                }
            }
            else if (config.typeName == "Behavior") {
                gameObject->AddComponent<Behavior>();
            }
            // Add more component types as needed
        }
        else {
            result.AddError("Failed to create component: " + config.typeName);
        }
    }
}

void GameObjectFactory::InitializeBuiltinTemplates() {
    // Player template
    auto playerTemplate = BUILD_TEMPLATE("Player", "Player")
        .WithTransform(0.0f, 1.0f, 0.0f)
        .WithBehavior()
        .Build();
    RegisterTemplate(playerTemplate);

    // Enemy template
    auto enemyTemplate = BUILD_TEMPLATE("Enemy", "Enemy")
        .WithTransform(10.0f, 0.0f, 5.0f)
        .WithBehavior()
        .Build();
    RegisterTemplate(enemyTemplate);

    // Static object template
    auto staticTemplate = BUILD_TEMPLATE("StaticObject", "Static")
        .WithTransform(0.0f, 0.0f, 0.0f)
        .Build();
    RegisterTemplate(staticTemplate);

    std::cout << "Built-in GameObject templates registered" << std::endl;
}

GameObjectTemplate GameObjectFactory::ParseTemplateFromString(const std::string& data) const {
    // Simple parsing: "TemplateName:Tag:Component1,Component2"
    std::stringstream ss(data);
    std::string name, tag, componentsStr;

    std::getline(ss, name, ':');
    std::getline(ss, tag, ':');
    std::getline(ss, componentsStr);

    GameObjectTemplate temp(name, tag);

    if (!componentsStr.empty()) {
        std::stringstream compStream(componentsStr);
        std::string component;

        while (std::getline(compStream, component, ',')) {
            temp.AddComponent(component);
        }
    }

    return temp;
}

std::vector<GameObjectTemplate> GameObjectFactory::ParseTemplatesFromFile(const std::string& filepath) const {
    std::vector<GameObjectTemplate> gameObjectTemplates;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return gameObjectTemplates;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#') {
            gameObjectTemplates.push_back(ParseTemplateFromString(line));
        }
    }

    file.close();
    return gameObjectTemplates;
}