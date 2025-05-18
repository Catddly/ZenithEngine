#pragma once

#include <glm/vec3.hpp>
#include <glm/vector_relational.hpp>

namespace ZE::Math
{
	class AxisAlignedBoundingBox
	{
		static constexpr float skMin = std::numeric_limits<float>::min();
		static constexpr float skMax = std::numeric_limits<float>::max();
		
	public:

		AxisAlignedBoundingBox() = default;
		AxisAlignedBoundingBox(const glm::vec3& min, const glm::vec3& max)
			: m_Min{min}, m_Max{max}
		{}

		void Reset() { m_Min = { skMax, skMax, skMax }; m_Max = { skMin, skMin, skMin }; }
		bool IsValid() const { return glm::all(glm::greaterThanEqual(m_Max, m_Min)); }

		void MergePoint(const glm::vec3& point);

		glm::vec3 GetMin() const { return m_Min; }
		glm::vec3 GetMax() const { return m_Max; }
		
		void SetMin(const glm::vec3& min) { m_Min = min; }
		void SetMax(const glm::vec3& max) { m_Max = max; }

		bool IsInside(const glm::vec3& point) const;
		
	private:

		glm::vec3					m_Min = { skMax, skMax, skMax };
		glm::vec3					m_Max = { skMin, skMin, skMin };
	};
}
