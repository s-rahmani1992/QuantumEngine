#pragma once
#include "vulkan-pch.h"
#include "Core/VulkanGraphicContext.h"

namespace QuantumEngine::Rendering::Vulkan::RayTracing {
	class VulkanRayTracingPipelineModule;

	class VulkanRayTracingContext : public VulkanGraphicContext {
	public:
		VulkanRayTracingContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window);
		bool Initialize();
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;

	private:
		void UploadMeshes(const std::vector<ref<GameEntity>>& entities);
		ref<RayTracing::VulkanRayTracingPipelineModule> m_rayTracingModule;
	};
}