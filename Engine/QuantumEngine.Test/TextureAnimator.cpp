#include "TextureAnimator.h"
#include "Platform/CommonWin.h"

TextureAnimator::TextureAnimator(ref<Rendering::Material>& material, ref<Texture2D> tex1, ref<Texture2D> tex2, float interval)
	: m_material(material), m_interval(static_cast<UInt8>(interval))
{
	m_textures[0] = tex1;
	m_textures[1] = tex2;
}

void TextureAnimator::Update(Float deltaTime)
{
	//m_currentTime += deltaTime;
	//if (m_currentTime >= m_interval) {
	//	// Swap Textures
	//	m_index = (m_index + 1) % 2;
	//	m_material->SetTexture2D("mainTexture", m_textures[m_index]);
	//	
	//	m_currentTime = 0.0f;
	//}

	if (GetKeyState('Q') & 0x80) {
		m_currentTime += deltaTime * m_speedSign;

		if(m_currentTime > 1.0f)
			m_currentTime = 1.0f;
	}

	if (GetKeyState('E') & 0x80) {
		m_currentTime -= deltaTime * m_speedSign;
		if (m_currentTime < 0.0f)
			m_currentTime = 0.0f;
	}

	m_material->SetValue("textureFactor", m_currentTime);
}
