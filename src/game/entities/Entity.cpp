#include "game/entities/Entity.h"

#include <utility>

Entity::Entity(std::string entityName)
    : name(std::move(entityName)) {
}

const std::string& Entity::GetName() const {
    return name;
}