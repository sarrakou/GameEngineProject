#pragma once

#include "Component.h"
#include "Transform.h"
#include <string>

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
    const char* GetTypeName() const override { return "Behavior"; }

    // Behavior lifecycle methods - override these in derived classes
    virtual void Start() {}          // Called once when first enabled
    virtual void OnUpdate(float deltaTime) {}  // Called every frame
    virtual void OnLateUpdate(float deltaTime) {}  // Called after all OnUpdate calls
    virtual void OnFixedUpdate(float fixedDeltaTime) {}  // Called at fixed intervals (physics)

    // Component lifecycle overrides
    void OnEnable() override;
    void OnDisable() override;
    void OnDestroy() override;

    // Common behavior functionality
    Transform* GetTransform();  // Cached access to transform component
    GameObject* GetGameObject() { return GetOwner(); }  // Alias for clarity

    // Time utilities
    static float GetTime();      // Total time since engine start
    static float GetDeltaTime(); // Time since last frame

    // Input utilities (basic framework - can be expanded)
    virtual void OnCollisionEnter(GameObject* other) {}
    virtual void OnCollisionStay(GameObject* other) {}
    virtual void OnCollisionExit(GameObject* other) {}

    // Debug utilities
    void Log(const std::string& message) const;
    void LogWarning(const std::string& message) const;
    void LogError(const std::string& message) const;

protected:
    // Protected utilities that derived classes can use
    template<typename T>
    T* FindObjectOfType() const;

    template<typename T>
    std::vector<T*> FindObjectsOfType() const;

    GameObject* FindGameObjectWithTag(const std::string& tag) const;
    std::vector<GameObject*> FindGameObjectsWithTag(const std::string& tag) const;

    // Component shortcuts
    template<typename T>
    T* GetComponent() {
        return GetOwner() ? GetOwner()->GetComponent<T>() : nullptr;
    }

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        return GetOwner() ? GetOwner()->AddComponent<T>(std::forward<Args>(args)...) : nullptr;
    }

private:
    void CacheTransform();
};

// Example concrete behavior classes
class TestBehavior : public Behavior {
public:
    void Start() override {
        Log("TestBehavior started!");
    }

    void OnUpdate(float deltaTime) override {
        // Example: rotate the object
        if (GetTransform()) {
            GetTransform()->Rotate(0.0f, 45.0f * deltaTime, 0.0f);
        }
    }

    const char* GetTypeName() const override { return "TestBehavior"; }
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

    const char* GetTypeName() const override { return "MovementBehavior"; }
};

class PlayerController : public Behavior {
private:
    float moveSpeed = 10.0f;
    float rotationSpeed = 90.0f;

public:
    PlayerController(float speed = 10.0f, float rotSpeed = 90.0f)
        : moveSpeed(speed), rotationSpeed(rotSpeed) {
    }

    void Start() override {
        Log("Player controller initialized");
    }

    void OnUpdate(float deltaTime) override {
        Transform* transform = GetTransform();
        if (!transform) return;

        // Simple movement (this would normally use input system)
        // For demo purposes, just move in a pattern
        static float time = 0.0f;
        time += deltaTime;

        // Move in a circle
        float x = std::cos(time) * 0.1f;
        float z = std::sin(time) * 0.1f;
        transform->Translate(x, 0.0f, z);

        // Rotate
        transform->Rotate(0.0f, rotationSpeed * deltaTime, 0.0f);
    }

    const char* GetTypeName() const override { return "PlayerController"; }
};