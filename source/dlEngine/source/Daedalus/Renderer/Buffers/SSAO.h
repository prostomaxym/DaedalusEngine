#pragma once

#include "Daedalus/Renderer/Primitives/FrameBuffer.h"
#include "Daedalus/Renderer/Primitives/UniformBuffer.h"
#include "Daedalus/Renderer/Resourses/Texture.h"

#include <glm/glm.hpp>

#include <random>

namespace Daedalus
{
    class SSAO
    {
    public:
        SSAO(int width, int height, int kernel_size, int noise_width, int noise_height);

        void CreateUBO(int width, int height);

        std::shared_ptr<Framebuffer> GetSSAOFramebuffer() const { return m_ssao_buffer; }
        std::shared_ptr<Framebuffer> GetBlurFramebuffer() const { return m_blur_buffer; }
        std::shared_ptr<Texture2D> GetNoiseTexture() const { return m_noise_texture; }
        const std::vector<glm::vec4>& GetKernelArray() const { return m_kernel; }

    private:
        void CreateFramebuffer(int width, int height);
        void CreateSampleKernel(int kernel_size);
        void CreateNoiseTexture(int noise_width, int noise_height);

        std::shared_ptr<Framebuffer> m_ssao_buffer = nullptr;
        std::shared_ptr<Framebuffer> m_blur_buffer = nullptr;
        std::vector<glm::vec4> m_kernel;
        std::shared_ptr<Texture2D> m_noise_texture = nullptr;
        std::unique_ptr<UniformBuffer> m_ubo = nullptr;
        int m_noise_width = 4;
        int m_noise_height = 4;
    };
}
