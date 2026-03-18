#pragma once
#include "Core/Behaviour.h"
#include <Core/Transform.h>
#include <Core/Vector3.h>
#include <Rendering/Material.h>

using namespace QuantumEngine;

class TextureAnimator : public QuantumEngine::Behaviour
{
public:
	TextureAnimator(ref<Rendering::Material>& material, ref<Texture2D> tex1, ref<Texture2D> tex2, float interval);
	virtual void Update(Float deltaTime) override;
private:
	ref<Rendering::Material> m_material;
	ref<Texture2D> m_textures[2];
	float m_currentTime = 0.0f;
	int m_index = 0;
	float m_speedSign = 1.0f;
	UInt8 m_interval;
};

