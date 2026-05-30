#include "TankFactory.h"
#include "controller/PlayerController.h"
#include "controller/AIController.h"

std::unique_ptr<Tank> TankFactory::CreatePlayerTank(
    TankType type, int team,
    int keyUp, int keyDown, int keyLeft, int keyRight, int keyFire)
{
    auto controller = std::make_unique<PlayerController>(keyUp, keyDown, keyLeft, keyRight, keyFire);
    return std::make_unique<Tank>(type, std::move(controller), team);
}

std::unique_ptr<Tank> TankFactory::CreateAITank(TankType type, int team) {
    auto controller = std::make_unique<AIController>();
    return std::make_unique<Tank>(type, std::move(controller), team);
}
