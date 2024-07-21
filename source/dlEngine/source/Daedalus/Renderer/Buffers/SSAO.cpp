#include "dlpch.h"

#include "SSAO.h"
#include "Daedalus/Config/GraphicsConfig.h"
#include "Daedalus/Renderer/API/RenderConstants.h"
#include "Daedalus/Utils/Random.h"

using namespace Daedalus;

namespace
{
    float Lerp(float a, float b, float f)
    {
        return a + f * (b - a);
    }
}

SSAO::SSAO(int width, int height, int kernel_size, int noise_width, int noise_height)
    : m_noise_width(noise_width)
    , m_noise_height(noise_height)
{
    CreateFramebuffer(width, height);
    CreateSampleKernel(kernel_size);
    CreateNoiseTexture(noise_width, noise_height);
}

void SSAO::CreateUBO(int width, int height)
{
    struct BufferData
    {
        std::array<glm::vec4, 64> ssao_kernel;
        glm::vec2 ssao_noise_scale;
        float ssao_radius;
        int ssao_kernel_size;
        float ssao_bias;
        int align1 = -1;
        int align2 = -1;
        int align3 = -1;
    };

    BufferData data;

    std::copy(m_kernel.begin(), m_kernel.end(), data.ssao_kernel.begin());
    data.ssao_noise_scale = glm::vec2(static_cast<float>(width) / static_cast<float>(m_noise_width),
        height / static_cast<float>(m_noise_width));

    data.ssao_radius = GraphicsConfig::GetSSAORadius();
    data.ssao_kernel_size = GraphicsConfig::GetSSAOKernelSize();
    data.ssao_bias = GraphicsConfig::GetSSAOBias();

    m_ubo = UniformBuffer::Create(sizeof(BufferData), 2, UniformBuffer::Type::Static, &data);
}

void SSAO::CreateFramebuffer(int width, int height)
{
    FramebufferSpecification ssao_spec;
    ssao_spec.width = width;
    ssao_spec.height = height;
    ssao_spec.samples = 1;
    ssao_spec.attachments = FramebufferAttachmentSpecification({ FramebufferTextureSpecification(FramebufferTextureFormat::RED_FLOAT) });
    ssao_spec.layers = -1;
    m_ssao_buffer = Framebuffer::Create(ssao_spec);

    FramebufferSpecification blur_specs;
    blur_specs.width = width;
    blur_specs.height = height;
    blur_specs.samples = 1;
    blur_specs.attachments = FramebufferAttachmentSpecification({ FramebufferTextureSpecification(FramebufferTextureFormat::RED_FLOAT) });
    blur_specs.layers = -1;
    m_blur_buffer = Framebuffer::Create(blur_specs);
}

void SSAO::CreateSampleKernel(int kernel_size)
{

    std::uniform_real_distribution<float> random_floats(0.f, 1.f);
    m_kernel.reserve((kernel_size));

    for (unsigned int i = 0; i < kernel_size; ++i)
    {
        glm::vec4 sample(Random::UniformRealDistribution(0.f, 1.f) * 2.f - 1.f, Random::UniformRealDistribution(0.f, 1.f) * 2.f - 1.f, Random::UniformRealDistribution(0.f, 1.f), 1.f);
        sample = glm::normalize(sample);
        sample *= Random::UniformRealDistribution(0.f, 1.f);
        float scale = static_cast<float>(i) / static_cast<float>(kernel_size);

        // scale samples s.t. they're more aligned to center of kernel
        scale = Lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        m_kernel.push_back(sample);
    }
}

void SSAO::CreateNoiseTexture(int noise_width, int noise_height)
{
    std::vector<glm::vec3> ssao_noise;
    ssao_noise.reserve(noise_width * noise_height);

    for (unsigned int i = 0; i < noise_width * noise_height; i++)
    {
        glm::vec3 noise(Random::UniformRealDistribution(0.f, 1.f) * 2.0 - 1.0, Random::UniformRealDistribution(0.f, 1.f) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssao_noise.push_back(glm::normalize(noise));
    }

    m_noise_texture = Texture2D::Create(&ssao_noise[0].x, noise_width, noise_height, 3);
}