#include "dlpch.h"

#include "LayerStack.h"

namespace Daedalus {

LayerStack::LayerStack()
{
	m_layer_insert = m_layers.begin();
}

LayerStack::~LayerStack()
{
	for (auto layer : m_layers)
	{
		delete layer;
	}
}

void LayerStack::PushLayer(Layer* layer)
{
	DL_CORE_TRACE("Layer {0} pushed.", layer->GetName());
	m_layer_insert = m_layers.emplace(m_layer_insert, layer);
}

void LayerStack::PushOverlay(Layer* overlay)
{
	DL_CORE_TRACE("Overay {0} pushed.", overlay->GetName());
	m_layers.emplace_back(overlay);
}

void LayerStack::PopLayer(Layer* layer)
{
	if (auto it = std::find(m_layers.begin(), m_layers.end(), layer);
		it != m_layers.end())
	{
		DL_CORE_TRACE("Layer {0} poped.", layer->GetName());
		m_layers.erase(it);
		m_layer_insert--;
	}
}

void LayerStack::PopOverlay(Layer* overlay)
{
	if (auto it = std::find(m_layers.begin(), m_layers.end(), overlay);
		it != m_layers.end())
	{
		DL_CORE_TRACE("Overlay {0} poped.", overlay->GetName());
		m_layers.erase(it);
	}
}

}