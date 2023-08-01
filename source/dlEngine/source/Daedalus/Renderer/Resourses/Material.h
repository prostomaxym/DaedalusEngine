#pragma once

#include "Texture.h"

#include <glm/glm.hpp>

namespace Daedalus
{
	class Material
	{
	public:
		Material(const std::string& material_name = { "Default Material" }, glm::vec3 kambient = { 1.f, 1.f, 0.1f }, glm::vec3 kdiffuse = { 1.f, 1.f, 1.f }, 
			glm::vec3 kspecular = { 1.f, 1.f, 1.f }, float shininess = { 16.f });

		void SetDiffuseMap(unsigned char* data, int width, int height, int channels);
		void SetSpecularMap(unsigned char* data, int width, int height, int channels);
		void SetNormalMap(unsigned char* data, int width, int height, int channels);

		std::shared_ptr<Texture2D> GetDiffuseMap() const { return m_map_diffuse; }
		std::shared_ptr<Texture2D> GetSpecularMap() const { return m_map_specular; }
		std::shared_ptr<Texture2D> GetNormalMap() const { return m_map_normal; }

		glm::vec3 GetAmbientK() const { return m_k_ambient; }
		glm::vec3 GetDiffuseK() const { return m_k_diffuse; }
		glm::vec3 GetSpecularK() const { return m_k_specular; }
		float GetShininess() const { return m_n_shininess; }

	private:
		std::string m_material_name{ "Default Material" };
		glm::vec3 m_k_ambient{ 1.f, 1.f, 1.f };
		glm::vec3 m_k_diffuse{ 1.f, 1.f, 1.f };
		glm::vec3 m_k_specular{ 1.f, 1.f, 1.f };
		float m_n_shininess{ 16.f };

		std::shared_ptr<Texture2D> m_map_diffuse{ nullptr };
		std::shared_ptr<Texture2D> m_map_specular{ nullptr };
		std::shared_ptr<Texture2D> m_map_normal{ nullptr };
	};
}