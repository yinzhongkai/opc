#pragma once

#include <string_view>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace space_rhythm::media::detail {

[[nodiscard]] constexpr std::string_view normalize_color_range(AVColorRange range) noexcept
{
    switch (range) {
    case AVCOL_RANGE_MPEG:
        return "limited";
    case AVCOL_RANGE_JPEG:
        return "full";
    case AVCOL_RANGE_UNSPECIFIED:
    default:
        return "unknown";
    }
}

} // namespace space_rhythm::media::detail
