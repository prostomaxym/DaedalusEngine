#include "dlpch.h"

#include "LayerStack.h"

#include <algorithm>

using namespace Daedalus;

LayerStack::~LayerStack()
{
	for (auto& layer : m_layers)
	{
		layer->OnDetach();
	}
}

Layer* LayerStack::GetMainLayer()
{
	const auto it = std::find_if(m_layers.begin(), m_layers.end(),
		[](const auto& lay)
		{
			return lay->IsMain();
		});

	return it != m_layers.end() ? it->get() : nullptr;
}

void LayerStack::PushLayer(std::unique_ptr<Layer>&& layer)
{
	layer->OnAttach();
	m_layers.emplace(m_layers.begin() + m_layer_insert_index, std::move(layer));
	m_layer_insert_index++;
}

void LayerStack::PushOverlay(std::unique_ptr<Layer>&& overlay)
{
	overlay->OnAttach();
	m_layers.push_back(std::move(overlay));
}

std::unique_ptr<Layer> LayerStack::PopLayer(Layer* layer)
{
	auto it = std::find_if(m_layers.begin(), m_layers.begin() + m_layer_insert_index,
	[layer](const auto& el)
	{
		return el.get() == layer;
	});
	
	std::unique_ptr<Layer> lay = nullptr;
	if (it != m_layers.begin() + m_layer_insert_index)
	{
		layer->OnDetach();
		lay = std::move(*it);
		m_layers.erase(it);
		m_layer_insert_index--;
	}
	
	return lay;
}

std::unique_ptr<Layer> LayerStack::PopOverlay(Layer* overlay)
{
	auto it = std::find_if(m_layers.begin(), m_layers.begin() + m_layer_insert_index,
	[overlay](const auto& el)
	{
		return el.get() == overlay;
	});
	
	std::unique_ptr<Layer> lay = nullptr;
	if (it != m_layers.end())
	{
		overlay->OnDetach();
		lay = std::move(*it);
		m_layers.erase(it);
	}
	
	return lay;
}