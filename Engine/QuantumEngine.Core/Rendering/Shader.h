#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine::Rendering {
	class Shader {
	public:
		Shader(Byte* byteCode, UInt64 codeSize)
			:m_codeSize(codeSize)
		{
			m_byteCode = new Byte[m_codeSize];
			std::memcpy(m_byteCode, byteCode, m_codeSize);
		}

		virtual ~Shader() {
			delete[] m_byteCode;
		}

		inline void* GetByteCode() const { return m_byteCode; }
		inline UInt64 GetCodeSize() const { return m_codeSize; }
	private:
		Byte* m_byteCode;
		UInt64 m_codeSize;
	};
}