#pragma once
#include "../BasicTypes.h"
#include <string>

namespace QuantumEngine {
	struct Vector3 {
	public:
		Vector3();
		Vector3(Float x);
		Vector3(Float x, Float y, Float z);
		Vector3 Normalize() const;
		Float Magnitude() const;
		Float SquareMagnitude() const;
		Vector3 operator-();
		Vector3 operator+(const Vector3& vectorB);
		Vector3 operator*(Float fValue);
		std::string ToString();
	public:
		Float x;
		Float y;
		Float z;
	};
}

QuantumEngine::Vector3 operator*(Float fValue, const QuantumEngine::Vector3& vector);