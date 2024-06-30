#pragma once

#include "Daedalus/ECS/NativeScript.h"

#include <optional>
namespace Daedalus {

    class DAEDALUS_API CameraController : public NativeScript
    {
    public:
        CameraController(Entity entity) : NativeScript(entity) {}

    protected:
        virtual void OnUpdate(DeltaTime dt) override;
        void UpdateSpotlight();

       std::optional<float> xPrevious{ std::nullopt };
       std::optional<float> yPrevious{ std::nullopt };
        bool m_spotlight_enabled{ true };
    };
}
