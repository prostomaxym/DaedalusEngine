#include "Daedalus/Utils/Random.h"

using namespace Daedalus;

std::random_device Random::s_random_device;
std::mt19937_64 Random::s_engine(s_random_device());