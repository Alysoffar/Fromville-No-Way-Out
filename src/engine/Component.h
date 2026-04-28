#pragma once

class Entity;

class Component {
public:
    virtual ~Component() = default;

    void SetOwner(Entity* newOwner) { owner = newOwner; }
    Entity* GetOwner() const { return owner; }

    virtual void Update(float dt) { (void)dt; }

protected:
    Entity* owner = nullptr;
};