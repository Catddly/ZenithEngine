#include "spdlog/fmt/bundled/format.h"
#include "glm/glm.hpp"

#include <format>

template<>
struct fmt::formatter<glm::mat4> : fmt::formatter<std::string>
{
	auto format(const glm::mat4& mat, format_context& ctx) const -> decltype(ctx.out())
	{
		return fmt::format_to(ctx.out(), "{}", std::format("Mat4 [ {:.3f}, {:.3f}, {:.3f}, {:.3f} ]\n     [ {:.3f}, {:.3f}, {:.3f}, {:.3f} ]\n     [ {:.3f}, {:.3f}, {:.3f}, {:.3f} ]\n     [ {:.3f}, {:.3f}, {:.3f}, {:.3f} ]",
			mat[0][0], mat[0][1], mat[0][2], mat[0][3],
			mat[1][0], mat[1][1], mat[1][2], mat[1][3],
			mat[2][0], mat[2][1], mat[2][2], mat[2][3],
			mat[3][0], mat[3][1], mat[3][2], mat[3][3]));
	}
};
