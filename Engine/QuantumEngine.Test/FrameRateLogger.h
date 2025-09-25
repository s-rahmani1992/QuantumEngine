#pragma once
#include "Core/Behaviour.h"

class FrameRateLogger : public QuantumEngine::Behaviour
{
public:
	virtual void Update(Float deltaTime) override;
};

