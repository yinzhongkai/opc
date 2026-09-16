#pragma once

#include <string_view>

namespace space_rhythm::build {

inline constexpr std::string_view target_architecture{"x64"};
inline constexpr std::string_view runtime_linkage{"dynamic"};

void core_link_anchor() noexcept;

} // namespace space_rhythm::build
