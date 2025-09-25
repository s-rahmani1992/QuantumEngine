#include "FrameRateLogger.h"
#include "Platform/Application.h"

void FrameRateLogger::Update(Float deltaTime)
{
	OutputDebugStringA((std::to_string(1.0f/deltaTime) + '\n').c_str());
}
