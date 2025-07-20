#pragma once

#include "Macros.h"
#include "Layer.h"

#include <vector>

namespace Daedalus {

	class DAEDALUS_API LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		LayerStack(const LayerStack&) = delete;
		LayerStack(LayerStack&&) = delete;
		LayerStack& operator=(const LayerStack&) = delete;
		LayerStack& operator=(LayerStack&&) = delete;

		void PushLayer(std::unique_ptr<Layer>&& layer);
		void PushOverlay(std::unique_ptr<Layer>&& overlay);
		std::unique_ptr<Layer> PopLayer(Layer* layer);
		std::unique_ptr<Layer> PopOverlay(Layer* overlay);

		std::vector<std::unique_ptr<Layer>>::iterator begin() { return m_layers.begin(); }
		std::vector<std::unique_ptr<Layer>>::iterator end() { return m_layers.end(); }
		std::vector<std::unique_ptr<Layer>>::reverse_iterator rbegin() { return m_layers.rbegin(); }
		std::vector<std::unique_ptr<Layer>>::reverse_iterator rend() { return m_layers.rend(); }

		std::vector<std::unique_ptr<Layer>>::const_iterator begin() const { return m_layers.begin(); }
		std::vector<std::unique_ptr<Layer>>::const_iterator end()	const { return m_layers.end(); }
		std::vector<std::unique_ptr<Layer>>::const_reverse_iterator rbegin() const { return m_layers.rbegin(); }
		std::vector<std::unique_ptr<Layer>>::const_reverse_iterator rend() const { return m_layers.rend(); }

	private:
		std::vector<std::unique_ptr<Layer>> m_layers;
		unsigned int m_layer_insert_index = 0;
	};
}