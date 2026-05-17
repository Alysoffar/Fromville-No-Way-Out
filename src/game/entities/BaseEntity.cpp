#include "game/entities/BaseEntity.h"

#include <utility>

BaseEntity::BaseEntity(std::string entityName)
    : name(std::move(entityName)) {
}

const std::string& BaseEntity::GetName() const {
    return name;
}
