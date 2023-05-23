#include "dlpch.h"

#include "LayerStack.h"

#include <algorithm>

using namespace Daedalus;

LayerStack::~LayerStack()
{
	for (Layer* layer : m_layers)
	{
		layer->OnDetach();
		delete layer;
	}
}

void LayerStack::PushLayer(Layer* layer)
{
	m_layers.emplace(m_layers.begin() + m_layer_insert_index, layer);
	m_layer_insert_index++;
	layer->OnAttach();
}

void LayerStack::PushOverlay(Layer* overlay)
{
	m_layers.emplace_back(overlay);
	overlay->OnAttach();
}

void LayerStack::PopLayer(Layer* layer)
{
	auto it = std::find(m_layers.begin(), m_layers.begin() + m_layer_insert_index, layer);
	if (it != m_layers.begin() + m_layer_insert_index)
	{
		layer->OnDetach();
		m_layers.erase(it);
		m_layer_insert_index--;
	}
}

void LayerStack::PopOverlay(Layer* overlay)
{
	auto it = std::find(m_layers.begin() + m_layer_insert_index, m_layers.end(), overlay);
	if (it != m_layers.end())
	{
		overlay->OnDetach();
		m_layers.erase(it);
	}
}