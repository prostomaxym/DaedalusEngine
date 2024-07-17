#include "dlpch.h"

#include "SSAO.h"

#include <random>

using namespace Daedalus;

namespace
{
    float Lerp(float a, float b, float f)
    {
        return a + f * (b - a);
    }
}

SSAO::SSAO(int width, int height, int kernel_size, int noise_width, int noise_height)
    : m_rand_generator(CreateRandomGenerator())
{
    CreateRandomGenerator();
    CreateFramebuffer(width, height);
    CreateSampleKernel(kernel_size);
    CreateNoiseTexture(noise_width, noise_height);
}

std::mt19937 SSAO::CreateRandomGenerator()
{
    const unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    const std::mt19937 generator(seed);

    return generator;
}

void SSAO::CreateFramebuffer(int width, int height)
{
    FramebufferSpecification buffer_specs;
    buffer_specs.width = width;
    buffer_specs.height = height;
    buffer_specs.samples = 1;
    buffer_specs.attachments = FramebufferAttachmentSpecification({
        FramebufferTextureSpecification(FramebufferTextureFormat::RED_FLOAT),
        FramebufferTextureSpecification(FramebufferTextureFormat::RED_FLOAT)});
    buffer_specs.layers = -1;
    m_buffer = Framebuffer::Create(buffer_specs);
}

void SSAO::CreateSampleKernel(int kernel_size)
{
    std::uniform_real_distribution<float> random_floats(0.f, 1.f);
    m_kernel.reserve((kernel_size));

    for (unsigned int i = 0; i < kernel_size; ++i)
    {
        glm::vec3 sample(random_floats(m_rand_generator) * 2.f - 1.f, random_floats(m_rand_generator) * 2.f - 1.f, random_floats(m_rand_generator));
        sample = glm::normalize(sample);
        sample *= random_floats(m_rand_generator);
        float scale = static_cast<float>(i) / static_cast<float>(kernel_size);

        // scale samples s.t. they're more aligned to center of kernel
        scale = Lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        m_kernel.push_back(sample);
    }
}

void SSAO::CreateNoiseTexture(int noise_width, int noise_height)
{
    std::uniform_real_distribution<float> random_floats(0.f, 1.f);
    std::vector<glm::vec3> ssao_noise;
    ssao_noise.reserve(noise_width * noise_height);

    for (unsigned int i = 0; i < noise_width * noise_height; i++)
    {
        glm::vec3 noise(random_floats(m_rand_generator) * 2.0 - 1.0, random_floats(m_rand_generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssao_noise.push_back(noise);
    }

    m_noise_texture = Texture2D::Create(&ssao_noise[0].x, noise_width, noise_height, 3);
}