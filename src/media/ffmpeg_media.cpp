#include <space_rhythm/media/media.hpp>

#include "ffmpeg_color_range.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/display.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/sha.h>
#include <libavutil/version.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <deque>
#include <fstream>
#include <intrin.h>
#include <limits>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace space_rhythm::media {
namespace {

std::atomic_uint64_t next_diagnostic_id{1};
std::atomic_uint64_t next_buffer_id{1};

core::ErrorInfo make_error(core::ErrorCategory category,
                           core::ErrorCode code,
                           std::string_view stage,
                           std::map<std::string, std::string> context = {})
{
    const auto sequence = next_diagnostic_id.fetch_add(1, std::memory_order_relaxed);
    return core::ErrorInfo{core::schema_version,
                           category,
                           code,
                           std::string{stage},
                           "media:" + std::to_string(sequence),
                           false,
                           std::string{core::to_string(code)},
                           std::move(context),
                           {}};
}

core::ErrorInfo media_error(core::ErrorCode code,
                            std::string_view stage,
                            std::map<std::string, std::string> context = {})
{
    return make_error(core::ErrorCategory::media, code, stage, std::move(context));
}

core::ErrorInfo validation_error(core::ErrorCode code, std::string_view stage)
{
    return make_error(core::ErrorCategory::validation, code, stage);
}

core::ErrorInfo resource_error(std::string_view stage, std::string_view limit)
{
    return make_error(core::ErrorCategory::resource_limit,
                      core::ErrorCode::resource_limit,
                      stage,
                      {{"limit", std::string{limit}}});
}

core::ErrorInfo cancelled_error(std::string_view stage)
{
    return make_error(core::ErrorCategory::cancelled,
                      core::ErrorCode::transaction_cancelled,
                      stage);
}

std::string ffmpeg_error_string(int error)
{
    std::array<char, AV_ERROR_MAX_STRING_SIZE> buffer{};
    if (av_strerror(error, buffer.data(), buffer.size()) < 0) {
        return "ffmpeg_error_" + std::to_string(error);
    }
    std::string result{buffer.data()};
    if (result.size() > 160) {
        result.resize(160);
    }
    return result;
}

std::string utf8_path(const std::filesystem::path& path)
{
    const auto bytes = path.u8string();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

std::uint64_t magnitude(std::int64_t value) noexcept
{
    if (value >= 0) {
        return static_cast<std::uint64_t>(value);
    }
    return static_cast<std::uint64_t>(-(value + 1)) + 1U;
}

struct WideUnsigned {
    std::array<std::uint64_t, 3> limbs{};
};

bool wide_multiply(WideUnsigned& value, std::uint64_t multiplier) noexcept
{
    std::uint64_t carry = 0;
    for (auto& limb : value.limbs) {
        std::uint64_t high = 0;
        const auto low = _umul128(limb, multiplier, &high);
        const auto with_carry = low + carry;
        const bool carry_overflow = with_carry < low;
        limb = with_carry;
        carry = high + static_cast<std::uint64_t>(carry_overflow);
        if (carry < high) {
            return false;
        }
    }
    return carry == 0;
}

bool wide_add(WideUnsigned& left, const WideUnsigned& right) noexcept
{
    std::uint64_t carry = 0;
    for (std::size_t index = 0; index < left.limbs.size(); ++index) {
        const auto first = left.limbs[index] + right.limbs[index];
        const bool first_overflow = first < left.limbs[index];
        const auto second = first + carry;
        const bool second_overflow = second < first;
        left.limbs[index] = second;
        carry = static_cast<std::uint64_t>(first_overflow || second_overflow);
    }
    return carry == 0;
}

int wide_compare(const WideUnsigned& left, const WideUnsigned& right) noexcept
{
    for (std::size_t index = left.limbs.size(); index-- > 0;) {
        if (left.limbs[index] < right.limbs[index]) {
            return -1;
        }
        if (left.limbs[index] > right.limbs[index]) {
            return 1;
        }
    }
    return 0;
}

WideUnsigned wide_subtract(WideUnsigned left, const WideUnsigned& right) noexcept
{
    std::uint64_t borrow = 0;
    for (std::size_t index = 0; index < left.limbs.size(); ++index) {
        const auto right_with_borrow = right.limbs[index] + borrow;
        const bool carry = right_with_borrow < right.limbs[index];
        const bool next_borrow = carry || left.limbs[index] < right_with_borrow;
        left.limbs[index] -= right_with_borrow;
        borrow = static_cast<std::uint64_t>(next_borrow);
    }
    return left;
}

std::pair<WideUnsigned, std::uint64_t> wide_divide(WideUnsigned value,
                                                   std::uint64_t divisor) noexcept
{
    WideUnsigned quotient;
    std::uint64_t remainder = 0;
    for (std::size_t index = value.limbs.size(); index-- > 0;) {
        quotient.limbs[index] =
            _udiv128(remainder, value.limbs[index], divisor, &remainder);
    }
    return {quotient, remainder};
}

bool wide_increment(WideUnsigned& value) noexcept
{
    for (auto& limb : value.limbs) {
        ++limb;
        if (limb != 0) {
            return true;
        }
    }
    return false;
}

struct SignedWide {
    WideUnsigned magnitude;
    bool negative{false};
};

SignedWide add_signed(SignedWide left, SignedWide right, bool& ok) noexcept
{
    if (left.negative == right.negative) {
        ok = wide_add(left.magnitude, right.magnitude);
        return left;
    }
    const auto comparison = wide_compare(left.magnitude, right.magnitude);
    if (comparison == 0) {
        return {};
    }
    if (comparison > 0) {
        left.magnitude = wide_subtract(left.magnitude, right.magnitude);
        return left;
    }
    right.magnitude = wide_subtract(right.magnitude, left.magnitude);
    return right;
}

core::Result<std::int64_t> round_wide(SignedWide value,
                                      std::uint64_t denominator,
                                      core::RoundingMode mode,
                                      std::string_view stage)
{
    if (denominator == 0) {
        return core::Result<std::int64_t>::failure(
            validation_error(core::ErrorCode::invalid_time_base, stage));
    }
    auto [quotient, remainder] = wide_divide(value.magnitude, denominator);
    bool away = false;
    switch (mode) {
    case core::RoundingMode::floor:
        away = value.negative && remainder != 0;
        break;
    case core::RoundingMode::ceil:
        away = !value.negative && remainder != 0;
        break;
    case core::RoundingMode::toward_zero:
        break;
    case core::RoundingMode::nearest_ties_to_even: {
        const auto complement = denominator - remainder;
        away = remainder > complement
            || (remainder == complement && (quotient.limbs[0] & 1U) != 0);
        break;
    }
    }
    if (away && !wide_increment(quotient)) {
        return core::Result<std::int64_t>::failure(
            validation_error(core::ErrorCode::time_overflow, stage));
    }
    if (quotient.limbs[1] != 0 || quotient.limbs[2] != 0) {
        return core::Result<std::int64_t>::failure(
            validation_error(core::ErrorCode::time_overflow, stage));
    }
    const auto limit = value.negative
        ? static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1U
        : static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    if (quotient.limbs[0] > limit) {
        return core::Result<std::int64_t>::failure(
            validation_error(core::ErrorCode::time_overflow, stage));
    }
    if (!value.negative) {
        return core::Result<std::int64_t>::success(
            static_cast<std::int64_t>(quotient.limbs[0]));
    }
    if (quotient.limbs[0] == limit) {
        return core::Result<std::int64_t>::success(
            std::numeric_limits<std::int64_t>::min());
    }
    return core::Result<std::int64_t>::success(
        -static_cast<std::int64_t>(quotient.limbs[0]));
}

core::Result<std::int64_t> round_product(std::int64_t value,
                                         std::uint64_t multiplier,
                                         std::uint64_t denominator,
                                         core::RoundingMode mode,
                                         std::string_view stage)
{
    SignedWide product{{{magnitude(value), 0, 0}}, value < 0};
    if (!wide_multiply(product.magnitude, multiplier)) {
        return core::Result<std::int64_t>::failure(
            validation_error(core::ErrorCode::time_overflow, stage));
    }
    return round_wide(product, denominator, mode, stage);
}

std::optional<Rational> make_rational(AVRational value)
{
    if (value.num <= 0 || value.den <= 0) {
        return std::nullopt;
    }
    const auto divisor = std::gcd(value.num, value.den);
    return Rational{value.num / divisor, value.den / divisor};
}

core::TimeBase make_time_base(AVRational value)
{
    return {value.num, value.den};
}

AVRational to_av_time_base(core::TimeBase value)
{
    return {static_cast<int>(value.seconds_numerator),
            static_cast<int>(value.seconds_denominator)};
}

bool valid_time_base(core::TimeBase value) noexcept
{
    return value.seconds_numerator > 0 && value.seconds_denominator > 0
        && value.seconds_numerator <= std::numeric_limits<int>::max()
        && value.seconds_denominator <= std::numeric_limits<int>::max();
}

std::string hex_digest(const std::uint8_t* bytes, std::size_t size)
{
    static constexpr char digits[] = "0123456789abcdef";
    std::string result(size * 2, '0');
    for (std::size_t index = 0; index < size; ++index) {
        result[index * 2] = digits[bytes[index] >> 4U];
        result[index * 2 + 1] = digits[bytes[index] & 0x0fU];
    }
    return result;
}

core::Result<std::string> sha256_bytes(std::span<const std::byte> bytes,
                                       std::string_view stage)
{
    struct ShaDeleter {
        void operator()(AVSHA* value) const noexcept { av_free(value); }
    };
    std::unique_ptr<AVSHA, ShaDeleter> sha{av_sha_alloc()};
    if (!sha || av_sha_init(sha.get(), 256) != 0) {
        return core::Result<std::string>::failure(
            make_error(core::ErrorCategory::internal, core::ErrorCode::internal_error, stage));
    }
    av_sha_update(sha.get(),
                  reinterpret_cast<const std::uint8_t*>(bytes.data()),
                  bytes.size());
    std::array<std::uint8_t, 32> digest{};
    av_sha_final(sha.get(), digest.data());
    return core::Result<std::string>::success(hex_digest(digest.data(), digest.size()));
}

core::Result<std::string> sha256_file(const std::filesystem::path& path,
                                      std::uint64_t max_bytes)
{
    struct ShaDeleter {
        void operator()(AVSHA* value) const noexcept { av_free(value); }
    };
    std::unique_ptr<AVSHA, ShaDeleter> sha{av_sha_alloc()};
    if (!sha || av_sha_init(sha.get(), 256) != 0) {
        return core::Result<std::string>::failure(
            make_error(core::ErrorCategory::internal,
                       core::ErrorCode::internal_error,
                       "media.fingerprint"));
    }
    std::ifstream input{path, std::ios::binary};
    if (!input) {
        return core::Result<std::string>::failure(
            media_error(core::ErrorCode::unsupported_media, "media.open"));
    }
    std::array<std::uint8_t, 64 * 1024> buffer{};
    std::uint64_t total = 0;
    while (input) {
        input.read(reinterpret_cast<char*>(buffer.data()),
                   static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count <= 0) {
            break;
        }
        total += static_cast<std::uint64_t>(count);
        if (total > max_bytes) {
            return core::Result<std::string>::failure(
                resource_error("media.fingerprint", "max_source_bytes"));
        }
        av_sha_update(sha.get(), buffer.data(), static_cast<std::size_t>(count));
    }
    std::array<std::uint8_t, 32> digest{};
    av_sha_final(sha.get(), digest.data());
    return core::Result<std::string>::success(hex_digest(digest.data(), digest.size()));
}

struct FormatCloser {
    void operator()(AVFormatContext* value) const noexcept
    {
        if (value != nullptr) {
            avformat_close_input(&value);
        }
    }
};
struct CodecCloser {
    void operator()(AVCodecContext* value) const noexcept
    {
        if (value != nullptr) {
            avcodec_free_context(&value);
        }
    }
};
struct FrameCloser {
    void operator()(AVFrame* value) const noexcept
    {
        if (value != nullptr) {
            av_frame_free(&value);
        }
    }
};
struct PacketCloser {
    void operator()(AVPacket* value) const noexcept
    {
        if (value != nullptr) {
            av_packet_free(&value);
        }
    }
};
struct SwsCloser {
    void operator()(SwsContext* value) const noexcept { sws_freeContext(value); }
};
struct SwrCloser {
    void operator()(SwrContext* value) const noexcept
    {
        if (value != nullptr) {
            swr_free(&value);
        }
    }
};

using FormatPtr = std::unique_ptr<AVFormatContext, FormatCloser>;
using CodecPtr = std::unique_ptr<AVCodecContext, CodecCloser>;
using FramePtr = std::unique_ptr<AVFrame, FrameCloser>;
using PacketPtr = std::unique_ptr<AVPacket, PacketCloser>;
using SwsPtr = std::unique_ptr<SwsContext, SwsCloser>;
using SwrPtr = std::unique_ptr<SwrContext, SwrCloser>;

struct InterruptState {
    const core::CancellationToken* cancellation{};
    std::chrono::steady_clock::time_point deadline{};
};

int interrupt_callback(void* opaque)
{
    const auto* state = static_cast<const InterruptState*>(opaque);
    return (state->cancellation != nullptr && state->cancellation->is_cancelled())
            || std::chrono::steady_clock::now() >= state->deadline;
}

core::Result<FormatPtr> open_format(const std::filesystem::path& path,
                                    const ProbeLimits& limits,
                                    InterruptState& interrupt,
                                    std::string_view stage)
{
    AVFormatContext* raw = avformat_alloc_context();
    if (raw == nullptr) {
        return core::Result<FormatPtr>::failure(
            resource_error(stage, "format_context"));
    }
    raw->probesize = static_cast<std::int64_t>(std::min<std::uint64_t>(
        limits.max_probe_bytes, static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())));
    raw->max_analyze_duration = limits.max_analyze_duration_us;
    raw->interrupt_callback = {interrupt_callback, &interrupt};
    const auto input_name = utf8_path(path);
    const int open_result = avformat_open_input(&raw, input_name.c_str(), nullptr, nullptr);
    if (open_result < 0) {
        if (raw != nullptr) {
            avformat_free_context(raw);
        }
        if (interrupt.cancellation != nullptr && interrupt.cancellation->is_cancelled()) {
            return core::Result<FormatPtr>::failure(cancelled_error(stage));
        }
        return core::Result<FormatPtr>::failure(media_error(
            core::ErrorCode::corrupt_media,
            stage,
            {{"ffmpeg", ffmpeg_error_string(open_result)}}));
    }
    FormatPtr format{raw};
    const int info_result = avformat_find_stream_info(format.get(), nullptr);
    if (info_result < 0) {
        if (interrupt.cancellation != nullptr && interrupt.cancellation->is_cancelled()) {
            return core::Result<FormatPtr>::failure(cancelled_error(stage));
        }
        return core::Result<FormatPtr>::failure(media_error(
            core::ErrorCode::corrupt_media,
            stage,
            {{"ffmpeg", ffmpeg_error_string(info_result)}}));
    }
    if (format->nb_streams > limits.max_streams) {
        return core::Result<FormatPtr>::failure(resource_error(stage, "max_streams"));
    }
    return core::Result<FormatPtr>::success(std::move(format));
}

StreamKind stream_kind(AVMediaType type)
{
    switch (type) {
    case AVMEDIA_TYPE_VIDEO:
        return StreamKind::video;
    case AVMEDIA_TYPE_AUDIO:
        return StreamKind::audio;
    case AVMEDIA_TYPE_SUBTITLE:
        return StreamKind::subtitle;
    case AVMEDIA_TYPE_DATA:
        return StreamKind::data;
    case AVMEDIA_TYPE_ATTACHMENT:
        return StreamKind::attachment;
    default:
        return StreamKind::unknown;
    }
}

std::string enum_name(const char* name)
{
    return name == nullptr ? "unknown" : std::string{name};
}

std::string channel_layout_name(const AVChannelLayout& layout)
{
    std::array<char, 128> value{};
    if (av_channel_layout_describe(&layout, value.data(), value.size()) < 0) {
        return "unknown";
    }
    return value.data();
}

ColorDescription color_description(const AVCodecParameters& parameters)
{
    ColorDescription result;
    const auto pixel_format = static_cast<AVPixelFormat>(parameters.format);
    result.pixel_format = enum_name(av_get_pix_fmt_name(pixel_format));
    if (const auto* descriptor = av_pix_fmt_desc_get(pixel_format); descriptor != nullptr) {
        for (std::uint8_t index = 0; index < descriptor->nb_components; ++index) {
            result.bit_depth = std::max(result.bit_depth,
                                        static_cast<std::int32_t>(descriptor->comp[index].depth));
        }
    }
    result.range = detail::normalize_color_range(parameters.color_range);
    result.primaries = enum_name(av_color_primaries_name(parameters.color_primaries));
    result.transfer = enum_name(av_color_transfer_name(parameters.color_trc));
    result.matrix = enum_name(av_color_space_name(parameters.color_space));
    result.chroma_location = std::to_string(parameters.chroma_location);
    return result;
}

std::optional<DisplayTransform> display_transform(const AVCodecParameters& parameters,
                                                  const AVDictionary* metadata,
                                                  std::vector<Diagnostic>& diagnostics)
{
    const auto* side_data = av_packet_side_data_get(parameters.coded_side_data,
                                                    parameters.nb_coded_side_data,
                                                    AV_PKT_DATA_DISPLAYMATRIX);
    if (side_data != nullptr && side_data->size >= 9 * sizeof(std::int32_t)) {
        const double counter_clockwise =
            av_display_rotation_get(reinterpret_cast<const std::int32_t*>(side_data->data));
        if (!std::isfinite(counter_clockwise)) {
            diagnostics.push_back({"unsupported_display_transform", {}});
            return std::nullopt;
        }
        const double clockwise = -counter_clockwise;
        const auto rounded = static_cast<std::int32_t>(std::llround(clockwise));
        if (std::abs(clockwise - rounded) > 0.01) {
            diagnostics.push_back({"unsupported_display_transform", {}});
            return std::nullopt;
        }
        const auto normalized = ((rounded % 360) + 360) % 360;
        return DisplayTransform{normalized, false, false, "display_matrix"};
    }
    const AVDictionaryEntry* rotate = av_dict_get(metadata, "rotate", nullptr, 0);
    if (rotate != nullptr) {
        try {
            const auto degrees = std::stoi(rotate->value);
            return DisplayTransform{((degrees % 360) + 360) % 360,
                                    false,
                                    false,
                                    "rotate_metadata"};
        } catch (...) {
            diagnostics.push_back({"invalid_rotate_metadata", {}});
        }
    }
    return std::nullopt;
}

std::optional<Rational> display_aspect(std::int32_t width,
                                      std::int32_t height,
                                      const std::optional<Rational>& sar,
                                      const std::optional<DisplayTransform>& transform)
{
    if (width <= 0 || height <= 0 || !sar) {
        return std::nullopt;
    }
    std::int64_t numerator = static_cast<std::int64_t>(width) * sar->numerator;
    std::int64_t denominator = static_cast<std::int64_t>(height) * sar->denominator;
    if (transform && (transform->clockwise_rotation_degrees == 90
                      || transform->clockwise_rotation_degrees == 270)) {
        std::swap(numerator, denominator);
    }
    const auto divisor = std::gcd(numerator, denominator);
    return Rational{numerator / divisor, denominator / divisor};
}

core::Result<RationalTimestamp> frame_timestamp(const AVFrame& frame,
                                                const AVStream& stream);
bool cancelled(const core::CancellationToken* cancellation) noexcept;

core::Result<CodecPtr> open_decoder(AVStream& stream, std::uint32_t thread_count)
{
    const AVCodec* codec = avcodec_find_decoder(stream.codecpar->codec_id);
    if (codec == nullptr) {
        return core::Result<CodecPtr>::failure(
            media_error(core::ErrorCode::unsupported_media, "media.decoder.find"));
    }
    CodecPtr context{avcodec_alloc_context3(codec)};
    if (!context) {
        return core::Result<CodecPtr>::failure(
            resource_error("media.decoder.allocate", "codec_context"));
    }
    int result = avcodec_parameters_to_context(context.get(), stream.codecpar);
    if (result < 0) {
        return core::Result<CodecPtr>::failure(media_error(
            core::ErrorCode::decode_failed,
            "media.decoder.parameters",
            {{"ffmpeg", ffmpeg_error_string(result)}}));
    }
    context->pkt_timebase = stream.time_base;
    context->thread_count = static_cast<int>(std::max<std::uint32_t>(1, thread_count));
    result = avcodec_open2(context.get(), codec, nullptr);
    if (result < 0) {
        return core::Result<CodecPtr>::failure(media_error(
            core::ErrorCode::unsupported_media,
            "media.decoder.open",
            {{"ffmpeg", ffmpeg_error_string(result)}}));
    }
    return core::Result<CodecPtr>::success(std::move(context));
}

core::Result<RationalTimestamp> first_presentation_timestamp(
    const std::filesystem::path& path,
    const ProbeLimits& limits,
    std::int32_t stream_index,
    const core::CancellationToken* cancellation)
{
    InterruptState interrupt{cancellation,
                             std::chrono::steady_clock::now()
                                 + std::chrono::milliseconds{limits.timeout_ms}};
    auto format = open_format(path, limits, interrupt, "media.origin.open");
    if (!format) {
        return core::Result<RationalTimestamp>::failure(format.error());
    }
    if (stream_index < 0
        || static_cast<unsigned int>(stream_index) >= format.value()->nb_streams) {
        return core::Result<RationalTimestamp>::failure(
            media_error(core::ErrorCode::stream_not_found, "media.origin"));
    }
    AVStream& stream = *format.value()->streams[stream_index];
    auto decoder = open_decoder(stream, 1);
    if (!decoder) {
        return core::Result<RationalTimestamp>::failure(decoder.error());
    }
    PacketPtr packet{av_packet_alloc()};
    FramePtr frame{av_frame_alloc()};
    if (!packet || !frame) {
        return core::Result<RationalTimestamp>::failure(
            resource_error("media.origin", "packet_or_frame"));
    }
    std::uint64_t packets = 0;
    while (packets++ < 100'000) {
        if (cancelled(cancellation)) {
            return core::Result<RationalTimestamp>::failure(cancelled_error("media.origin"));
        }
        const int read = av_read_frame(format.value().get(), packet.get());
        if (read == AVERROR_EOF) {
            break;
        }
        if (read < 0) {
            return core::Result<RationalTimestamp>::failure(
                media_error(core::ErrorCode::corrupt_media, "media.origin.read"));
        }
        if (packet->stream_index == stream_index) {
            const int sent = avcodec_send_packet(decoder.value().get(), packet.get());
            if (sent < 0 && sent != AVERROR(EAGAIN)) {
                return core::Result<RationalTimestamp>::failure(
                    media_error(core::ErrorCode::decode_failed, "media.origin.send"));
            }
            while (true) {
                const int received = avcodec_receive_frame(decoder.value().get(), frame.get());
                if (received == AVERROR(EAGAIN) || received == AVERROR_EOF) {
                    break;
                }
                if (received < 0) {
                    return core::Result<RationalTimestamp>::failure(
                        media_error(core::ErrorCode::decode_failed, "media.origin.receive"));
                }
                auto timestamp = frame_timestamp(*frame, stream);
                if (timestamp) {
                    timestamp.value().origin = TimestampOrigin::decoded_first_presentation;
                    return timestamp;
                }
                av_frame_unref(frame.get());
            }
        }
        av_packet_unref(packet.get());
    }
    return core::Result<RationalTimestamp>::failure(media_error(
        core::ErrorCode::timestamp_origin_unavailable,
        "media.origin"));
}

std::optional<std::int32_t> find_stream_index(const MediaInfo& info,
                                              const StreamKey& key,
                                              StreamKind expected)
{
    const auto found = std::ranges::find_if(info.streams, [&](const StreamInfo& stream) {
        return stream.key == key && stream.kind == expected;
    });
    if (found == info.streams.end()) {
        return std::nullopt;
    }
    return found->key.stream_index;
}

core::Result<StreamSelectionResult> select_kind(const MediaInfo& info,
                                                StreamKind kind,
                                                const StreamSelectionRequest& request)
{
    StreamSelectionResult result;
    result.mode = request.mode;
    for (const auto& stream : info.streams) {
        if (stream.kind == kind && stream.decodable
            && (request.allow_attached_picture || !stream.attached_picture)) {
            result.candidates.push_back(stream.key);
        }
    }
    if (request.mode == SelectionMode::none) {
        result.reason = "none_requested";
        return core::Result<StreamSelectionResult>::success(std::move(result));
    }
    if (request.mode == SelectionMode::explicit_stream) {
        if (!request.explicit_key
            || request.explicit_key->source_fingerprint_sha256
                != info.source_fingerprint_sha256) {
            return core::Result<StreamSelectionResult>::failure(
                media_error(core::ErrorCode::stream_not_found, "media.select"));
        }
        const auto any_stream = std::ranges::find(info.streams,
                                                  request.explicit_key->stream_index,
                                                  [](const StreamInfo& stream) {
                                                      return stream.key.stream_index;
                                                  });
        if (any_stream == info.streams.end()) {
            return core::Result<StreamSelectionResult>::failure(
                media_error(core::ErrorCode::stream_not_found, "media.select"));
        }
        if (any_stream->kind != kind) {
            return core::Result<StreamSelectionResult>::failure(
                media_error(core::ErrorCode::stream_type_mismatch, "media.select"));
        }
        if (!any_stream->decodable) {
            return core::Result<StreamSelectionResult>::failure(
                media_error(core::ErrorCode::unsupported_media, "media.select"));
        }
        result.selected.push_back(any_stream->key);
        result.reason = "explicit_stream";
        return core::Result<StreamSelectionResult>::success(std::move(result));
    }
    if (request.mode == SelectionMode::all) {
        result.selected = result.candidates;
        result.reason = "all_decodable_streams_in_index_order";
        return core::Result<StreamSelectionResult>::success(std::move(result));
    }
    if (result.candidates.empty()) {
        if (request.mode == SelectionMode::required_default_then_lowest_index) {
            return core::Result<StreamSelectionResult>::failure(
                media_error(core::ErrorCode::missing_required_stream, "media.select"));
        }
        result.reason = "no_optional_candidate";
        return core::Result<StreamSelectionResult>::success(std::move(result));
    }
    const auto selected = std::ranges::min_element(result.candidates, [&](const auto& left,
                                                                         const auto& right) {
        const auto& left_info = info.streams.at(static_cast<std::size_t>(left.stream_index));
        const auto& right_info = info.streams.at(static_cast<std::size_t>(right.stream_index));
        return std::tuple{!left_info.default_disposition, left.stream_index}
            < std::tuple{!right_info.default_disposition, right.stream_index};
    });
    result.selected.push_back(*selected);
    result.reason = "default_disposition_then_lowest_index";
    return core::Result<StreamSelectionResult>::success(std::move(result));
}

std::optional<RationalTimestamp> earliest_known_start(const MediaInfo& info,
                                                      const MediaSelection& selection)
{
    std::optional<RationalTimestamp> earliest;
    const auto consider = [&](const StreamSelectionResult& streams) {
        for (const auto& key : streams.selected) {
            const auto& stream = info.streams.at(static_cast<std::size_t>(key.stream_index));
            if (!stream.start_pts) {
                continue;
            }
            RationalTimestamp candidate{*stream.start_pts,
                                        stream.time_base,
                                        TimestampOrigin::stream_start};
            if (!earliest
                || av_compare_ts(candidate.ticks,
                                 to_av_time_base(candidate.time_base),
                                 earliest->ticks,
                                 to_av_time_base(earliest->time_base)) < 0) {
                earliest = candidate;
            }
        }
    };
    consider(selection.video);
    consider(selection.audio);
    return earliest;
}

struct ConvertedVideo {
    std::vector<std::byte> bytes;
    std::uint32_t width{};
    std::uint32_t height{};
};

int swscale_colorspace(AVColorSpace colorspace) noexcept
{
    switch (colorspace) {
    case AVCOL_SPC_BT709:
        return SWS_CS_ITU709;
    case AVCOL_SPC_FCC:
        return SWS_CS_FCC;
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
        return SWS_CS_ITU601;
    case AVCOL_SPC_SMPTE240M:
        return SWS_CS_SMPTE240M;
    case AVCOL_SPC_BT2020_NCL:
    case AVCOL_SPC_BT2020_CL:
        return SWS_CS_BT2020;
    default:
        return SWS_CS_DEFAULT;
    }
}

core::Result<ConvertedVideo> convert_video(const AVFrame& frame,
                                           const VideoOutputSpec& output,
                                           const std::optional<DisplayTransform>& transform,
                                           std::uint64_t max_bytes)
{
    std::uint32_t rotation = output.apply_display_transform && transform
        ? static_cast<std::uint32_t>(transform->clockwise_rotation_degrees)
        : 0U;
    const bool swaps = rotation == 90U || rotation == 270U;
    std::uint32_t final_width = output.width;
    std::uint32_t final_height = output.height;
    if (final_width == 0) {
        final_width = static_cast<std::uint32_t>(swaps ? frame.height : frame.width);
    }
    if (final_height == 0) {
        final_height = static_cast<std::uint32_t>(swaps ? frame.width : frame.height);
    }
    if (final_width == 0 || final_height == 0
        || final_width > static_cast<std::uint32_t>(std::numeric_limits<int>::max() / 4)
        || final_height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        return core::Result<ConvertedVideo>::failure(
            validation_error(core::ErrorCode::time_overflow, "media.video.output"));
    }
    const int intermediate_width = static_cast<int>(swaps ? final_height : final_width);
    const int intermediate_height = static_cast<int>(swaps ? final_width : final_height);
    const auto row_bytes = static_cast<std::uint64_t>(intermediate_width) * 4U;
    const auto required = row_bytes * static_cast<std::uint64_t>(intermediate_height);
    if (required > max_bytes || required > std::numeric_limits<std::size_t>::max()) {
        return core::Result<ConvertedVideo>::failure(
            resource_error("media.video.output", "max_single_buffer_bytes"));
    }
    std::vector<std::byte> intermediate(static_cast<std::size_t>(required));
    std::array<std::uint8_t*, 4> destination{};
    std::array<int, 4> lines{};
    if (av_image_fill_arrays(destination.data(),
                             lines.data(),
                             reinterpret_cast<std::uint8_t*>(intermediate.data()),
                             AV_PIX_FMT_BGRA,
                             intermediate_width,
                             intermediate_height,
                             1) < 0) {
        return core::Result<ConvertedVideo>::failure(
            media_error(core::ErrorCode::decode_failed, "media.video.layout"));
    }
    SwsPtr scale{sws_getContext(frame.width,
                                frame.height,
                                static_cast<AVPixelFormat>(frame.format),
                                intermediate_width,
                                intermediate_height,
                                AV_PIX_FMT_BGRA,
                                SWS_BICUBIC,
                                nullptr,
                                nullptr,
                                nullptr)};
    if (!scale) {
        return core::Result<ConvertedVideo>::failure(
            media_error(core::ErrorCode::unsupported_media, "media.video.scale"));
    }
    const auto* source_coefficients =
        sws_getCoefficients(swscale_colorspace(frame.colorspace));
    const auto* output_coefficients = sws_getCoefficients(SWS_CS_ITU709);
    const int source_full_range = frame.color_range == AVCOL_RANGE_JPEG ? 1 : 0;
    if (source_coefficients == nullptr || output_coefficients == nullptr
        || sws_setColorspaceDetails(scale.get(),
                                    source_coefficients,
                                    source_full_range,
                                    output_coefficients,
                                    1,
                                    0,
                                    1 << 16,
                                    1 << 16) < 0) {
        return core::Result<ConvertedVideo>::failure(
            media_error(core::ErrorCode::unsupported_media,
                        "media.video.colorspace"));
    }
    const int converted = sws_scale(scale.get(),
                                    frame.data,
                                    frame.linesize,
                                    0,
                                    frame.height,
                                    destination.data(),
                                    lines.data());
    if (converted != intermediate_height) {
        return core::Result<ConvertedVideo>::failure(
            media_error(core::ErrorCode::decode_failed, "media.video.scale"));
    }
    if (rotation == 0U) {
        return core::Result<ConvertedVideo>::success(
            {std::move(intermediate), final_width, final_height});
    }
    std::vector<std::byte> rotated(static_cast<std::size_t>(final_width)
                                   * static_cast<std::size_t>(final_height) * 4U);
    const auto copy_pixel = [&](std::uint32_t destination_x,
                                std::uint32_t destination_y,
                                std::uint32_t source_x,
                                std::uint32_t source_y) {
        const auto source_offset =
            (static_cast<std::size_t>(source_y) * static_cast<std::size_t>(intermediate_width)
             + source_x)
            * 4U;
        const auto destination_offset =
            (static_cast<std::size_t>(destination_y) * final_width + destination_x) * 4U;
        std::copy_n(intermediate.begin() + static_cast<std::ptrdiff_t>(source_offset),
                    4,
                    rotated.begin() + static_cast<std::ptrdiff_t>(destination_offset));
    };
    for (std::uint32_t y = 0; y < final_height; ++y) {
        for (std::uint32_t x = 0; x < final_width; ++x) {
            if (rotation == 90U) {
                copy_pixel(x, y, y, static_cast<std::uint32_t>(intermediate_height) - 1U - x);
            } else if (rotation == 180U) {
                copy_pixel(x,
                           y,
                           static_cast<std::uint32_t>(intermediate_width) - 1U - x,
                           static_cast<std::uint32_t>(intermediate_height) - 1U - y);
            } else if (rotation == 270U) {
                copy_pixel(x, y, static_cast<std::uint32_t>(intermediate_width) - 1U - y, x);
            } else {
                return core::Result<ConvertedVideo>::failure(media_error(
                    core::ErrorCode::unsupported_display_transform,
                    "media.video.rotate"));
            }
        }
    }
    return core::Result<ConvertedVideo>::success(
        {std::move(rotated), final_width, final_height});
}

bool cancelled(const core::CancellationToken* cancellation) noexcept
{
    return cancellation != nullptr && cancellation->is_cancelled();
}

std::string video_signature(const AVFrame& frame,
                            const VideoOutputSpec& output,
                            const std::optional<DisplayTransform>& transform)
{
    std::ostringstream signature;
    signature << frame.width << ':' << frame.height << ':' << frame.format << ':'
              << frame.color_range << ':' << frame.color_primaries << ':' << frame.color_trc
              << ':' << frame.colorspace << ':' << frame.chroma_location << ':'
              << frame.sample_aspect_ratio.num << ':' << frame.sample_aspect_ratio.den << ':'
              << output.width << ':' << output.height << ':' << output.apply_display_transform;
    if (transform) {
        signature << ':' << transform->clockwise_rotation_degrees << ':'
                  << transform->mirror_horizontal << ':' << transform->mirror_vertical;
    }
    return signature.str();
}

std::string audio_signature(const AVFrame& frame, const AudioOutputSpec& output)
{
    std::ostringstream signature;
    signature << frame.sample_rate << ':' << frame.format << ':' << frame.ch_layout.nb_channels
              << ':' << output.sample_rate << ':' << output.channels;
    return signature.str();
}

core::Result<RationalTimestamp> frame_timestamp(const AVFrame& frame,
                                                const AVStream& stream)
{
    const auto base = frame.time_base.num > 0 && frame.time_base.den > 0
        ? frame.time_base
        : stream.time_base;
    if (frame.pts != AV_NOPTS_VALUE) {
        return core::Result<RationalTimestamp>::success(
            {frame.pts, make_time_base(base), TimestampOrigin::frame_pts});
    }
    if (frame.best_effort_timestamp != AV_NOPTS_VALUE) {
        return core::Result<RationalTimestamp>::success(
            {frame.best_effort_timestamp,
             make_time_base(base),
             TimestampOrigin::best_effort});
    }
    return core::Result<RationalTimestamp>::failure(
        media_error(core::ErrorCode::timestamp_unavailable, "media.timestamp"));
}

ColorDescription frame_color(const AVFrame& frame)
{
    ColorDescription result;
    const auto format = static_cast<AVPixelFormat>(frame.format);
    result.pixel_format = enum_name(av_get_pix_fmt_name(format));
    if (const auto* descriptor = av_pix_fmt_desc_get(format); descriptor != nullptr) {
        for (std::uint8_t index = 0; index < descriptor->nb_components; ++index) {
            result.bit_depth = std::max(result.bit_depth,
                                        static_cast<std::int32_t>(descriptor->comp[index].depth));
        }
    }
    result.range = detail::normalize_color_range(frame.color_range);
    result.primaries = enum_name(av_color_primaries_name(frame.color_primaries));
    result.transfer = enum_name(av_color_transfer_name(frame.color_trc));
    result.matrix = enum_name(av_color_space_name(frame.colorspace));
    result.chroma_location = std::to_string(frame.chroma_location);
    return result;
}

core::Result<std::int64_t> seek_timestamp(const PresentationOrigin& origin,
                                          core::TimeNs target_time_ns,
                                          core::TimeBase destination,
                                          core::RoundingMode rounding)
{
    if (!valid_time_base(origin.timestamp.time_base) || !valid_time_base(destination)) {
        return core::Result<std::int64_t>::failure(
            validation_error(core::ErrorCode::invalid_time_base, "media.seek.convert"));
    }
    const auto checked_product = [](std::uint64_t left,
                                    std::uint64_t right) -> std::optional<std::uint64_t> {
        std::uint64_t high = 0;
        const auto low = _umul128(left, right, &high);
        if (high != 0) {
            return std::nullopt;
        }
        return low;
    };
    if (origin.timestamp.time_base == destination) {
        std::uint64_t numerator_factor =
            static_cast<std::uint64_t>(destination.seconds_denominator);
        std::uint64_t nano_factor = 1'000'000'000ULL;
        std::uint64_t time_base_factor =
            static_cast<std::uint64_t>(destination.seconds_numerator);
        auto divisor = std::gcd(numerator_factor, nano_factor);
        numerator_factor /= divisor;
        nano_factor /= divisor;
        divisor = std::gcd(numerator_factor, time_base_factor);
        numerator_factor /= divisor;
        time_base_factor /= divisor;
        const auto denominator = checked_product(nano_factor, time_base_factor);
        if (!denominator) {
            return core::Result<std::int64_t>::failure(
                validation_error(core::ErrorCode::time_overflow, "media.seek.convert"));
        }
        const auto relative = round_product(target_time_ns,
                                            numerator_factor,
                                            *denominator,
                                            rounding,
                                            "media.seek.convert");
        if (!relative) {
            return relative;
        }
        const auto result = core::checked_add(origin.timestamp.ticks, relative.value());
        if (!result) {
            return core::Result<std::int64_t>::failure(result.error());
        }
        return core::Result<std::int64_t>::success(result.value());
    }

    const auto origin_den =
        static_cast<std::uint64_t>(origin.timestamp.time_base.seconds_denominator);
    const auto common = std::gcd(origin_den, 1'000'000'000ULL);
    const auto origin_scale = 1'000'000'000ULL / common;
    const auto target_scale = origin_den / common;
    SignedWide origin_term{{{magnitude(origin.timestamp.ticks), 0, 0}},
                            origin.timestamp.ticks < 0};
    SignedWide target_term{{{magnitude(target_time_ns), 0, 0}}, target_time_ns < 0};
    if (!wide_multiply(origin_term.magnitude,
                       static_cast<std::uint64_t>(
                           origin.timestamp.time_base.seconds_numerator))
        || !wide_multiply(origin_term.magnitude, origin_scale)
        || !wide_multiply(target_term.magnitude, target_scale)) {
        return core::Result<std::int64_t>::failure(
            validation_error(core::ErrorCode::time_overflow, "media.seek.convert"));
    }
    bool ok = true;
    auto absolute = add_signed(origin_term, target_term, ok);
    if (!ok) {
        return core::Result<std::int64_t>::failure(
            validation_error(core::ErrorCode::time_overflow, "media.seek.convert"));
    }
    std::uint64_t numerator_factor =
        static_cast<std::uint64_t>(destination.seconds_denominator);
    std::array<std::uint64_t, 3> denominator_factors{
        target_scale,
        1'000'000'000ULL,
        static_cast<std::uint64_t>(destination.seconds_numerator)};
    for (auto& factor : denominator_factors) {
        const auto divisor = std::gcd(numerator_factor, factor);
        numerator_factor /= divisor;
        factor /= divisor;
    }
    if (!wide_multiply(absolute.magnitude, numerator_factor)) {
        return core::Result<std::int64_t>::failure(
            validation_error(core::ErrorCode::time_overflow, "media.seek.convert"));
    }
    std::optional<std::uint64_t> denominator{1};
    for (const auto factor : denominator_factors) {
        denominator = checked_product(*denominator, factor);
        if (!denominator) {
            return core::Result<std::int64_t>::failure(
                validation_error(core::ErrorCode::time_overflow, "media.seek.convert"));
        }
    }
    return round_wide(absolute, *denominator, rounding, "media.seek.convert");
}

} // namespace

struct BufferLease::State {
    std::shared_ptr<const std::vector<std::byte>> bytes;
    std::uint64_t format_epoch{};
    std::string buffer_id;
    std::shared_ptr<void> lifetime_guard;
};

BufferLease::BufferLease(std::shared_ptr<const State> state)
    : state_(std::move(state))
{
}

BufferLease BufferLease::from_bytes(std::vector<std::byte> bytes,
                                    std::uint64_t format_epoch,
    std::string buffer_id)
{
    return BufferLease{std::make_shared<const State>(
        State{std::make_shared<const std::vector<std::byte>>(std::move(bytes)),
              format_epoch,
              std::move(buffer_id),
              {}})};
}

std::span<const std::byte> BufferLease::bytes() const noexcept
{
    if (!state_) {
        return {};
    }
    return *state_->bytes;
}

std::uint64_t BufferLease::byte_size() const noexcept
{
    return state_ ? static_cast<std::uint64_t>(state_->bytes->size()) : 0;
}

std::uint64_t BufferLease::format_epoch() const noexcept
{
    return state_ ? state_->format_epoch : 0;
}

std::string_view BufferLease::buffer_id() const noexcept
{
    return state_ ? std::string_view{state_->buffer_id} : std::string_view{};
}

BufferLease::operator bool() const noexcept
{
    return state_ != nullptr;
}

BufferLease BufferLease::with_lifetime_guard(std::shared_ptr<void> guard) const
{
    if (!state_) {
        return {};
    }
    return BufferLease{std::make_shared<const State>(
        State{state_->bytes, state_->format_epoch, state_->buffer_id, std::move(guard)})};
}

struct BoundedMediaQueue::Impl : std::enable_shared_from_this<BoundedMediaQueue::Impl> {
    struct Reservation {
        std::weak_ptr<Impl> owner;
        std::uint64_t bytes{};

        ~Reservation()
        {
            if (const auto owner_queue = owner.lock()) {
                std::scoped_lock lock{owner_queue->mutex};
                owner_queue->outstanding_bytes -= bytes;
                owner_queue->update_end_state_locked();
            }
        }
    };

    Impl(std::uint64_t item_limit, std::uint64_t byte_limit)
        : max_items(item_limit), max_bytes(byte_limit)
    {
    }

    void update_end_state_locked() noexcept
    {
        if (channel_state == ChannelState::draining && queue.empty()
            && outstanding_bytes == 0) {
            channel_state = ChannelState::ended;
        }
    }

    template <typename Buffer>
    PublishResult publish(Buffer buffer)
    {
        std::scoped_lock lock{mutex};
        if (channel_state == ChannelState::cancelled) {
            return PublishResult::cancelled;
        }
        if (channel_state != ChannelState::open) {
            return PublishResult::closed;
        }
        const auto bytes = buffer.lease.byte_size();
        if (!buffer.lease || bytes == 0 || queue.size() >= max_items
            || bytes > max_bytes - outstanding_bytes) {
            return PublishResult::would_block;
        }
        outstanding_bytes += bytes;
        auto reservation = std::make_shared<Reservation>();
        reservation->owner = weak_from_this();
        reservation->bytes = bytes;
        buffer.lease = buffer.lease.with_lifetime_guard(std::move(reservation));
        queue.emplace_back(std::move(buffer));
        return PublishResult::accepted;
    }

    mutable std::mutex mutex;
    const std::uint64_t max_items;
    const std::uint64_t max_bytes;
    std::deque<MediaBuffer> queue;
    std::uint64_t outstanding_bytes{};
    ChannelState channel_state{ChannelState::open};
};

BoundedMediaQueue::BoundedMediaQueue(std::uint64_t max_items, std::uint64_t max_bytes)
    : impl_(std::make_shared<Impl>(max_items, max_bytes))
{
    if (max_items == 0 || max_bytes == 0) {
        throw std::invalid_argument{"BoundedMediaQueue limits must be positive"};
    }
}

BoundedMediaQueue::~BoundedMediaQueue()
{
    close();
}

PublishResult BoundedMediaQueue::publish(VideoFrame frame)
{
    return impl_->publish(std::move(frame));
}

PublishResult BoundedMediaQueue::publish(PcmBuffer pcm)
{
    return impl_->publish(std::move(pcm));
}

std::optional<MediaBuffer> BoundedMediaQueue::try_take()
{
    std::scoped_lock lock{impl_->mutex};
    if (impl_->queue.empty()) {
        impl_->update_end_state_locked();
        return std::nullopt;
    }
    auto value = std::move(impl_->queue.front());
    impl_->queue.pop_front();
    impl_->update_end_state_locked();
    return value;
}

void BoundedMediaQueue::drain() noexcept
{
    std::scoped_lock lock{impl_->mutex};
    if (impl_->channel_state == ChannelState::open) {
        impl_->channel_state = ChannelState::draining;
        impl_->update_end_state_locked();
    }
}

void BoundedMediaQueue::cancel() noexcept
{
    std::deque<MediaBuffer> discarded;
    {
        std::scoped_lock lock{impl_->mutex};
        if (impl_->channel_state == ChannelState::closed
            || impl_->channel_state == ChannelState::ended) {
            return;
        }
        impl_->channel_state = ChannelState::cancelled;
        discarded.swap(impl_->queue);
    }
    discarded.clear();
}

void BoundedMediaQueue::close() noexcept
{
    std::deque<MediaBuffer> discarded;
    {
        std::scoped_lock lock{impl_->mutex};
        impl_->channel_state = ChannelState::closed;
        discarded.swap(impl_->queue);
    }
    discarded.clear();
}

ChannelState BoundedMediaQueue::state() const noexcept
{
    std::scoped_lock lock{impl_->mutex};
    return impl_->channel_state;
}

std::uint64_t BoundedMediaQueue::queued_items() const noexcept
{
    std::scoped_lock lock{impl_->mutex};
    return static_cast<std::uint64_t>(impl_->queue.size());
}

std::uint64_t BoundedMediaQueue::outstanding_bytes() const noexcept
{
    std::scoped_lock lock{impl_->mutex};
    return impl_->outstanding_bytes;
}

core::Result<FfmpegBuildInfo> query_ffmpeg_build_info()
{
    FfmpegBuildInfo result;
    result.ffmpeg_version = av_version_info();
    result.build_configuration = avcodec_configuration();
    result.license = avcodec_license();
    const auto digest = sha256_bytes(
        std::as_bytes(std::span{result.build_configuration.data(),
                                result.build_configuration.size()}),
        "media.ffmpeg.configuration");
    if (!digest) {
        return core::Result<FfmpegBuildInfo>::failure(digest.error());
    }
    result.build_configuration_sha256 = digest.value();
    const auto version_text = [](unsigned int version) {
        return std::to_string(AV_VERSION_MAJOR(version)) + "."
            + std::to_string(AV_VERSION_MINOR(version)) + "."
            + std::to_string(AV_VERSION_MICRO(version));
    };
    result.library_versions.emplace("avcodec", version_text(avcodec_version()));
    result.library_versions.emplace("avformat", version_text(avformat_version()));
    result.library_versions.emplace("avutil", version_text(avutil_version()));
    result.library_versions.emplace("swresample", version_text(swresample_version()));
    result.library_versions.emplace("swscale", version_text(swscale_version()));
    return core::Result<FfmpegBuildInfo>::success(std::move(result));
}

core::Result<core::TimeNs> map_presentation_time(const RationalTimestamp& timestamp,
                                                const PresentationOrigin& origin,
                                                core::RoundingMode rounding)
{
    if (!valid_time_base(timestamp.time_base)
        || !valid_time_base(origin.timestamp.time_base)) {
        return core::Result<core::TimeNs>::failure(
            validation_error(core::ErrorCode::invalid_time_base, "media.time.map"));
    }
    const auto left_den = static_cast<std::uint64_t>(timestamp.time_base.seconds_denominator);
    const auto right_den =
        static_cast<std::uint64_t>(origin.timestamp.time_base.seconds_denominator);
    const auto common = std::gcd(left_den, right_den);
    std::uint64_t denominator_high = 0;
    const auto denominator = _umul128(left_den / common,
                                     right_den,
                                     &denominator_high);
    if (denominator_high != 0 || denominator == 0) {
        return core::Result<core::TimeNs>::failure(
            validation_error(core::ErrorCode::time_overflow, "media.time.map"));
    }
    SignedWide left{{{magnitude(timestamp.ticks), 0, 0}}, timestamp.ticks < 0};
    SignedWide right{{{magnitude(origin.timestamp.ticks), 0, 0}},
                     origin.timestamp.ticks >= 0};
    if (!wide_multiply(left.magnitude,
                       static_cast<std::uint64_t>(timestamp.time_base.seconds_numerator))
        || !wide_multiply(left.magnitude, right_den / common)
        || !wide_multiply(right.magnitude,
                          static_cast<std::uint64_t>(
                              origin.timestamp.time_base.seconds_numerator))
        || !wide_multiply(right.magnitude, left_den / common)) {
        return core::Result<core::TimeNs>::failure(
            validation_error(core::ErrorCode::time_overflow, "media.time.map"));
    }
    bool ok = true;
    auto difference = add_signed(left, right, ok);
    if (!ok || !wide_multiply(difference.magnitude, 1'000'000'000ULL)) {
        return core::Result<core::TimeNs>::failure(
            validation_error(core::ErrorCode::time_overflow, "media.time.map"));
    }
    return round_wide(difference, denominator, rounding, "media.time.map");
}

core::Result<core::TimeNs> sample_index_to_time_ns(std::int64_t sample_index,
                                                   std::uint32_t sample_rate,
                                                   core::TimeNs origin_time_ns,
                                                   core::RoundingMode rounding)
{
    if (sample_rate == 0) {
        return core::Result<core::TimeNs>::failure(
            validation_error(core::ErrorCode::invalid_time_base, "media.sample.to_time"));
    }
    const auto relative = round_product(sample_index,
                                        1'000'000'000ULL,
                                        sample_rate,
                                        rounding,
                                        "media.sample.to_time");
    if (!relative) {
        return core::Result<core::TimeNs>::failure(relative.error());
    }
    return core::checked_add(origin_time_ns, relative.value());
}

core::Result<std::int64_t> time_ns_to_sample_index(core::TimeNs time_ns,
                                                   core::TimeNs origin_time_ns,
                                                   std::uint32_t sample_rate,
                                                   core::RoundingMode rounding)
{
    if (sample_rate == 0) {
        return core::Result<std::int64_t>::failure(
            validation_error(core::ErrorCode::invalid_time_base, "media.time.to_sample"));
    }
    const auto relative = core::checked_subtract(time_ns, origin_time_ns);
    if (!relative) {
        return core::Result<std::int64_t>::failure(relative.error());
    }
    return round_product(relative.value(),
                         sample_rate,
                         1'000'000'000ULL,
                         rounding,
                         "media.time.to_sample");
}

struct MediaSource::Impl {
    std::filesystem::path path;
    ProbeLimits probe_limits;
    MediaInfo media_info;
};

MediaSource::MediaSource(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

MediaSource::~MediaSource() = default;

core::Result<std::shared_ptr<MediaSource>> MediaSource::open(
    std::filesystem::path path,
    ProbeLimits limits,
    const core::CancellationToken* cancellation)
{
    if (limits.max_source_bytes == 0 || limits.max_probe_bytes == 0
        || limits.max_analyze_duration_us <= 0 || limits.max_streams == 0
        || limits.timeout_ms == 0) {
        return core::Result<std::shared_ptr<MediaSource>>::failure(
            resource_error("media.probe.limits", "positive_limits_required"));
    }
    std::error_code file_error;
    const auto size = std::filesystem::file_size(path, file_error);
    if (file_error) {
        return core::Result<std::shared_ptr<MediaSource>>::failure(
            media_error(core::ErrorCode::unsupported_media, "media.open"));
    }
    if (size > limits.max_source_bytes) {
        return core::Result<std::shared_ptr<MediaSource>>::failure(
            resource_error("media.probe", "max_source_bytes"));
    }
    if (cancelled(cancellation)) {
        return core::Result<std::shared_ptr<MediaSource>>::failure(
            cancelled_error("media.probe"));
    }
    const auto fingerprint = sha256_file(path, limits.max_source_bytes);
    if (!fingerprint) {
        return core::Result<std::shared_ptr<MediaSource>>::failure(fingerprint.error());
    }
    InterruptState interrupt{cancellation,
                             std::chrono::steady_clock::now()
                                 + std::chrono::milliseconds{limits.timeout_ms}};
    auto format = open_format(path, limits, interrupt, "media.probe");
    if (!format) {
        return core::Result<std::shared_ptr<MediaSource>>::failure(format.error());
    }
    auto build = query_ffmpeg_build_info();
    if (!build) {
        return core::Result<std::shared_ptr<MediaSource>>::failure(build.error());
    }
    auto impl = std::make_unique<Impl>();
    impl->path = std::move(path);
    impl->probe_limits = limits;
    auto& info = impl->media_info;
    info.source_fingerprint_sha256 = fingerprint.value();
    info.probe_implementation = std::move(build.value());
    if (format.value()->iformat != nullptr && format.value()->iformat->name != nullptr) {
        std::stringstream names{format.value()->iformat->name};
        std::string name;
        while (std::getline(names, name, ',')) {
            if (!name.empty()
                && std::ranges::find(info.format_names, name) == info.format_names.end()) {
                info.format_names.push_back(std::move(name));
            }
        }
    }
    if (format.value()->start_time != AV_NOPTS_VALUE) {
        info.format_start = RationalTimestamp{format.value()->start_time,
                                              {1, AV_TIME_BASE},
                                              TimestampOrigin::format_start};
    }
    if (format.value()->duration != AV_NOPTS_VALUE && format.value()->duration >= 0) {
        info.duration = RationalDuration{format.value()->duration, {1, AV_TIME_BASE}};
        info.duration_evidence = "header";
    }
    info.streams.reserve(format.value()->nb_streams);
    for (unsigned int index = 0; index < format.value()->nb_streams; ++index) {
        const AVStream& source = *format.value()->streams[index];
        const AVCodecParameters& parameters = *source.codecpar;
        StreamInfo stream;
        stream.key = {info.source_fingerprint_sha256, static_cast<std::int32_t>(index)};
        if (source.id >= 0) {
            stream.container_stream_id = source.id;
        }
        stream.kind = stream_kind(parameters.codec_type);
        stream.default_disposition = (source.disposition & AV_DISPOSITION_DEFAULT) != 0;
        stream.forced_disposition = (source.disposition & AV_DISPOSITION_FORCED) != 0;
        stream.attached_picture = (source.disposition & AV_DISPOSITION_ATTACHED_PIC) != 0;
        if (const auto* language = av_dict_get(source.metadata, "language", nullptr, 0);
            language != nullptr && language->value != nullptr && *language->value != '\0') {
            stream.language = language->value;
        }
        if (const auto* descriptor = avcodec_descriptor_get(parameters.codec_id);
            descriptor != nullptr && descriptor->name != nullptr) {
            stream.codec_name = descriptor->name;
        }
        stream.decodable = avcodec_find_decoder(parameters.codec_id) != nullptr;
        stream.time_base = make_time_base(source.time_base);
        if (source.start_time != AV_NOPTS_VALUE) {
            stream.start_pts = source.start_time;
        }
        if (source.duration != AV_NOPTS_VALUE) {
            stream.duration_ticks = source.duration;
        }
        stream.average_frame_rate = make_rational(source.avg_frame_rate);
        stream.nominal_frame_rate = make_rational(source.r_frame_rate);
        if (stream.kind == StreamKind::video) {
            VideoFormat video;
            video.geometry.coded_width = parameters.width;
            video.geometry.coded_height = parameters.height;
            video.geometry.sample_aspect_ratio = make_rational(source.sample_aspect_ratio);
            if (!video.geometry.sample_aspect_ratio) {
                video.geometry.sample_aspect_ratio =
                    make_rational(parameters.sample_aspect_ratio);
            }
            video.geometry.display_transform =
                display_transform(parameters, source.metadata, info.diagnostics);
            video.geometry.display_aspect_ratio = display_aspect(
                parameters.width,
                parameters.height,
                video.geometry.sample_aspect_ratio,
                video.geometry.display_transform);
            video.color = color_description(parameters);
            stream.video = std::move(video);
        } else if (stream.kind == StreamKind::audio) {
            AudioFormat audio;
            audio.sample_rate = static_cast<std::uint32_t>(parameters.sample_rate);
            audio.sample_format = enum_name(
                av_get_sample_fmt_name(static_cast<AVSampleFormat>(parameters.format)));
            audio.channel_layout = channel_layout_name(parameters.ch_layout);
            audio.planar = av_sample_fmt_is_planar(
                               static_cast<AVSampleFormat>(parameters.format))
                != 0;
            stream.audio = std::move(audio);
        }
        info.streams.push_back(std::move(stream));
    }
    return core::Result<std::shared_ptr<MediaSource>>::success(
        std::shared_ptr<MediaSource>{new MediaSource{std::move(impl)}});
}

const MediaInfo& MediaSource::info() const noexcept
{
    return impl_->media_info;
}

core::Result<MediaSelection> MediaSource::select(
    const StreamSelectionRequest& video,
    const StreamSelectionRequest& audio,
    const core::CancellationToken* cancellation) const
{
    auto video_result = select_kind(impl_->media_info, StreamKind::video, video);
    if (!video_result) {
        return core::Result<MediaSelection>::failure(video_result.error());
    }
    auto audio_result = select_kind(impl_->media_info, StreamKind::audio, audio);
    if (!audio_result) {
        return core::Result<MediaSelection>::failure(audio_result.error());
    }
    MediaSelection selection;
    selection.video = std::move(video_result.value());
    selection.audio = std::move(audio_result.value());
    const auto add_default_diagnostic = [&](const StreamSelectionResult& result,
                                            std::string_view kind) {
        const auto defaults = std::ranges::count_if(result.candidates, [&](const auto& key) {
            return impl_->media_info.streams
                .at(static_cast<std::size_t>(key.stream_index))
                .default_disposition;
        });
        if (defaults > 1) {
            selection.diagnostics.push_back(
                {"multiple_default_streams",
                 {{"kind", std::string{kind}}, {"count", std::to_string(defaults)}}});
        }
    };
    add_default_diagnostic(selection.video, "video");
    add_default_diagnostic(selection.audio, "audio");
    if (impl_->media_info.format_start) {
        selection.presentation_origin = {*impl_->media_info.format_start, false};
    } else if (const auto start = earliest_known_start(impl_->media_info, selection)) {
        selection.presentation_origin = {*start, false};
    } else {
        std::optional<RationalTimestamp> earliest;
        const auto consider = [&](const StreamSelectionResult& streams)
            -> std::optional<core::ErrorInfo> {
            for (const auto& key : streams.selected) {
                auto timestamp = first_presentation_timestamp(
                    impl_->path, impl_->probe_limits, key.stream_index, cancellation);
                if (!timestamp) {
                    return timestamp.error();
                }
                if (!earliest
                    || av_compare_ts(timestamp.value().ticks,
                                     to_av_time_base(timestamp.value().time_base),
                                     earliest->ticks,
                                     to_av_time_base(earliest->time_base)) < 0) {
                    earliest = timestamp.value();
                }
            }
            return std::nullopt;
        };
        if (const auto error = consider(selection.video)) {
            return core::Result<MediaSelection>::failure(*error);
        }
        if (const auto error = consider(selection.audio)) {
            return core::Result<MediaSelection>::failure(*error);
        }
        if (!earliest) {
            return core::Result<MediaSelection>::failure(media_error(
                core::ErrorCode::timestamp_origin_unavailable,
                "media.select.origin"));
        }
        selection.presentation_origin = {*earliest, false};
    }
    return core::Result<MediaSelection>::success(std::move(selection));
}

core::Result<DecodeSummary> MediaSource::decode_video(
    const MediaSelection& selection,
    const StreamKey& stream,
    const VideoOutputSpec& output,
    const DecodeLimits& limits,
    const VideoCallbacks& callbacks,
    const core::CancellationToken* cancellation) const
{
    const auto stream_index = find_stream_index(impl_->media_info, stream, StreamKind::video);
    if (!stream_index || std::ranges::find(selection.video.selected, stream)
            == selection.video.selected.end()) {
        return core::Result<DecodeSummary>::failure(
            media_error(core::ErrorCode::stream_not_found, "media.video.decode"));
    }
    if (!callbacks.on_frame || limits.max_packets == 0 || limits.max_frames == 0
        || limits.max_output_bytes == 0 || limits.max_single_buffer_bytes == 0
        || limits.decoder_threads == 0) {
        return core::Result<DecodeSummary>::failure(
            resource_error("media.video.decode", "positive_limits_and_callback_required"));
    }
    InterruptState interrupt{cancellation,
                             std::chrono::steady_clock::now()
                                 + std::chrono::milliseconds{impl_->probe_limits.timeout_ms}};
    auto format = open_format(impl_->path,
                              impl_->probe_limits,
                              interrupt,
                              "media.video.decode.open");
    if (!format) {
        return core::Result<DecodeSummary>::failure(format.error());
    }
    AVStream& av_stream = *format.value()->streams[*stream_index];
    auto decoder = open_decoder(av_stream, limits.decoder_threads);
    if (!decoder) {
        return core::Result<DecodeSummary>::failure(decoder.error());
    }
    PacketPtr packet{av_packet_alloc()};
    FramePtr frame{av_frame_alloc()};
    if (!packet || !frame) {
        return core::Result<DecodeSummary>::failure(
            resource_error("media.video.decode", "packet_or_frame"));
    }
    const auto& probed_stream =
        impl_->media_info.streams.at(static_cast<std::size_t>(*stream_index));
    const auto transform = probed_stream.video
        ? probed_stream.video->geometry.display_transform
        : std::optional<DisplayTransform>{};
    DecodeSummary summary;
    std::string previous_signature;
    std::uint64_t format_epoch = 0;
    std::uint64_t decode_ordinal = 0;
    std::optional<core::TimeNs> previous_time_ns;
    auto receive = [&]() -> std::optional<core::ErrorInfo> {
        while (true) {
            const int receive_result = avcodec_receive_frame(decoder.value().get(), frame.get());
            if (receive_result == AVERROR(EAGAIN) || receive_result == AVERROR_EOF) {
                return std::nullopt;
            }
            if (receive_result < 0) {
                return media_error(core::ErrorCode::decode_failed,
                                   "media.video.receive",
                                   {{"ffmpeg", ffmpeg_error_string(receive_result)}});
            }
            if (++summary.frames_published > limits.max_frames) {
                return resource_error("media.video.decode", "max_frames");
            }
            const auto timestamp = frame_timestamp(*frame, av_stream);
            if (!timestamp) {
                return timestamp.error();
            }
            const auto mapped = map_presentation_time(timestamp.value(),
                                                      selection.presentation_origin,
                                                      core::RoundingMode::nearest_ties_to_even);
            if (!mapped) {
                return mapped.error();
            }
            if (timestamp.value().origin == TimestampOrigin::best_effort) {
                summary.diagnostics.push_back(
                    {"best_effort_timestamp",
                     {{"decodeOrdinal", std::to_string(decode_ordinal)}}});
            }
            if (previous_time_ns && mapped.value() < *previous_time_ns) {
                summary.diagnostics.push_back(
                    {"timestamp_discontinuity",
                     {{"decodeOrdinal", std::to_string(decode_ordinal)},
                      {"previousTimeNs", std::to_string(*previous_time_ns)},
                      {"timeNs", std::to_string(mapped.value())}}});
            }
            previous_time_ns = mapped.value();
            auto converted =
                convert_video(*frame, output, transform, limits.max_single_buffer_bytes);
            if (!converted) {
                return converted.error();
            }
            auto normalized_color = frame_color(*frame);
            const bool color_assumed = normalized_color.range == "unknown"
                || normalized_color.primaries == "unknown"
                || normalized_color.transfer == "unknown"
                || normalized_color.matrix == "unknown";
            normalized_color.pixel_format = "bgra";
            normalized_color.bit_depth = 8;
            normalized_color.range = detail::normalize_color_range(AVCOL_RANGE_JPEG);
            normalized_color.matrix = "rgb";
            normalized_color.assumed = color_assumed;
            const DisplayGeometry normalized_geometry{
                static_cast<std::int32_t>(converted.value().width),
                static_cast<std::int32_t>(converted.value().height),
                0,
                0,
                0,
                0,
                Rational{1, 1},
                std::nullopt,
                Rational{converted.value().width, converted.value().height}};
            const auto signature = video_signature(*frame, output, transform);
            if (signature != previous_signature) {
                ++format_epoch;
                previous_signature = signature;
                FormatChanged change;
                change.format_epoch = format_epoch;
                change.stream_key = stream;
                VideoFormat new_format;
                new_format.geometry = normalized_geometry;
                new_format.color = normalized_color;
                change.video = std::move(new_format);
                if (callbacks.on_format_changed) {
                    const auto published = callbacks.on_format_changed(change);
                    if (published != PublishResult::accepted) {
                        summary.terminal_publish_result = published;
                        return std::nullopt;
                    }
                }
            }
            const auto byte_size = static_cast<std::uint64_t>(converted.value().bytes.size());
            if (summary.bytes_published > limits.max_output_bytes - byte_size) {
                return resource_error("media.video.decode", "max_output_bytes");
            }
            summary.bytes_published += byte_size;
            summary.peak_single_buffer_bytes =
                std::max(summary.peak_single_buffer_bytes, byte_size);
            VideoFrame delivered;
            delivered.stream_key = stream;
            delivered.time_ns = mapped.value();
            if (frame->duration > 0) {
                const auto duration = core::scale_ticks(frame->duration,
                                                        timestamp.value().time_base,
                                                        core::RoundingMode::nearest_ties_to_even);
                if (duration) {
                    delivered.duration_ns = duration.value();
                }
            }
            delivered.source_pts = timestamp.value();
            if (frame->pkt_dts != AV_NOPTS_VALUE) {
                delivered.source_dts = RationalTimestamp{frame->pkt_dts,
                                                         timestamp.value().time_base,
                                                         TimestampOrigin::packet_dts};
            }
            delivered.timestamp_origin = timestamp.value().origin;
            delivered.decode_ordinal = decode_ordinal++;
            delivered.key_frame = (frame->flags & AV_FRAME_FLAG_KEY) != 0;
            delivered.decode_had_errors = frame->decode_error_flags != 0;
            delivered.geometry = normalized_geometry;
            delivered.color = normalized_color;
            delivered.pixel_format = "bgra";
            delivered.planes.push_back({0,
                                        static_cast<std::int64_t>(converted.value().width) * 4,
                                        converted.value().height,
                                        byte_size});
            delivered.lease = BufferLease::from_bytes(
                std::move(converted.value().bytes),
                format_epoch,
                "video:" + std::to_string(
                               next_buffer_id.fetch_add(1, std::memory_order_relaxed)));
            const auto published = callbacks.on_frame(std::move(delivered));
            av_frame_unref(frame.get());
            if (published != PublishResult::accepted) {
                summary.terminal_publish_result = published;
                return std::nullopt;
            }
            if (cancelled(cancellation)) {
                return cancelled_error("media.video.decode");
            }
        }
    };
    while (true) {
        if (cancelled(cancellation)) {
            return core::Result<DecodeSummary>::failure(cancelled_error("media.video.decode"));
        }
        const int read_result = av_read_frame(format.value().get(), packet.get());
        if (read_result == AVERROR_EOF) {
            break;
        }
        if (read_result < 0) {
            return core::Result<DecodeSummary>::failure(media_error(
                core::ErrorCode::corrupt_media,
                "media.video.read",
                {{"ffmpeg", ffmpeg_error_string(read_result)}}));
        }
        if (++summary.packets_read > limits.max_packets) {
            return core::Result<DecodeSummary>::failure(
                resource_error("media.video.decode", "max_packets"));
        }
        if (packet->stream_index == *stream_index) {
            const int send_result = avcodec_send_packet(decoder.value().get(), packet.get());
            if (send_result < 0) {
                return core::Result<DecodeSummary>::failure(media_error(
                    core::ErrorCode::decode_failed,
                    "media.video.send",
                    {{"ffmpeg", ffmpeg_error_string(send_result)}}));
            }
            if (const auto error = receive()) {
                return core::Result<DecodeSummary>::failure(*error);
            }
            if (summary.terminal_publish_result != PublishResult::accepted) {
                return core::Result<DecodeSummary>::success(summary);
            }
        }
        av_packet_unref(packet.get());
    }
    const int flush_result = avcodec_send_packet(decoder.value().get(), nullptr);
    if (flush_result >= 0) {
        if (const auto error = receive()) {
            return core::Result<DecodeSummary>::failure(*error);
        }
    }
    summary.end_of_stream = true;
    return core::Result<DecodeSummary>::success(summary);
}

core::Result<DecodeSummary> MediaSource::decode_audio(
    const MediaSelection& selection,
    const StreamKey& stream,
    const AudioOutputSpec& output,
    const DecodeLimits& limits,
    const AudioCallbacks& callbacks,
    const core::CancellationToken* cancellation) const
{
    const auto stream_index = find_stream_index(impl_->media_info, stream, StreamKind::audio);
    if (!stream_index || std::ranges::find(selection.audio.selected, stream)
            == selection.audio.selected.end()) {
        return core::Result<DecodeSummary>::failure(
            media_error(core::ErrorCode::stream_not_found, "media.audio.decode"));
    }
    if (!callbacks.on_pcm || output.sample_rate == 0 || output.channels == 0
        || output.channels > 2 || limits.max_packets == 0 || limits.max_frames == 0
        || limits.max_output_bytes == 0 || limits.max_single_buffer_bytes == 0) {
        return core::Result<DecodeSummary>::failure(
            resource_error("media.audio.decode", "positive_limits_and_callback_required"));
    }
    InterruptState interrupt{cancellation,
                             std::chrono::steady_clock::now()
                                 + std::chrono::milliseconds{impl_->probe_limits.timeout_ms}};
    auto format = open_format(impl_->path,
                              impl_->probe_limits,
                              interrupt,
                              "media.audio.decode.open");
    if (!format) {
        return core::Result<DecodeSummary>::failure(format.error());
    }
    AVStream& av_stream = *format.value()->streams[*stream_index];
    auto decoder = open_decoder(av_stream, limits.decoder_threads);
    if (!decoder) {
        return core::Result<DecodeSummary>::failure(decoder.error());
    }
    PacketPtr packet{av_packet_alloc()};
    FramePtr frame{av_frame_alloc()};
    if (!packet || !frame) {
        return core::Result<DecodeSummary>::failure(
            resource_error("media.audio.decode", "packet_or_frame"));
    }
    DecodeSummary summary;
    SwrPtr resampler;
    std::string previous_signature;
    std::uint64_t format_epoch = 0;
    std::uint64_t segment = 0;
    std::int64_t next_sample_index = 0;
    bool have_sample_index = false;
    std::int64_t next_source_sample_index = 0;
    std::uint32_t source_sample_rate = 0;
    bool have_source_sample_index = false;
    auto publish_pcm = [&](std::vector<std::byte> bytes,
                           int converted) -> std::optional<core::ErrorInfo> {
        if (converted <= 0) {
            return std::nullopt;
        }
        const auto byte_size = static_cast<std::uint64_t>(bytes.size());
        if (byte_size > limits.max_output_bytes
            || summary.bytes_published > limits.max_output_bytes - byte_size) {
            return resource_error("media.audio.decode", "max_output_bytes");
        }
        const auto duration = core::scale_ticks(converted,
                                                {1, output.sample_rate},
                                                core::RoundingMode::nearest_ties_to_even);
        const auto time = sample_index_to_time_ns(next_sample_index,
                                                  output.sample_rate,
                                                  0,
                                                  core::RoundingMode::nearest_ties_to_even);
        if (!duration || !time) {
            return duration ? time.error() : duration.error();
        }
        PcmBuffer delivered;
        delivered.stream_key = stream;
        delivered.time_ns = time.value();
        delivered.duration_ns = duration.value();
        delivered.segment_id = "audio-segment:" + std::to_string(segment);
        delivered.first_sample_index = next_sample_index;
        delivered.sample_count = static_cast<std::uint64_t>(converted);
        delivered.sample_rate = output.sample_rate;
        delivered.sample_format = "flt";
        delivered.channel_layout = output.channels == 1 ? "mono" : "stereo";
        delivered.planar = false;
        delivered.planes.push_back({0,
                                    static_cast<std::int64_t>(output.channels
                                                              * sizeof(float)),
                                    static_cast<std::uint32_t>(converted),
                                    byte_size});
        delivered.lease = BufferLease::from_bytes(
            std::move(bytes),
            format_epoch,
            "audio:"
                + std::to_string(next_buffer_id.fetch_add(1, std::memory_order_relaxed)));
        const auto published = callbacks.on_pcm(std::move(delivered));
        next_sample_index += converted;
        summary.samples_published += static_cast<std::uint64_t>(converted);
        summary.bytes_published += byte_size;
        summary.peak_single_buffer_bytes =
            std::max(summary.peak_single_buffer_bytes, byte_size);
        if (published != PublishResult::accepted) {
            summary.terminal_publish_result = published;
        }
        return std::nullopt;
    };
    auto drain_resampler = [&]() -> std::optional<core::ErrorInfo> {
        if (!resampler) {
            return std::nullopt;
        }
        while (true) {
            if (cancelled(cancellation)) {
                return cancelled_error("media.audio.drain");
            }
            const int output_capacity = swr_get_out_samples(resampler.get(), 0);
            if (output_capacity < 0) {
                return media_error(core::ErrorCode::decode_failed,
                                   "media.audio.resampler.drain_capacity");
            }
            if (output_capacity == 0) {
                return std::nullopt;
            }
            const auto byte_capacity = static_cast<std::uint64_t>(output_capacity)
                * output.channels * sizeof(float);
            if (byte_capacity > limits.max_single_buffer_bytes
                || byte_capacity > std::numeric_limits<std::size_t>::max()) {
                return resource_error("media.audio.decode", "max_single_buffer_bytes");
            }
            std::vector<std::byte> bytes(static_cast<std::size_t>(byte_capacity));
            std::uint8_t* output_data = reinterpret_cast<std::uint8_t*>(bytes.data());
            const int converted = swr_convert(resampler.get(),
                                              &output_data,
                                              output_capacity,
                                              nullptr,
                                              0);
            if (converted < 0) {
                return media_error(core::ErrorCode::decode_failed,
                                   "media.audio.resampler.drain",
                                   {{"ffmpeg", ffmpeg_error_string(converted)}});
            }
            if (converted == 0) {
                return std::nullopt;
            }
            bytes.resize(static_cast<std::size_t>(converted)
                         * output.channels * sizeof(float));
            if (const auto error = publish_pcm(std::move(bytes), converted)) {
                return error;
            }
            if (summary.terminal_publish_result != PublishResult::accepted) {
                return std::nullopt;
            }
        }
    };
    auto receive = [&]() -> std::optional<core::ErrorInfo> {
        while (true) {
            const int receive_result = avcodec_receive_frame(decoder.value().get(), frame.get());
            if (receive_result == AVERROR(EAGAIN) || receive_result == AVERROR_EOF) {
                return std::nullopt;
            }
            if (receive_result < 0) {
                return media_error(core::ErrorCode::decode_failed,
                                   "media.audio.receive",
                                   {{"ffmpeg", ffmpeg_error_string(receive_result)}});
            }
            if (++summary.frames_published > limits.max_frames) {
                return resource_error("media.audio.decode", "max_frames");
            }
            const auto timestamp = frame_timestamp(*frame, av_stream);
            if (!timestamp) {
                return timestamp.error();
            }
            const auto mapped = map_presentation_time(timestamp.value(),
                                                      selection.presentation_origin,
                                                      core::RoundingMode::nearest_ties_to_even);
            if (!mapped) {
                return mapped.error();
            }
            if (timestamp.value().origin == TimestampOrigin::best_effort) {
                summary.diagnostics.push_back(
                    {"best_effort_timestamp",
                     {{"decodedFrame", std::to_string(summary.frames_published - 1)}}});
            }
            const auto signature = audio_signature(*frame, output);
            if (signature != previous_signature) {
                if (const auto error = drain_resampler()) {
                    return error;
                }
                if (summary.terminal_publish_result != PublishResult::accepted) {
                    return std::nullopt;
                }
                ++format_epoch;
                ++segment;
                previous_signature = signature;
                SwrContext* raw = nullptr;
                AVChannelLayout target_layout{};
                av_channel_layout_default(&target_layout, static_cast<int>(output.channels));
                const int allocation = swr_alloc_set_opts2(&raw,
                                                           &target_layout,
                                                           AV_SAMPLE_FMT_FLT,
                                                           static_cast<int>(output.sample_rate),
                                                           &frame->ch_layout,
                                                           static_cast<AVSampleFormat>(frame->format),
                                                           frame->sample_rate,
                                                           0,
                                                           nullptr);
                av_channel_layout_uninit(&target_layout);
                if (allocation < 0 || raw == nullptr || swr_init(raw) < 0) {
                    if (raw != nullptr) {
                        swr_free(&raw);
                    }
                    return media_error(core::ErrorCode::unsupported_media,
                                       "media.audio.resampler");
                }
                resampler.reset(raw);
                have_sample_index = false;
                have_source_sample_index = false;
                FormatChanged change;
                change.format_epoch = format_epoch;
                change.stream_key = stream;
                change.audio = AudioFormat{output.sample_rate,
                                           "flt",
                                           output.channels == 1 ? "mono" : "stereo",
                                           false};
                if (callbacks.on_format_changed) {
                    const auto published = callbacks.on_format_changed(change);
                    if (published != PublishResult::accepted) {
                        summary.terminal_publish_result = published;
                        return std::nullopt;
                    }
                }
            }
            const auto timestamp_sample = time_ns_to_sample_index(
                mapped.value(),
                0,
                output.sample_rate,
                core::RoundingMode::nearest_ties_to_even);
            const auto source_timestamp_sample = time_ns_to_sample_index(
                mapped.value(),
                0,
                static_cast<std::uint32_t>(frame->sample_rate),
                core::RoundingMode::nearest_ties_to_even);
            if (!timestamp_sample || !source_timestamp_sample) {
                return timestamp_sample ? source_timestamp_sample.error()
                                        : timestamp_sample.error();
            }
            const bool discontinuity = have_source_sample_index
                && (source_sample_rate != static_cast<std::uint32_t>(frame->sample_rate)
                    || source_timestamp_sample.value() != next_source_sample_index);
            if (!have_sample_index || discontinuity) {
                if (discontinuity) {
                    summary.diagnostics.push_back(
                        {"timestamp_discontinuity",
                         {{"expectedSourceSampleIndex",
                           std::to_string(next_source_sample_index)},
                          {"actualSourceSampleIndex",
                           std::to_string(source_timestamp_sample.value())},
                          {"sourceSampleRate", std::to_string(frame->sample_rate)}}});
                    ++segment;
                }
                next_sample_index = timestamp_sample.value();
                have_sample_index = true;
            }
            const auto checked_source_next = core::checked_add(
                source_timestamp_sample.value(), static_cast<std::int64_t>(frame->nb_samples));
            if (!checked_source_next) {
                return checked_source_next.error();
            }
            next_source_sample_index = checked_source_next.value();
            source_sample_rate = static_cast<std::uint32_t>(frame->sample_rate);
            have_source_sample_index = true;
            const int output_capacity = swr_get_out_samples(resampler.get(), frame->nb_samples);
            if (output_capacity < 0) {
                return media_error(core::ErrorCode::decode_failed,
                                   "media.audio.resampler.capacity");
            }
            const auto byte_capacity = static_cast<std::uint64_t>(output_capacity)
                * output.channels * sizeof(float);
            if (byte_capacity > limits.max_single_buffer_bytes
                || byte_capacity > std::numeric_limits<std::size_t>::max()) {
                return resource_error("media.audio.decode", "max_single_buffer_bytes");
            }
            std::vector<std::byte> bytes(static_cast<std::size_t>(byte_capacity));
            std::uint8_t* output_data = reinterpret_cast<std::uint8_t*>(bytes.data());
            const std::uint8_t* const* input_data =
                const_cast<const std::uint8_t* const*>(frame->extended_data);
            const int converted = swr_convert(resampler.get(),
                                              &output_data,
                                              output_capacity,
                                              input_data,
                                              frame->nb_samples);
            if (converted < 0) {
                return media_error(core::ErrorCode::decode_failed,
                                   "media.audio.resampler.convert",
                                   {{"ffmpeg", ffmpeg_error_string(converted)}});
            }
            bytes.resize(static_cast<std::size_t>(converted)
                         * output.channels * sizeof(float));
            if (const auto error = publish_pcm(std::move(bytes), converted)) {
                return error;
            }
            av_frame_unref(frame.get());
            if (summary.terminal_publish_result != PublishResult::accepted) {
                return std::nullopt;
            }
            if (cancelled(cancellation)) {
                return cancelled_error("media.audio.decode");
            }
        }
    };
    while (true) {
        if (cancelled(cancellation)) {
            return core::Result<DecodeSummary>::failure(cancelled_error("media.audio.decode"));
        }
        const int read_result = av_read_frame(format.value().get(), packet.get());
        if (read_result == AVERROR_EOF) {
            break;
        }
        if (read_result < 0) {
            return core::Result<DecodeSummary>::failure(media_error(
                core::ErrorCode::corrupt_media,
                "media.audio.read",
                {{"ffmpeg", ffmpeg_error_string(read_result)}}));
        }
        if (++summary.packets_read > limits.max_packets) {
            return core::Result<DecodeSummary>::failure(
                resource_error("media.audio.decode", "max_packets"));
        }
        if (packet->stream_index == *stream_index) {
            const int send_result = avcodec_send_packet(decoder.value().get(), packet.get());
            if (send_result < 0) {
                return core::Result<DecodeSummary>::failure(media_error(
                    core::ErrorCode::decode_failed,
                    "media.audio.send",
                    {{"ffmpeg", ffmpeg_error_string(send_result)}}));
            }
            if (const auto error = receive()) {
                return core::Result<DecodeSummary>::failure(*error);
            }
            if (summary.terminal_publish_result != PublishResult::accepted) {
                return core::Result<DecodeSummary>::success(summary);
            }
        }
        av_packet_unref(packet.get());
    }
    const int flush_result = avcodec_send_packet(decoder.value().get(), nullptr);
    if (flush_result >= 0) {
        if (const auto error = receive()) {
            return core::Result<DecodeSummary>::failure(*error);
        }
    }
    if (summary.terminal_publish_result != PublishResult::accepted) {
        return core::Result<DecodeSummary>::success(summary);
    }
    if (const auto error = drain_resampler()) {
        return core::Result<DecodeSummary>::failure(*error);
    }
    if (summary.terminal_publish_result != PublishResult::accepted) {
        return core::Result<DecodeSummary>::success(summary);
    }
    summary.end_of_stream = true;
    return core::Result<DecodeSummary>::success(summary);
}

core::Result<SeekResult> MediaSource::seek_video(
    const MediaSelection& selection,
    const SeekRequest& request,
    const VideoOutputSpec& output,
    const DecodeLimits& limits,
    const core::CancellationToken* cancellation) const
{
    if (request.tolerance_before_ns < 0 || request.tolerance_after_ns < 0) {
        return core::Result<SeekResult>::failure(
            validation_error(core::ErrorCode::invalid_dto, "media.seek"));
    }
    const auto stream_index =
        find_stream_index(impl_->media_info, request.selected_stream, StreamKind::video);
    if (!stream_index) {
        return core::Result<SeekResult>::failure(
            media_error(core::ErrorCode::stream_not_found, "media.seek"));
    }
    const auto time_base = impl_->media_info.streams
                               .at(static_cast<std::size_t>(*stream_index))
                               .time_base;
    core::RoundingMode rounding = core::RoundingMode::nearest_ties_to_even;
    if (request.mode == SeekMode::at_or_before) {
        rounding = core::RoundingMode::floor;
    } else if (request.mode == SeekMode::at_or_after) {
        rounding = core::RoundingMode::ceil;
    }
    const auto target_ticks =
        seek_timestamp(selection.presentation_origin, request.target_time_ns, time_base, rounding);
    if (!target_ticks) {
        return core::Result<SeekResult>::failure(target_ticks.error());
    }
    const auto checked_minimum =
        core::checked_subtract(request.target_time_ns, request.tolerance_before_ns);
    const auto checked_maximum =
        core::checked_add(request.target_time_ns, request.tolerance_after_ns);
    auto minimum_tick_value = std::numeric_limits<std::int64_t>::min();
    auto maximum_tick_value = std::numeric_limits<std::int64_t>::max();
    if (checked_minimum) {
        const auto converted = seek_timestamp(selection.presentation_origin,
                                              checked_minimum.value(),
                                              time_base,
                                              core::RoundingMode::floor);
        if (!converted) {
            return core::Result<SeekResult>::failure(converted.error());
        }
        minimum_tick_value = converted.value();
    }
    if (checked_maximum) {
        const auto converted = seek_timestamp(selection.presentation_origin,
                                              checked_maximum.value(),
                                              time_base,
                                              core::RoundingMode::ceil);
        if (!converted) {
            return core::Result<SeekResult>::failure(converted.error());
        }
        maximum_tick_value = converted.value();
    }
    InterruptState interrupt{cancellation,
                             std::chrono::steady_clock::now()
                                 + std::chrono::milliseconds{impl_->probe_limits.timeout_ms}};
    auto format = open_format(impl_->path, impl_->probe_limits, interrupt, "media.seek.open");
    if (!format) {
        return core::Result<SeekResult>::failure(format.error());
    }
    int seek_result = avformat_seek_file(format.value().get(),
                                         *stream_index,
                                         minimum_tick_value,
                                         target_ticks.value(),
                                         maximum_tick_value,
                                         0);
    if (seek_result < 0) {
        seek_result = avformat_seek_file(format.value().get(),
                                         *stream_index,
                                         std::numeric_limits<std::int64_t>::min(),
                                         target_ticks.value(),
                                         maximum_tick_value,
                                         AVSEEK_FLAG_BACKWARD);
    }
    if (seek_result < 0) {
        return core::Result<SeekResult>::failure(media_error(
            core::ErrorCode::seek_unreachable,
            "media.seek.demux",
            {{"ffmpeg", ffmpeg_error_string(seek_result)}}));
    }
    AVStream& av_stream = *format.value()->streams[*stream_index];
    auto decoder = open_decoder(av_stream, limits.decoder_threads);
    if (!decoder) {
        return core::Result<SeekResult>::failure(decoder.error());
    }
    avcodec_flush_buffers(decoder.value().get());
    PacketPtr packet{av_packet_alloc()};
    FramePtr frame{av_frame_alloc()};
    if (!packet || !frame) {
        return core::Result<SeekResult>::failure(
            resource_error("media.seek", "packet_or_frame"));
    }
    std::optional<VideoFrame> before;
    std::optional<VideoFrame> after;
    std::optional<core::TimeNs> keyframe_anchor;
    std::uint64_t decoded = 0;
    std::uint64_t packets = 0;
    const auto& probed = impl_->media_info.streams.at(static_cast<std::size_t>(*stream_index));
    const auto transform = probed.video ? probed.video->geometry.display_transform
                                        : std::optional<DisplayTransform>{};
    auto receive = [&]() -> std::optional<core::ErrorInfo> {
        while (!after) {
            const int received = avcodec_receive_frame(decoder.value().get(), frame.get());
            if (received == AVERROR(EAGAIN) || received == AVERROR_EOF) {
                return std::nullopt;
            }
            if (received < 0) {
                return media_error(core::ErrorCode::decode_failed,
                                   "media.seek.receive",
                                   {{"ffmpeg", ffmpeg_error_string(received)}});
            }
            if (++decoded > limits.max_frames) {
                return resource_error("media.seek", "max_frames");
            }
            const auto timestamp = frame_timestamp(*frame, av_stream);
            if (!timestamp) {
                return timestamp.error();
            }
            const auto mapped = map_presentation_time(timestamp.value(),
                                                      selection.presentation_origin,
                                                      core::RoundingMode::nearest_ties_to_even);
            if (!mapped) {
                return mapped.error();
            }
            if ((frame->flags & AV_FRAME_FLAG_KEY) != 0) {
                keyframe_anchor = mapped.value();
            }
            auto converted =
                convert_video(*frame, output, transform, limits.max_single_buffer_bytes);
            if (!converted) {
                return converted.error();
            }
            VideoFrame candidate;
            candidate.stream_key = request.selected_stream;
            candidate.time_ns = mapped.value();
            candidate.source_pts = timestamp.value();
            candidate.timestamp_origin = timestamp.value().origin;
            candidate.decode_ordinal = decoded - 1;
            candidate.key_frame = (frame->flags & AV_FRAME_FLAG_KEY) != 0;
            candidate.decode_had_errors = frame->decode_error_flags != 0;
            candidate.geometry.coded_width =
                static_cast<std::int32_t>(converted.value().width);
            candidate.geometry.coded_height =
                static_cast<std::int32_t>(converted.value().height);
            candidate.geometry.sample_aspect_ratio = Rational{1, 1};
            candidate.geometry.display_aspect_ratio =
                Rational{converted.value().width, converted.value().height};
            candidate.color = frame_color(*frame);
            candidate.color.pixel_format = "bgra";
            candidate.color.bit_depth = 8;
            candidate.color.range = detail::normalize_color_range(AVCOL_RANGE_JPEG);
            candidate.color.matrix = "rgb";
            candidate.pixel_format = "bgra";
            const auto bytes = static_cast<std::uint64_t>(converted.value().bytes.size());
            candidate.planes.push_back({0,
                                        static_cast<std::int64_t>(converted.value().width) * 4,
                                        converted.value().height,
                                        bytes});
            candidate.lease = BufferLease::from_bytes(
                std::move(converted.value().bytes),
                1,
                "seek:" + std::to_string(
                              next_buffer_id.fetch_add(1, std::memory_order_relaxed)));
            if (candidate.time_ns < request.target_time_ns) {
                before = std::move(candidate);
            } else {
                after = std::move(candidate);
            }
            av_frame_unref(frame.get());
        }
        return std::nullopt;
    };
    while (!after) {
        if (cancelled(cancellation)) {
            return core::Result<SeekResult>::failure(cancelled_error("media.seek"));
        }
        const int read = av_read_frame(format.value().get(), packet.get());
        if (read == AVERROR_EOF) {
            break;
        }
        if (read < 0) {
            return core::Result<SeekResult>::failure(
                media_error(core::ErrorCode::corrupt_media, "media.seek.read"));
        }
        if (++packets > limits.max_packets) {
            return core::Result<SeekResult>::failure(
                resource_error("media.seek", "max_packets"));
        }
        if (packet->stream_index == *stream_index) {
            const int sent = avcodec_send_packet(decoder.value().get(), packet.get());
            if (sent < 0) {
                return core::Result<SeekResult>::failure(
                    media_error(core::ErrorCode::decode_failed, "media.seek.send"));
            }
            if (const auto error = receive()) {
                return core::Result<SeekResult>::failure(*error);
            }
        }
        av_packet_unref(packet.get());
    }
    std::optional<VideoFrame> chosen;
    switch (request.mode) {
    case SeekMode::at_or_before:
        chosen = before ? std::move(before) : std::move(after);
        break;
    case SeekMode::at_or_after:
        chosen = std::move(after);
        break;
    case SeekMode::nearest:
        if (before && after) {
            const auto before_distance = request.target_time_ns - before->time_ns;
            const auto after_distance = after->time_ns - request.target_time_ns;
            chosen = before_distance <= after_distance ? std::move(before) : std::move(after);
        } else {
            chosen = before ? std::move(before) : std::move(after);
        }
        break;
    case SeekMode::exact_or_error:
        if (after && after->time_ns == request.target_time_ns) {
            chosen = std::move(after);
        }
        break;
    }
    if (!chosen) {
        return core::Result<SeekResult>::failure(
            media_error(core::ErrorCode::seek_unreachable, "media.seek.select"));
    }
    const auto error = core::checked_subtract(chosen->time_ns, request.target_time_ns);
    if (!error || error.value() < -request.tolerance_before_ns
        || error.value() > request.tolerance_after_ns) {
        return core::Result<SeekResult>::failure(
            media_error(core::ErrorCode::seek_unreachable, "media.seek.tolerance"));
    }
    SeekResult result;
    result.requested_time_ns = request.target_time_ns;
    result.actual_frame_time_ns = chosen->time_ns;
    result.actual_frame_duration_ns = chosen->duration_ns;
    result.keyframe_anchor_time_ns = keyframe_anchor;
    result.decoded_preroll_frames = decoded;
    result.error_ns = error.value();
    result.exact = error.value() == 0;
    result.stream_key = request.selected_stream;
    result.frame = std::move(*chosen);
    return core::Result<SeekResult>::success(std::move(result));
}

core::Result<VideoFrame> MediaSource::proxy_frame(
    const MediaSelection& selection,
    const StreamKey& stream,
    core::TimeNs target_time_ns,
    std::uint32_t max_width,
    std::uint32_t max_height,
    const core::CancellationToken* cancellation) const
{
    if (max_width == 0 || max_height == 0) {
        return core::Result<VideoFrame>::failure(
            validation_error(core::ErrorCode::invalid_dto, "media.proxy"));
    }
    const auto stream_index = find_stream_index(impl_->media_info, stream, StreamKind::video);
    if (!stream_index
        || std::ranges::find(selection.video.selected, stream)
            == selection.video.selected.end()) {
        return core::Result<VideoFrame>::failure(
            media_error(core::ErrorCode::stream_not_found, "media.proxy"));
    }
    const auto& info =
        impl_->media_info.streams.at(static_cast<std::size_t>(*stream_index));
    if (!info.video || info.video->geometry.coded_width <= 0
        || info.video->geometry.coded_height <= 0) {
        return core::Result<VideoFrame>::failure(
            media_error(core::ErrorCode::stream_type_mismatch, "media.proxy"));
    }
    double width = info.video->geometry.coded_width;
    double height = info.video->geometry.coded_height;
    if (info.video->geometry.display_transform
        && (info.video->geometry.display_transform->clockwise_rotation_degrees == 90
            || info.video->geometry.display_transform->clockwise_rotation_degrees == 270)) {
        std::swap(width, height);
    }
    const double scale = std::min({1.0, max_width / width, max_height / height});
    const auto target_width =
        std::max<std::uint32_t>(1, static_cast<std::uint32_t>(std::llround(width * scale)));
    const auto target_height =
        std::max<std::uint32_t>(1, static_cast<std::uint32_t>(std::llround(height * scale)));
    SeekRequest request{stream,
                        target_time_ns,
                        SeekMode::nearest,
                        std::numeric_limits<core::DurationNs>::max(),
                        std::numeric_limits<core::DurationNs>::max()};
    auto result = seek_video(selection,
                             request,
                             {target_width, target_height, true},
                             DecodeLimits{},
                             cancellation);
    if (!result) {
        return core::Result<VideoFrame>::failure(result.error());
    }
    return core::Result<VideoFrame>::success(std::move(result.value().frame));
}

core::Result<VideoFrame> MediaSource::thumbnail(
    const MediaSelection& selection,
    const StreamKey& stream,
    core::TimeNs target_time_ns,
    std::uint32_t width,
    std::uint32_t height,
    const core::CancellationToken* cancellation) const
{
    if (width == 0 || height == 0) {
        return core::Result<VideoFrame>::failure(
            validation_error(core::ErrorCode::invalid_dto, "media.thumbnail"));
    }
    SeekRequest request{stream,
                        target_time_ns,
                        SeekMode::nearest,
                        std::numeric_limits<core::DurationNs>::max(),
                        std::numeric_limits<core::DurationNs>::max()};
    auto result = seek_video(selection,
                             request,
                             {width, height, true},
                             DecodeLimits{},
                             cancellation);
    if (!result) {
        return core::Result<VideoFrame>::failure(result.error());
    }
    return core::Result<VideoFrame>::success(std::move(result.value().frame));
}

} // namespace space_rhythm::media
