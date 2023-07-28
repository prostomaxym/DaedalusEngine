#pragma once

#include "Daedalus/ECS/NativeScript.h"

#include <optional>
namespace Daedalus {

    class DAEDALUS_API CameraController : public NativeScript
    {
    public:
        CameraController(Entity entity) : NativeScript(entity) {}

    protected:

        virtual void OnUpdate(DeltaTime ts) override;

       std::optional<float> xPrevious{ std::nullopt };
       std::optional<float> yPrevious{ std::nullopt };
    };
}
