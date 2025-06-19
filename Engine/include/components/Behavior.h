#pragma once

#include "Component.h"
#include "Transform.h"
#include <string>
#include <vector>

// Forward declarations
class GameObject;

class Behavior : public Component {
private:
    bool started = false;
    Transform* cachedTransform = nullptr;

public:
    // Constructor and destructor
    Behavior() = default;
    virtual ~Behavior() = default;

    // Component interface
    void Update(float deltaTime) override final;
    std::string GetDisplayName() const override { return "Behavior Component"; }

    // Behavior lifecycle methods
    virtual void Start() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnLateUpdate(float deltaTime) {}
    virtual void OnFixedUpdate(float fixedDeltaTime) {}

    // Component lifecycle overrides
    void OnEnable() override;
    void OnDisable() override;
    void OnDestroy() override;

    // Common behavior functionality
    Transform* GetTransform();
    GameObject* GetGameObject() { return GetOwner(); }

    // Time utilities
    static float GetTime();
    static float GetDeltaTime();

    // Input utilities
    virtual void OnCollisionEnter(GameObject* other) {}
    virtual void OnCollisionStay(GameObject* other) {}
    virtual void OnCollisionExit(GameObject* other) {}

    // Debug utilities
    void Log(const std::string& message) const;
    void LogWarning(const std::string& message) const;
    void LogError(const std::string& message) const;

protected:
    // ===== DECLARE TEMPLATE METHODS (IMPLEMENT IN .cpp) =====

    template<typename T>
    T* FindObjectOfType() const;

    template<typename T>
    std::vector<T*> FindObjectsOfType() const;

    GameObject* FindGameObjectWithTag(const std::string& tag) const;
    std::vector<GameObject*> FindGameObjectsWithTag(const std::string& tag) const;

    // Component shortcuts - DECLARE ONLY
    template<typename T>
    T* GetComponent();

    template<typename T>
    const T* GetComponent() const;

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args);

    template<typename T>
    T* GetBehavior();

    template<typename T>
    const T* GetBehavior() const;

    template<typename T>
    std::vector<T*> GetBehaviors();

    template<typename T>
    std::vector<const T*> GetBehaviors() const;

    template<typename T>
    bool IsBehaviorOfType() const;

private:
    void CacheTransform();
};

// Concrete behavior classes can stay in the header since they don't use GameObject methods
class TestBehavior : public Behavior {
public:
    void Start() override {
        Log("TestBehavior started!");
    }

    void OnUpdate(float deltaTime) override {
        if (GetTransform()) {
            GetTransform()->Rotate(0.0f, 45.0f * deltaTime, 0.0f);
        }
    }

    std::string GetDisplayName() const override { return "Test Behavior"; }
};

class MovementBehavior : public Behavior {
private:
    Vector3 velocity = Vector3::Zero;
    float speed = 5.0f;

public:
    MovementBehavior(float moveSpeed = 5.0f) : speed(moveSpeed) {}

    void SetVelocity(const Vector3& vel) { velocity = vel; }
    void SetSpeed(float newSpeed) { speed = newSpeed; }
    float GetSpeed() const { return speed; }

    void OnUpdate(float deltaTime) override {
        if (GetTransform()) {
            Vector3 movement = velocity * speed * deltaTime;
            GetTransform()->Translate(movement);
        }
    }

    std::string GetDisplayName() const override { return "Movement Behavior"; }

    // DECLARE ONLY - implement in .cpp
    bool HasConflictingBehaviors() const;
};

class PlayerController : public Behavior {
private:
    float moveSpeed = 10.0f;
    float rotationSpeed = 90.0f;

public:
    PlayerController(float speed = 10.0f, float rotSpeed = 90.0f)
        : moveSpeed(speed), rotationSpeed(rotSpeed) {
    }

    void Start() override;  // Move implementation to .cpp
    void OnUpdate(float deltaTime) override;  // Move implementation to .cpp

    std::string GetDisplayName() const override { return "Player Controller"; }

    // DECLARE ONLY - implement in .cpp
    std::vector<PlayerController*> FindAllPlayers() const;
    bool IsMainPlayer() const;
};