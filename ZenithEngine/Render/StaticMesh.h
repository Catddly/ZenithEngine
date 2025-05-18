#pragma once

#include "Core/Reflection.h"
#include "Asset/Asset.h"

namespace ZE::Render
{
	class StaticMesh : public Asset::Asset
	{
		friend class StaticMeshLoader;

		ZE_CLASS_REFL(StaticMesh)
		
	public:

		struct Vertex
		{
			glm::vec3			m_Position;
			glm::vec2			m_UV;
		};

		Math::AxisAlignedBoundingBox GetAABB() const { return m_AABB; }
	
	private:

		std::vector<Vertex>								m_Vertices;
		std::vector<uint32_t>							m_Indices;
														
		Math::AxisAlignedBoundingBox					m_AABB;
	};

}

REFL_AUTO(type(ZE::Render::StaticMesh, bases<ZE::Asset::Asset>), field(m_Vertices), field(m_Indices), field(m_AABB))
