#include "Vector3.h"

QuantumEngine::Vector3::Vector3()
	:Vector3(0.0f)
{
}

QuantumEngine::Vector3::Vector3(Float x)
	:Vector3(x, x, x)
{
}

QuantumEngine::Vector3::Vector3(Float x, Float y, Float z)
	:x(x), y(y), z(z)
{
}
