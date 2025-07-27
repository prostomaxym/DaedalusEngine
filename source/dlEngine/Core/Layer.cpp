#include "Layer.h"

using namespace Daedalus;

Layer::Layer(const std::string& name, bool main_layer) :
	m_debug_name(name)
	, m_main_layer(main_layer)
{
}