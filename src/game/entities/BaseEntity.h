#pragma once

#include <string>

#include "engine/Transform.h"

class BaseEntity {
public:
    explicit BaseEntity(std::string entityName = "Entity");
    virtual ~BaseEntity() = default;

    virtual void Update(float dt) = 0;
    virtual void OnSpawn() {}
    virtual void OnDespawn() {}

    const std::string& GetName() const;

    bool IsActive() const { return active; }
    void SetActive(bool value) { active = value; }

    bool IsVisible() const { return visible; }
    void SetVisible(bool value) { visible = value; }

    Transform transform;

protected:
    std::string name;
    bool active = true;
    bool visible = true;
};
