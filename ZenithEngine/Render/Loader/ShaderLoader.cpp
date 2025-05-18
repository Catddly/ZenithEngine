#include "ShaderLoader.h"

#include "Core/Assertion.h"
#include "Render/Shader.h"
#include "RenderBackend/RenderDevice.h"
#include "RenderBackend/VulkanHelper.h"

namespace ZE::Render
{
	ShaderLoader::ShaderLoader(RenderBackend::RenderDevice& renderDevice)
		: m_RenderDevice{ renderDevice }
	{}
	
	bool ShaderLoader::Load(const Core::FilePath& filePath, Asset::AssetRequest& pAssetRequest)
	{
		Shader* pShaderAsset;
		const auto filename = filePath.GetExtension();
		if (filename == ".vsdr")
		{
			pShaderAsset = VertexShader::Create(m_RenderDevice);
		}
		else if (filename == ".psdr")
		{
			pShaderAsset = PixelShader::Create(m_RenderDevice);
		}
		else
		{
			ZE_UNIMPLEMENTED();
			return false;
		}
	
		auto handle = Core::FileSystem::Load(filePath);
		if (auto pBinary = handle.TryGetBinaryData(); pBinary)
		{
			VulkanZeroStruct(VkShaderModuleCreateInfo, shaderCI);
			shaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderCI.codeSize = pBinary->size();
			shaderCI.pCode = reinterpret_cast<const uint32_t*>(pBinary->data());

			pShaderAsset->m_Hash = Core::Hash(*pBinary);
			
			VulkanCheckSucceed(vkCreateShaderModule(pShaderAsset->GetRenderDevice().GetNativeDevice(), &shaderCI, nullptr, &(pShaderAsset->m_Shader)));
		}

		pAssetRequest.SetAsset(pShaderAsset);
		return true;
	}
}
