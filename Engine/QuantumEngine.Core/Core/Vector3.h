#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	struct Vector3 {
	public:
		Vector3();
		Vector3(Float x);
		Vector3(Float x, Float y, Float z);

	public:
		Float x;
		Float y;
		Float z;
	};
}