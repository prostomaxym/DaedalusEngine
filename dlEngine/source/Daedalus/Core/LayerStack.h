#pragma once

#include "Core.h"
#include "Layer.h"

#include <vector>

namespace Daedalus {

class DAEDALUS_API LayerStack
{
public:
	LayerStack();
	~LayerStack();

	// @brief Push layer and TRANSFER OWNERSHIP to layer stack
	void PushLayer(Layer* layer);

	// @brief Push layer and TRANSFER OWNERSHIP to layer stack
	void PushOverlay(Layer* overlay);

	// @brief Pop layer and RELEASE OWNERSHIP from layer stack
	void PopLayer(Layer* layer);

	// @brief Pop layer and RELEASE OWNERSHIP from layer stack
	void PopOverlay(Layer* overlay);

	std::vector<Layer*>::iterator begin() { return m_layers.begin(); }
	std::vector<Layer*>::iterator end() { return m_layers.end(); }

private:
	std::vector<Layer*> m_layers;
	std::vector<Layer*>::iterator m_layer_insert;
};

}