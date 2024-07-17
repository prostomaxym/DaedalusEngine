#pragma once

#include "Daedalus/Renderer/Primitives/FrameBuffer.h"
#include "Daedalus/Renderer/Resourses/Texture.h"

#include <glm/glm.hpp>

#include <random>

namespace Daedalus
{
    class SSAO
    {
    public:
        SSAO(int width, int height, int kernel_size, int noise_width, int noise_height);

    private:
        void CreateFramebuffer(int width, int height);
        void CreateSampleKernel(int kernel_size);
        void CreateNoiseTexture(int noise_width, int noise_height);

        static std::mt19937 CreateRandomGenerator();

        std::mt19937 m_rand_generator;
        std::shared_ptr<Framebuffer> m_buffer = nullptr;
        std::vector<glm::vec3> m_kernel;
        std::shared_ptr<Texture2D> m_noise_texture = nullptr;
    };
}