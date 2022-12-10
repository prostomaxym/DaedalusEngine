#pragma once

#include "Core.h"
#include "Layer.h"

#include <vector>

namespace Daedalus {

class DAEDALUS_API LayerStack
{
public:
	LayerStack() = default;
	~LayerStack();

	void PushLayer(Layer* layer);
	void PushOverlay(Layer* overlay);
	void PopLayer(Layer* layer);
	void PopOverlay(Layer* overlay);

	std::vector<Layer*>::iterator begin() { return m_layers.begin(); }
	std::vector<Layer*>::iterator end() { return m_layers.end(); }
	std::vector<Layer*>::reverse_iterator rbegin() { return m_layers.rbegin(); }
	std::vector<Layer*>::reverse_iterator rend() { return m_layers.rend(); }

	std::vector<Layer*>::const_iterator begin() const { return m_layers.begin(); }
	std::vector<Layer*>::const_iterator end()	const { return m_layers.end(); }
	std::vector<Layer*>::const_reverse_iterator rbegin() const { return m_layers.rbegin(); }
	std::vector<Layer*>::const_reverse_iterator rend() const { return m_layers.rend(); }

private:
	std::vector<Layer*> m_layers;
	unsigned int m_layer_insert_index = 0;
};

}