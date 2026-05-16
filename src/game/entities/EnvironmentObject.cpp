#include "game/entities/EnvironmentObject.h"

EnvironmentObject::EnvironmentObject(std::string entityName)
    : BaseEntity(std::move(entityName)) {
}

void EnvironmentObject::Update(float dt) {
    (void)dt;
}
