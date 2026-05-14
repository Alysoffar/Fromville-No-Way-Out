#pragma once

#include <string>

#include "game/entities/BaseEntity.h"

class EnvironmentObject : public BaseEntity {
public:
    explicit EnvironmentObject(std::string entityName = "EnvironmentObject");

    void Update(float dt) override;
};
