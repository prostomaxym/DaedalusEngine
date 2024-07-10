#include "dlpch.h"

#include "PointLightSource.h"

using namespace Daedalus;

PointLightSource::PointLightSource(const LightProps& props)
	: 
	LightSource(LightSourceType::Point, props)
{}