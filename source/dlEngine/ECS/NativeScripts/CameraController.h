#pragma once

#include "ECS/NativeScript.h"

#include <optional>
namespace Daedalus {

    class DAEDALUS_API CameraController : public NativeScript
    {
    public:
        CameraController(Entity entity) : NativeScript(entity) {}

    protected:
        virtual void OnUpdate(DeltaTime dt) override;

       std::optional<float> xPrevious{ std::nullopt };
       std::optional<float> yPrevious{ std::nullopt };

    private:
        void ProcessMovement(float dt);
        void ProcessMouse(float dt);
        void ProcessGamepad(float dt);
    };
}
