#include "TextureSwitcher.h"
#include "Platform/CommonWin.h"

TextureSwitcher::TextureSwitcher(ref<Rendering::Material>& material, const std::vector<ref<Texture2D>>& textures)
	:m_material(material), m_textures(textures)
{
}

void TextureSwitcher::Update(Float deltaTime)
{
	if (GetKeyState(VK_SPACE) & 0x80) {
		m_index = (m_index + 1) % m_textures.size();
		m_material->SetTexture2D("mainTexture", m_textures[m_index]);
	}
}
