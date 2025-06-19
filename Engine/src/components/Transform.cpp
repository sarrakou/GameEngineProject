#include "../include/components/Transform.h"
#include <iostream>
#include <algorithm>
#include <cmath>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Static Vector3 constants
const Vector3 Vector3::Zero(0.0f, 0.0f, 0.0f);
const Vector3 Vector3::One(1.0f, 1.0f, 1.0f);
const Vector3 Vector3::Up(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::Right(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::Forward(0.0f, 0.0f, 1.0f);

// Transform constructors
Transform::Transform()
    : position(Vector3::Zero), rotation(Vector3::Zero), scale(Vector3::One) {
}

Transform::Transform(float x, float y, float z)
    : position(x, y, z), rotation(Vector3::Zero), scale(Vector3::One) {
}

Transform::Transform(const Vector3& pos, const Vector3& rot, const Vector3& scl)
    : position(pos), rotation(rot), scale(scl) {
}

Transform::~Transform() {
    // Remove from parent
    if (parent) {
        parent->RemoveChild(this);
    }

    // Clear children (they become orphaned, not deleted)
    for (Transform* child : children) {
        child->parent = nullptr;
    }
    children.clear();
}

void Transform::Update(float deltaTime) {
    // Transform updates are typically handled by systems
    // This could be used for animations or physics integration
}

// Position methods
void Transform::SetPosition(const Vector3& pos) {
    position = pos;
    MarkWorldTransformDirty();
}

void Transform::SetPosition(float x, float y, float z) {
    position = Vector3(x, y, z);
    MarkWorldTransformDirty();
}

void Transform::Translate(const Vector3& translation) {
    position += translation;
    MarkWorldTransformDirty();
}

void Transform::Translate(float x, float y, float z) {
    position += Vector3(x, y, z);
    MarkWorldTransformDirty();
}

// Rotation methods
void Transform::SetRotation(const Vector3& rot) {
    rotation = rot;
    MarkWorldTransformDirty();
}

void Transform::SetRotation(float x, float y, float z) {
    rotation = Vector3(x, y, z);
    MarkWorldTransformDirty();
}

void Transform::Rotate(const Vector3& rot) {
    rotation += rot;
    MarkWorldTransformDirty();
}

void Transform::Rotate(float x, float y, float z) {
    rotation += Vector3(x, y, z);
    MarkWorldTransformDirty();
}

// Scale methods
void Transform::SetScale(const Vector3& scl) {
    scale = scl;
    MarkWorldTransformDirty();
}

void Transform::SetScale(float x, float y, float z) {
    scale = Vector3(x, y, z);
    MarkWorldTransformDirty();
}

void Transform::SetScale(float uniformScale) {
    scale = Vector3(uniformScale, uniformScale, uniformScale);
    MarkWorldTransformDirty();
}

// World space transforms
Vector3 Transform::GetWorldPosition() const {
    UpdateWorldTransform();
    return worldPosition;
}

Vector3 Transform::GetWorldRotation() const {
    UpdateWorldTransform();
    return worldRotation;
}

Vector3 Transform::GetWorldScale() const {
    UpdateWorldTransform();
    return worldScale;
}

// Direction vectors
Vector3 Transform::GetForward() const {
    return EulerToDirection(rotation);
}

Vector3 Transform::GetRight() const {
    Vector3 forward = GetForward();
    return forward.Cross(Vector3::Up).Normalized();
}

Vector3 Transform::GetUp() const {
    Vector3 forward = GetForward();
    Vector3 right = GetRight();
    return right.Cross(forward).Normalized();
}

// Hierarchy management
void Transform::SetParent(Transform* newParent) {
    if (parent == newParent) return;

    // Remove from current parent
    if (parent) {
        parent->RemoveChild(this);
    }

    // Set new parent
    parent = newParent;
    if (parent) {
        parent->AddChild(this);
    }

    MarkWorldTransformDirty();
}

// Utility functions
float Transform::DistanceTo(const Transform* other) const {
    if (!other) return 0.0f;
    Vector3 diff = GetWorldPosition() - other->GetWorldPosition();
    return diff.Magnitude();
}

Vector3 Transform::DirectionTo(const Transform* other) const {
    if (!other) return Vector3::Zero;
    Vector3 diff = other->GetWorldPosition() - GetWorldPosition();
    return diff.Normalized();
}

void Transform::PrintTransform() const {
    std::cout << "Transform:\n";
    std::cout << "  Position: (" << position.x << ", " << position.y << ", " << position.z << ")\n";
    std::cout << "  Rotation: (" << rotation.x << ", " << rotation.y << ", " << rotation.z << ")\n";
    std::cout << "  Scale: (" << scale.x << ", " << scale.y << ", " << scale.z << ")\n";
    std::cout << "  Children: " << children.size() << "\n";
}

// Private methods
void Transform::MarkWorldTransformDirty() {
    worldTransformDirty = true;

    // Mark all children as dirty too
    for (Transform* child : children) {
        child->MarkWorldTransformDirty();
    }
}

void Transform::UpdateWorldTransform() const {
    if (!worldTransformDirty) return;

    if (parent) {
        parent->UpdateWorldTransform();

        // Apply parent transformations
        worldPosition = parent->worldPosition + position;
        worldRotation = parent->worldRotation + rotation;
        worldScale = Vector3(
            parent->worldScale.x * scale.x,
            parent->worldScale.y * scale.y,
            parent->worldScale.z * scale.z
        );
    }
    else {
        // No parent, world transform = local transform
        worldPosition = position;
        worldRotation = rotation;
        worldScale = scale;
    }

    worldTransformDirty = false;
}

void Transform::AddChild(Transform* child) {
    if (child && std::find(children.begin(), children.end(), child) == children.end()) {
        children.push_back(child);
    }
}

void Transform::RemoveChild(Transform* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        children.erase(it);
    }
}

Vector3 Transform::EulerToDirection(const Vector3& euler) const {
    // Convert Euler angles (degrees) to direction vector
    float yawRad = euler.y * (M_PI / 180.0f);
    float pitchRad = euler.x * (M_PI / 180.0f);

    return Vector3(
        std::cos(pitchRad) * std::sin(yawRad),
        std::sin(pitchRad),
        std::cos(pitchRad) * std::cos(yawRad)
    ).Normalized();
}