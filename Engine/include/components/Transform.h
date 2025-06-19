#pragma once

#include "Component.h"
#include <cmath>
#include <vector>

// Simple 3D vector struct for Transform
struct Vector3 {
    float x, y, z;

    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Vector operations
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    Vector3& operator+=(const Vector3& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& other) {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }

    Vector3& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }

    // Utility functions
    float Magnitude() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3 Normalized() const {
        float mag = Magnitude();
        if (mag > 0.0f) {
            return Vector3(x / mag, y / mag, z / mag);
        }
        return Vector3(0, 0, 0);
    }

    float Dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vector3 Cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    // Static constants
    static const Vector3 Zero;
    static const Vector3 One;
    static const Vector3 Up;
    static const Vector3 Right;
    static const Vector3 Forward;
};

class Transform : public Component {
private:
    Vector3 position;
    Vector3 rotation; // Euler angles in degrees
    Vector3 scale;

    // Cached world transform data
    mutable bool worldTransformDirty = true;
    mutable Vector3 worldPosition;
    mutable Vector3 worldRotation;
    mutable Vector3 worldScale;

    // Parent-child hierarchy
    Transform* parent = nullptr;
    std::vector<Transform*> children;

public:
    // Constructors
    Transform();
    Transform(float x, float y, float z);
    Transform(const Vector3& pos, const Vector3& rot = Vector3::Zero, const Vector3& scl = Vector3::One);

    // Destructor
    ~Transform();

    // Component interface
    void Update(float deltaTime) override;
    const char* GetTypeName() const override { return "Transform"; }

    // Position
    const Vector3& GetPosition() const { return position; }
    void SetPosition(const Vector3& pos);
    void SetPosition(float x, float y, float z);
    void Translate(const Vector3& translation);
    void Translate(float x, float y, float z);

    // Rotation (Euler angles in degrees)
    const Vector3& GetRotation() const { return rotation; }
    void SetRotation(const Vector3& rot);
    void SetRotation(float x, float y, float z);
    void Rotate(const Vector3& rotation);
    void Rotate(float x, float y, float z);

    // Scale
    const Vector3& GetScale() const { return scale; }
    void SetScale(const Vector3& scl);
    void SetScale(float x, float y, float z);
    void SetScale(float uniformScale);

    // World space transforms (considering parent hierarchy)
    Vector3 GetWorldPosition() const;
    Vector3 GetWorldRotation() const;
    Vector3 GetWorldScale() const;

    // Direction vectors (based on rotation)
    Vector3 GetForward() const;
    Vector3 GetRight() const;
    Vector3 GetUp() const;

    // Hierarchy management
    void SetParent(Transform* newParent);
    Transform* GetParent() const { return parent; }
    const std::vector<Transform*>& GetChildren() const { return children; }

    // Utility functions
    float DistanceTo(const Transform* other) const;
    Vector3 DirectionTo(const Transform* other) const;

    // Debug
    void PrintTransform() const;

private:
    void MarkWorldTransformDirty();
    void UpdateWorldTransform() const;
    void AddChild(Transform* child);
    void RemoveChild(Transform* child);

    // Helper functions for rotation calculations
    Vector3 EulerToDirection(const Vector3& euler) const;
};