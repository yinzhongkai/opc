#pragma once

#include <space_rhythm/core/timeline.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace space_rhythm::media {

inline constexpr std::string_view contract_version{"0.1.0"};
inline constexpr std::uint32_t schema_version = 1;

enum class StreamKind { video, audio, subtitle, data, attachment, unknown };
enum class RateMode { constant, variable, unknown };
enum class TimestampOrigin {
    packet_pts,
    packet_dts,
    frame_pts,
    best_effort,
    stream_start,
    format_start,
    decoded_first_presentation,
    synthesized_duration,
};
enum class SelectionMode {
    none,
    explicit_stream,
    required_default_then_lowest_index,
    optional_default_then_lowest_index,
    all,
};
enum class PublishResult { accepted, would_block, closed, cancelled };
enum class ChannelState { created, open, draining, ended, failed, cancelled, closed };
enum class SeekMode { at_or_before, at_or_after, nearest, exact_or_error };

struct Diagnostic {
    std::string code;
    std::map<std::string, std::string> context;
};

struct RationalTimestamp {
    std::int64_t ticks{};
    core::TimeBase time_base;
    TimestampOrigin origin{TimestampOrigin::frame_pts};
};

struct RationalDuration {
    std::int64_t ticks{};
    core::TimeBase time_base;
};

struct Rational {
    std::int64_t numerator{};
    std::int64_t denominator{};

    bool operator==(const Rational&) const = default;
};

struct PresentationOrigin {
    RationalTimestamp timestamp;
    bool provisional{false};
};

struct StreamKey {
    std::string source_fingerprint_sha256;
    std::int32_t stream_index{-1};

    bool operator==(const StreamKey&) const = default;
};

struct DisplayTransform {
    std::int32_t clockwise_rotation_degrees{};
    bool mirror_horizontal{false};
    bool mirror_vertical{false};
    std::string source;
};

struct DisplayGeometry {
    std::int32_t coded_width{};
    std::int32_t coded_height{};
    std::int32_t crop_top{};
    std::int32_t crop_bottom{};
    std::int32_t crop_left{};
    std::int32_t crop_right{};
    std::optional<Rational> sample_aspect_ratio;
    std::optional<DisplayTransform> display_transform;
    std::optional<Rational> display_aspect_ratio;
};

struct ColorDescription {
    std::string pixel_format{"unknown"};
    std::int32_t bit_depth{};
    std::string range{"unknown"};
    std::string primaries{"unknown"};
    std::string transfer{"unknown"};
    std::string matrix{"unknown"};
    std::string chroma_location{"unknown"};
    bool assumed{false};
};

struct VideoFormat {
    DisplayGeometry geometry;
    ColorDescription color;
};

struct AudioFormat {
    std::uint32_t sample_rate{};
    std::string sample_format{"unknown"};
    std::string channel_layout{"unknown"};
    bool planar{false};
};

struct StreamInfo {
    StreamKey key;
    std::optional<std::int64_t> container_stream_id;
    StreamKind kind{StreamKind::unknown};
    bool default_disposition{false};
    bool forced_disposition{false};
    bool attached_picture{false};
    std::optional<std::string> language;
    std::string codec_name{"unknown"};
    bool decodable{false};
    core::TimeBase time_base;
    std::optional<std::int64_t> start_pts;
    std::optional<std::int64_t> duration_ticks;
    RateMode rate_mode{RateMode::unknown};
    std::optional<Rational> average_frame_rate;
    std::optional<Rational> nominal_frame_rate;
    std::optional<VideoFormat> video;
    std::optional<AudioFormat> audio;
};

struct FfmpegBuildInfo {
    std::string ffmpeg_version;
    std::string build_configuration;
    std::string build_configuration_sha256;
    std::string license;
    std::map<std::string, std::string> library_versions;
};

struct MediaInfo {
    std::uint32_t schema_version{media::schema_version};
    std::string media_contract_version{contract_version};
    std::string source_fingerprint_sha256;
    FfmpegBuildInfo probe_implementation;
    std::vector<std::string> format_names;
    std::optional<RationalTimestamp> format_start;
    std::optional<RationalDuration> duration;
    std::string duration_evidence{"unknown"};
    std::vector<StreamInfo> streams;
    std::vector<Diagnostic> diagnostics;
};

struct ProbeLimits {
    std::uint64_t max_source_bytes{1ULL << 34U};
    std::uint64_t max_probe_bytes{8ULL << 20U};
    std::int64_t max_analyze_duration_us{5'000'000};
    std::uint32_t max_streams{64};
    std::uint32_t timeout_ms{10'000};
};

struct StreamSelectionRequest {
    SelectionMode mode{SelectionMode::none};
    std::optional<StreamKey> explicit_key;
    bool allow_attached_picture{false};
};

struct StreamSelectionResult {
    SelectionMode mode{SelectionMode::none};
    std::vector<StreamKey> selected;
    std::vector<StreamKey> candidates;
    std::string reason;
};

struct MediaSelection {
    StreamSelectionResult video;
    StreamSelectionResult audio;
    PresentationOrigin presentation_origin;
    std::vector<Diagnostic> diagnostics;
};

class BufferLease final {
public:
    BufferLease() = default;
    static BufferLease from_bytes(std::vector<std::byte> bytes,
                                  std::uint64_t format_epoch,
                                  std::string buffer_id);

    [[nodiscard]] std::span<const std::byte> bytes() const noexcept;
    [[nodiscard]] std::uint64_t byte_size() const noexcept;
    [[nodiscard]] std::uint64_t format_epoch() const noexcept;
    [[nodiscard]] std::string_view buffer_id() const noexcept;
    [[nodiscard]] explicit operator bool() const noexcept;

private:
    struct State;
    explicit BufferLease(std::shared_ptr<const State> state);
    [[nodiscard]] BufferLease with_lifetime_guard(std::shared_ptr<void> guard) const;
    std::shared_ptr<const State> state_;

    friend class BoundedMediaQueue;
};

struct PlaneView {
    std::uint64_t offset_bytes{};
    std::int64_t row_bytes{};
    std::uint32_t rows{};
    std::uint64_t valid_bytes{};
};

struct VideoFrame {
    StreamKey stream_key;
    core::TimeNs time_ns{};
    std::optional<core::DurationNs> duration_ns;
    std::optional<RationalTimestamp> source_pts;
    std::optional<RationalTimestamp> source_dts;
    TimestampOrigin timestamp_origin{TimestampOrigin::frame_pts};
    std::uint64_t decode_ordinal{};
    bool key_frame{false};
    bool decode_had_errors{false};
    DisplayGeometry geometry;
    ColorDescription color;
    std::string pixel_format;
    std::vector<PlaneView> planes;
    BufferLease lease;
};

struct PcmBuffer {
    StreamKey stream_key;
    core::TimeNs time_ns{};
    core::DurationNs duration_ns{};
    std::string segment_id;
    std::int64_t first_sample_index{};
    std::uint64_t sample_count{};
    std::uint32_t sample_rate{};
    std::string sample_format;
    std::string channel_layout;
    bool planar{false};
    std::vector<PlaneView> planes;
    BufferLease lease;
};

using MediaBuffer = std::variant<VideoFrame, PcmBuffer>;

class BoundedMediaQueue final {
public:
    BoundedMediaQueue(std::uint64_t max_items, std::uint64_t max_bytes);
    ~BoundedMediaQueue();
    BoundedMediaQueue(const BoundedMediaQueue&) = delete;
    BoundedMediaQueue& operator=(const BoundedMediaQueue&) = delete;

    PublishResult publish(VideoFrame frame);
    PublishResult publish(PcmBuffer pcm);
    [[nodiscard]] std::optional<MediaBuffer> try_take();
    void drain() noexcept;
    void cancel() noexcept;
    void close() noexcept;
    [[nodiscard]] ChannelState state() const noexcept;
    [[nodiscard]] std::uint64_t queued_items() const noexcept;
    [[nodiscard]] std::uint64_t outstanding_bytes() const noexcept;

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

struct VideoOutputSpec {
    std::uint32_t width{};
    std::uint32_t height{};
    bool apply_display_transform{false};
};

struct AudioOutputSpec {
    std::uint32_t sample_rate{};
    std::uint32_t channels{1};
};

struct DecodeLimits {
    std::uint64_t max_packets{1'000'000};
    std::uint64_t max_frames{1'000'000};
    std::uint64_t max_output_bytes{1ULL << 34U};
    std::uint64_t max_single_buffer_bytes{256ULL << 20U};
    std::uint32_t decoder_threads{2};
};

struct FormatChanged {
    std::uint64_t format_epoch{};
    StreamKey stream_key;
    std::optional<VideoFormat> video;
    std::optional<AudioFormat> audio;
};

struct DecodeSummary {
    std::uint64_t packets_read{};
    std::uint64_t frames_published{};
    std::uint64_t samples_published{};
    std::uint64_t bytes_published{};
    std::uint64_t peak_single_buffer_bytes{};
    PublishResult terminal_publish_result{PublishResult::accepted};
    bool end_of_stream{false};
    std::vector<Diagnostic> diagnostics;
};

struct VideoCallbacks {
    std::function<PublishResult(const FormatChanged&)> on_format_changed;
    std::function<PublishResult(VideoFrame)> on_frame;
};

struct AudioCallbacks {
    std::function<PublishResult(const FormatChanged&)> on_format_changed;
    std::function<PublishResult(PcmBuffer)> on_pcm;
};

struct SeekRequest {
    StreamKey selected_stream;
    core::TimeNs target_time_ns{};
    SeekMode mode{SeekMode::nearest};
    core::DurationNs tolerance_before_ns{};
    core::DurationNs tolerance_after_ns{};
};

struct SeekResult {
    core::TimeNs requested_time_ns{};
    core::TimeNs actual_frame_time_ns{};
    std::optional<core::DurationNs> actual_frame_duration_ns;
    std::optional<core::TimeNs> keyframe_anchor_time_ns;
    std::uint64_t decoded_preroll_frames{};
    core::TimeNs error_ns{};
    bool exact{false};
    StreamKey stream_key;
    VideoFrame frame;
};

core::Result<FfmpegBuildInfo> query_ffmpeg_build_info();
core::Result<core::TimeNs> map_presentation_time(
    const RationalTimestamp& timestamp,
    const PresentationOrigin& origin,
    core::RoundingMode rounding = core::RoundingMode::nearest_ties_to_even);
core::Result<core::TimeNs> sample_index_to_time_ns(
    std::int64_t sample_index,
    std::uint32_t sample_rate,
    core::TimeNs origin_time_ns,
    core::RoundingMode rounding = core::RoundingMode::nearest_ties_to_even);
core::Result<std::int64_t> time_ns_to_sample_index(
    core::TimeNs time_ns,
    core::TimeNs origin_time_ns,
    std::uint32_t sample_rate,
    core::RoundingMode rounding);

class MediaSource final {
public:
    static core::Result<std::shared_ptr<MediaSource>> open(
        std::filesystem::path path,
        ProbeLimits limits = {},
        const core::CancellationToken* cancellation = nullptr);

    ~MediaSource();
    MediaSource(const MediaSource&) = delete;
    MediaSource& operator=(const MediaSource&) = delete;

    [[nodiscard]] const MediaInfo& info() const noexcept;
    core::Result<MediaSelection> select(const StreamSelectionRequest& video,
                                        const StreamSelectionRequest& audio,
                                        const core::CancellationToken* cancellation = nullptr) const;
    core::Result<DecodeSummary> decode_video(
        const MediaSelection& selection,
        const StreamKey& stream,
        const VideoOutputSpec& output,
        const DecodeLimits& limits,
        const VideoCallbacks& callbacks,
        const core::CancellationToken* cancellation = nullptr) const;
    core::Result<DecodeSummary> decode_audio(
        const MediaSelection& selection,
        const StreamKey& stream,
        const AudioOutputSpec& output,
        const DecodeLimits& limits,
        const AudioCallbacks& callbacks,
        const core::CancellationToken* cancellation = nullptr) const;
    core::Result<SeekResult> seek_video(
        const MediaSelection& selection,
        const SeekRequest& request,
        const VideoOutputSpec& output,
        const DecodeLimits& limits,
        const core::CancellationToken* cancellation = nullptr) const;
    core::Result<VideoFrame> proxy_frame(
        const MediaSelection& selection,
        const StreamKey& stream,
        core::TimeNs target_time_ns,
        std::uint32_t max_width,
        std::uint32_t max_height,
        const core::CancellationToken* cancellation = nullptr) const;
    core::Result<VideoFrame> thumbnail(
        const MediaSelection& selection,
        const StreamKey& stream,
        core::TimeNs target_time_ns,
        std::uint32_t width,
        std::uint32_t height,
        const core::CancellationToken* cancellation = nullptr) const;

private:
    struct Impl;
    explicit MediaSource(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

} // namespace space_rhythm::media
