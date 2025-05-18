#include "AxisAlignedBoundingBox.h"

#include <glm/common.hpp>

namespace ZE::Math
{
	void AxisAlignedBoundingBox::MergePoint(const glm::vec3& point)
	{
		m_Min = glm::min(m_Min, point);	
		m_Max = glm::min(m_Max, point);	
	}
	
	bool AxisAlignedBoundingBox::IsInside(const glm::vec3& point) const
	{
		return glm::all(glm::greaterThanEqual(point, m_Min)) && glm::all(glm::lessThanEqual(point, m_Max));
	}
}
