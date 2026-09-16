#include <space_rhythm/media/playback_export.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <ranges>
#include <sstream>
#include <system_error>
#include <tuple>
#include <utility>

namespace space_rhythm::media::playback_export {
namespace {

std::atomic_uint64_t next_diagnostic_id{1};

core::ErrorInfo make_error(core::ErrorCategory category,
                           core::ErrorCode code,
                           std::string_view stage,
                           bool retryable = false,
                           std::map<std::string, std::string> context = {})
{
    const auto sequence = next_diagnostic_id.fetch_add(1, std::memory_order_relaxed);
    return core::ErrorInfo{core::schema_version,
                           category,
                           code,
                           std::string{stage},
                           "playback-export:" + std::to_string(sequence),
                           retryable,
                           std::string{core::to_string(code)},
                           std::move(context),
                           {}};
}

core::ErrorInfo invalid(std::string_view stage,
                        core::ErrorCode code = core::ErrorCode::invalid_dto)
{
    return make_error(core::ErrorCategory::validation, code, stage);
}

core::ErrorInfo cancelled(std::string_view stage)
{
    return make_error(core::ErrorCategory::cancelled,
                      core::ErrorCode::cancelled,
                      stage);
}

core::ErrorInfo internal_error(std::string_view stage,
                               std::map<std::string, std::string> context = {})
{
    return make_error(core::ErrorCategory::internal,
                      core::ErrorCode::internal_error,
                      stage,
                      false,
                      std::move(context));
}

bool is_sha256(std::string_view value)
{
    return value.size() == 64
        && std::ranges::all_of(value, [](unsigned char character) {
               return (character >= '0' && character <= '9')
                   || (character >= 'a' && character <= 'f');
           });
}

bool safe_job_id(std::string_view value)
{
    if (!core::validate_identifier(value)) {
        return false;
    }
    return std::ranges::all_of(value, [](unsigned char character) {
        return (character >= '0' && character <= '9')
            || (character >= 'A' && character <= 'Z')
            || (character >= 'a' && character <= 'z') || character == '-'
            || character == '_' || character == '.';
    });
}

std::string ffmpeg_error_string(int error)
{
    std::array<char, AV_ERROR_MAX_STRING_SIZE> buffer{};
    if (av_strerror(error, buffer.data(), buffer.size()) < 0) {
        return "ffmpeg_error_" + std::to_string(error);
    }
    return buffer.data();
}

std::string utf8_path(const std::filesystem::path& path)
{
    const auto bytes = path.u8string();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

core::ErrorInfo ffmpeg_error(std::string_view stage, int error)
{
    return make_error(core::ErrorCategory::media,
                      core::ErrorCode::internal_error,
                      stage,
                      false,
                      {{"ffmpegCode", std::to_string(error)},
                       {"ffmpegMessage", ffmpeg_error_string(error)}});
}

core::Result<core::TimeNs> monotonic_elapsed_ns(
    PreviewSynchronizer::MonotonicTimePoint begin,
    PreviewSynchronizer::MonotonicTimePoint end)
{
    const auto elapsed = end - begin;
    if (elapsed.count() < 0) {
        return core::Result<core::TimeNs>::failure(
            invalid("media.preview.monotonic", core::ErrorCode::timestamp_discontinuity));
    }
    if constexpr (sizeof(elapsed.count()) > sizeof(std::int64_t)) {
        if (elapsed.count() > std::numeric_limits<std::int64_t>::max()) {
            return core::Result<core::TimeNs>::failure(
                invalid("media.preview.monotonic", core::ErrorCode::time_overflow));
        }
    }
    return core::scale_ticks(
        static_cast<std::int64_t>(elapsed.count()),
        core::TimeBase{std::chrono::steady_clock::period::num,
                       std::chrono::steady_clock::period::den},
        core::RoundingMode::nearest_ties_to_even);
}

core::Result<core::DurationNs> absolute_difference(core::TimeNs left,
                                                   core::TimeNs right)
{
    return left >= right ? core::checked_subtract(left, right)
                         : core::checked_subtract(right, left);
}

std::optional<std::size_t> frame_at_or_before(
    const std::vector<ScheduledVideoFrame>& frames,
    core::TimeNs time_ns)
{
    const auto iterator = std::upper_bound(
        frames.begin(), frames.end(), time_ns, [](core::TimeNs time,
                                                  const ScheduledVideoFrame& frame) {
            return time < frame.time_ns;
        });
    if (iterator == frames.begin()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(frames.begin(), iterator) - 1);
}

std::string canonical_frozen_inputs(const ExportFreezeRequest& request)
{
    const auto& recipe = request.render_snapshot->recipe();
    std::ostringstream stream;
    stream << "schema=" << request.schema_version << '\n'
           << "contract=" << request.contract_version << '\n'
           << "job=" << request.job_id << '\n'
           << "timelineRevision=" << recipe.timeline_revision << '\n'
           << "snapshot=" << request.render_snapshot->id().value << '\n'
           << "recipe=" << recipe.recipe_id.value << '\n'
           << "media=" << request.media.source_fingerprint_sha256 << '\n'
           << "timeRange=" << request.time_range.start_ns << ','
           << request.time_range.end_ns << '\n'
           << "frameRate=" << request.frame_rate.numerator << '/'
           << request.frame_rate.denominator << '\n'
           << "seed=" << request.deterministic_seed << '\n'
           << "includeAudio=" << (request.include_audio ? 1 : 0) << '\n'
           << "audio="
           << audio::render::canonical_render_parameters(request.audio_parameters)
           << '\n';
    const auto append_selection = [&stream](std::string_view kind,
                                             const StreamSelectionResult& selection) {
        stream << kind << "Mode=" << static_cast<int>(selection.mode) << '\n';
        for (const auto& key : selection.selected) {
            stream << kind << "Stream=" << key.source_fingerprint_sha256 << ':'
                   << key.stream_index << '\n';
        }
    };
    append_selection("video", request.media.selection.video);
    append_selection("audio", request.media.selection.audio);
    for (const auto& hash : request.timbre_sha256) {
        stream << "timbre=" << hash << '\n';
    }
    return stream.str();
}

bool valid_selection(const StreamSelectionResult& selection,
                     std::string_view fingerprint)
{
    return std::ranges::all_of(selection.selected, [fingerprint](const StreamKey& key) {
        return key.source_fingerprint_sha256 == fingerprint && key.stream_index >= 0;
    });
}

template <typename Type, void (*FreeFunction)(Type**)>
struct FfmpegDeleter {
    void operator()(Type* value) const noexcept
    {
        if (value != nullptr) {
            FreeFunction(&value);
        }
    }
};

using CodecContextPtr =
    std::unique_ptr<AVCodecContext, FfmpegDeleter<AVCodecContext, avcodec_free_context>>;
using FramePtr = std::unique_ptr<AVFrame, FfmpegDeleter<AVFrame, av_frame_free>>;
using PacketPtr = std::unique_ptr<AVPacket, FfmpegDeleter<AVPacket, av_packet_free>>;

class FfmpegTestEncoder final : public ExportEncoder {
public:
    ~FfmpegTestEncoder() override { abort(); }

    [[nodiscard]] ExportFormatDescriptor format() const override
    {
        return ffmpeg_test_format();
    }

    core::Result<bool> open(const std::filesystem::path& temporary_path,
                            const FrozenExportSnapshot& snapshot) override
    {
        if (format_context_ != nullptr) {
            return core::Result<bool>::failure(invalid("media.export.open"));
        }
        const auto output_path = utf8_path(temporary_path);
        auto result = avformat_alloc_output_context2(
            &format_context_, nullptr, "nut", output_path.c_str());
        if (result < 0 || format_context_ == nullptr) {
            return core::Result<bool>::failure(
                ffmpeg_error("media.export.alloc-format", result));
        }

        auto video = open_video(snapshot.render_recipe());
        if (!video) {
            abort();
            return video;
        }
        if (snapshot.includes_audio()) {
            auto audio = open_audio(snapshot.audio_parameters());
            if (!audio) {
                abort();
                return audio;
            }
        }
        result = avio_open(&format_context_->pb, output_path.c_str(), AVIO_FLAG_WRITE);
        if (result < 0) {
            auto error = ffmpeg_error("media.export.open-io", result);
            abort();
            return core::Result<bool>::failure(std::move(error));
        }
        result = avformat_write_header(format_context_, nullptr);
        if (result < 0) {
            auto error = ffmpeg_error("media.export.write-header", result);
            abort();
            return core::Result<bool>::failure(std::move(error));
        }
        header_written_ = true;
        return core::Result<bool>::success(true);
    }

    core::Result<bool> write_video_frame(
        const rendering::RenderedFrame& frame) override
    {
        if (!header_written_ || !video_context_ || video_stream_ == nullptr) {
            return core::Result<bool>::failure(invalid("media.export.video-state"));
        }
        FramePtr av_frame{av_frame_alloc()};
        if (!av_frame) {
            return core::Result<bool>::failure(
                internal_error("media.export.video-frame-alloc"));
        }
        av_frame->format = video_context_->pix_fmt;
        av_frame->width = video_context_->width;
        av_frame->height = video_context_->height;
        av_frame->pts = static_cast<std::int64_t>(frame.frame_index);
        auto result = av_frame_get_buffer(av_frame.get(), 32);
        if (result < 0) {
            return core::Result<bool>::failure(
                ffmpeg_error("media.export.video-frame-buffer", result));
        }
        result = av_frame_make_writable(av_frame.get());
        if (result < 0) {
            return core::Result<bool>::failure(
                ffmpeg_error("media.export.video-frame-writable", result));
        }
        const auto row_bytes = static_cast<std::size_t>(frame.width_px) * 4U;
        for (std::uint32_t row = 0; row < frame.height_px; ++row) {
            const auto* source = frame.bytes.data()
                + static_cast<std::size_t>(frame.stride_bytes) * row;
            auto* destination = av_frame->data[0]
                + static_cast<std::ptrdiff_t>(av_frame->linesize[0]) * row;
            std::memcpy(destination, source, row_bytes);
        }
        return send_frame(video_context_.get(), video_stream_, av_frame.get(),
                          "media.export.video-encode");
    }

    core::Result<bool> write_audio_pcm(
        const audio::render::RenderedPcm& pcm) override
    {
        if (!header_written_ || !audio_context_ || audio_stream_ == nullptr) {
            return core::Result<bool>::failure(invalid("media.export.audio-state"));
        }
        constexpr std::uint64_t maximum_frame_samples = 4'096;
        std::uint64_t offset = 0;
        while (offset < pcm.frame_count) {
            const auto count = std::min(maximum_frame_samples, pcm.frame_count - offset);
            FramePtr av_frame{av_frame_alloc()};
            if (!av_frame) {
                return core::Result<bool>::failure(
                    internal_error("media.export.audio-frame-alloc"));
            }
            av_frame->format = audio_context_->sample_fmt;
            av_frame->sample_rate = audio_context_->sample_rate;
            av_frame->nb_samples = static_cast<int>(count);
            av_frame->pts = static_cast<std::int64_t>(pcm.first_frame + offset);
            auto result = av_channel_layout_copy(&av_frame->ch_layout,
                                                 &audio_context_->ch_layout);
            if (result < 0) {
                return core::Result<bool>::failure(
                    ffmpeg_error("media.export.audio-layout", result));
            }
            result = av_frame_get_buffer(av_frame.get(), 0);
            if (result < 0) {
                return core::Result<bool>::failure(
                    ffmpeg_error("media.export.audio-frame-buffer", result));
            }
            const auto channels = static_cast<std::size_t>(pcm.channel_count);
            const auto sample_offset = static_cast<std::size_t>(offset) * channels;
            const auto byte_count = static_cast<std::size_t>(count) * channels
                * sizeof(float);
            std::memcpy(av_frame->data[0],
                        pcm.interleaved_f32.data() + sample_offset,
                        byte_count);
            auto encoded = send_frame(audio_context_.get(), audio_stream_, av_frame.get(),
                                      "media.export.audio-encode");
            if (!encoded) {
                return encoded;
            }
            offset += count;
        }
        return core::Result<bool>::success(true);
    }

    core::Result<bool> finish() override
    {
        if (!header_written_) {
            return core::Result<bool>::failure(invalid("media.export.finish-state"));
        }
        auto video = send_frame(video_context_.get(), video_stream_, nullptr,
                                "media.export.video-drain");
        if (!video) {
            return video;
        }
        if (audio_context_) {
            auto audio = send_frame(audio_context_.get(), audio_stream_, nullptr,
                                    "media.export.audio-drain");
            if (!audio) {
                return audio;
            }
        }
        const auto result = av_write_trailer(format_context_);
        if (result < 0) {
            return core::Result<bool>::failure(
                ffmpeg_error("media.export.write-trailer", result));
        }
        header_written_ = false;
        close_io();
        return core::Result<bool>::success(true);
    }

    void abort() noexcept override
    {
        header_written_ = false;
        close_io();
        audio_context_.reset();
        video_context_.reset();
        audio_stream_ = nullptr;
        video_stream_ = nullptr;
        if (format_context_ != nullptr) {
            avformat_free_context(format_context_);
            format_context_ = nullptr;
        }
    }

private:
    core::Result<bool> open_video(const rendering::RenderRecipe& recipe)
    {
        const auto* codec = avcodec_find_encoder(AV_CODEC_ID_RAWVIDEO);
        if (codec == nullptr) {
            return core::Result<bool>::failure(
                ffmpeg_error("media.export.rawvideo-unavailable", AVERROR_ENCODER_NOT_FOUND));
        }
        video_stream_ = avformat_new_stream(format_context_, nullptr);
        video_context_.reset(avcodec_alloc_context3(codec));
        if (video_stream_ == nullptr || !video_context_) {
            return core::Result<bool>::failure(
                internal_error("media.export.video-context"));
        }
        video_context_->codec_id = AV_CODEC_ID_RAWVIDEO;
        video_context_->codec_type = AVMEDIA_TYPE_VIDEO;
        video_context_->width = static_cast<int>(recipe.output.width_px);
        video_context_->height = static_cast<int>(recipe.output.height_px);
        video_context_->pix_fmt = AV_PIX_FMT_RGBA;
        video_context_->time_base = AVRational{
            static_cast<int>(recipe.frame_rate.denominator),
            static_cast<int>(recipe.frame_rate.numerator)};
        video_context_->framerate = AVRational{
            static_cast<int>(recipe.frame_rate.numerator),
            static_cast<int>(recipe.frame_rate.denominator)};
        if ((format_context_->oformat->flags & AVFMT_GLOBALHEADER) != 0) {
            video_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        auto result = avcodec_open2(video_context_.get(), codec, nullptr);
        if (result < 0) {
            return core::Result<bool>::failure(
                ffmpeg_error("media.export.open-video-codec", result));
        }
        result = avcodec_parameters_from_context(video_stream_->codecpar,
                                                 video_context_.get());
        if (result < 0) {
            return core::Result<bool>::failure(
                ffmpeg_error("media.export.video-parameters", result));
        }
        video_stream_->time_base = video_context_->time_base;
        return core::Result<bool>::success(true);
    }

    core::Result<bool> open_audio(const audio::render::RenderParameters& parameters)
    {
        const auto* codec = avcodec_find_encoder(AV_CODEC_ID_PCM_F32LE);
        if (codec == nullptr) {
            return core::Result<bool>::failure(
                ffmpeg_error("media.export.pcm-unavailable", AVERROR_ENCODER_NOT_FOUND));
        }
        audio_stream_ = avformat_new_stream(format_context_, nullptr);
        audio_context_.reset(avcodec_alloc_context3(codec));
        if (audio_stream_ == nullptr || !audio_context_) {
            return core::Result<bool>::failure(
                internal_error("media.export.audio-context"));
        }
        audio_context_->codec_id = AV_CODEC_ID_PCM_F32LE;
        audio_context_->codec_type = AVMEDIA_TYPE_AUDIO;
        audio_context_->sample_fmt = AV_SAMPLE_FMT_FLT;
        audio_context_->sample_rate = static_cast<int>(parameters.sample_rate);
        audio_context_->time_base = AVRational{1,
                                               static_cast<int>(parameters.sample_rate)};
        av_channel_layout_default(&audio_context_->ch_layout,
                                  static_cast<int>(parameters.channel_count));
        if ((format_context_->oformat->flags & AVFMT_GLOBALHEADER) != 0) {
            audio_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        auto result = avcodec_open2(audio_context_.get(), codec, nullptr);
        if (result < 0) {
            return core::Result<bool>::failure(
                ffmpeg_error("media.export.open-audio-codec", result));
        }
        result = avcodec_parameters_from_context(audio_stream_->codecpar,
                                                 audio_context_.get());
        if (result < 0) {
            return core::Result<bool>::failure(
                ffmpeg_error("media.export.audio-parameters", result));
        }
        audio_stream_->time_base = audio_context_->time_base;
        return core::Result<bool>::success(true);
    }

    core::Result<bool> send_frame(AVCodecContext* context,
                                  AVStream* stream,
                                  AVFrame* frame,
                                  std::string_view stage)
    {
        auto result = avcodec_send_frame(context, frame);
        if (result < 0) {
            return core::Result<bool>::failure(ffmpeg_error(stage, result));
        }
        PacketPtr packet{av_packet_alloc()};
        if (!packet) {
            return core::Result<bool>::failure(internal_error(stage));
        }
        while (true) {
            result = avcodec_receive_packet(context, packet.get());
            if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
                break;
            }
            if (result < 0) {
                return core::Result<bool>::failure(ffmpeg_error(stage, result));
            }
            av_packet_rescale_ts(packet.get(), context->time_base, stream->time_base);
            packet->stream_index = stream->index;
            result = av_interleaved_write_frame(format_context_, packet.get());
            av_packet_unref(packet.get());
            if (result < 0) {
                return core::Result<bool>::failure(ffmpeg_error(stage, result));
            }
        }
        return core::Result<bool>::success(true);
    }

    void close_io() noexcept
    {
        if (format_context_ != nullptr && format_context_->pb != nullptr) {
            avio_closep(&format_context_->pb);
        }
    }

    AVFormatContext* format_context_{};
    AVStream* video_stream_{};
    AVStream* audio_stream_{};
    CodecContextPtr video_context_;
    CodecContextPtr audio_context_;
    bool header_written_{};
};

} // namespace

struct PreviewSynchronizer::Impl final {
    PreviewConfiguration configuration;
    PreviewClockMode clock_mode{PreviewClockMode::monotonic};
    PreviewState state{PreviewState::playing};
    MonotonicTimePoint monotonic_anchor;
    core::TimeNs anchor_time_ns{};
    std::uint64_t audio_anchor_frames{};
    std::uint64_t audio_played_frames{};
    std::optional<std::size_t> last_presented_index;
    bool force_present{};
    std::uint64_t total_dropped{};

    core::Result<core::TimeNs> current_playhead(MonotonicTimePoint now) const
    {
        if (state == PreviewState::paused) {
            return core::Result<core::TimeNs>::success(anchor_time_ns);
        }
        core::Result<core::TimeNs> elapsed =
            core::Result<core::TimeNs>::success(0);
        if (clock_mode == PreviewClockMode::audio_played_samples) {
            if (audio_played_frames < audio_anchor_frames
                || audio_played_frames - audio_anchor_frames
                    > static_cast<std::uint64_t>(
                        std::numeric_limits<std::int64_t>::max())) {
                return core::Result<core::TimeNs>::failure(
                    invalid("media.preview.audio-clock",
                            core::ErrorCode::timestamp_discontinuity));
            }
            elapsed = core::scale_ticks(
                static_cast<std::int64_t>(audio_played_frames - audio_anchor_frames),
                core::TimeBase{1, *configuration.audio_sample_rate},
                core::RoundingMode::nearest_ties_to_even);
        } else {
            elapsed = monotonic_elapsed_ns(monotonic_anchor, now);
        }
        if (!elapsed) {
            return elapsed;
        }
        auto head = core::checked_add(anchor_time_ns, elapsed.value());
        if (!head) {
            return head;
        }
        return core::Result<core::TimeNs>::success(
            std::min(head.value(), configuration.timeline_range.end_ns));
    }

    core::Result<PreviewUpdate> make_update(MonotonicTimePoint now)
    {
        auto head = current_playhead(now);
        if (!head) {
            return core::Result<PreviewUpdate>::failure(head.error());
        }
        PreviewUpdate update;
        update.clock_mode = clock_mode;
        update.state = state;
        update.playhead_time_ns = head.value();
        update.total_dropped_video_frames = total_dropped;
        const auto candidate = frame_at_or_before(configuration.video_frames,
                                                   head.value());
        if (!candidate.has_value()) {
            return core::Result<PreviewUpdate>::success(std::move(update));
        }

        bool should_present = force_present || !last_presented_index.has_value()
            || *candidate != *last_presented_index;
        if (last_presented_index.has_value() && *candidate > *last_presented_index + 1U) {
            update.dropped_video_frames = static_cast<std::uint64_t>(
                *candidate - *last_presented_index - 1U);
        } else if (!last_presented_index.has_value() && *candidate > 0U) {
            update.dropped_video_frames = static_cast<std::uint64_t>(*candidate);
        }
        if (should_present) {
            update.video_frame = configuration.video_frames[*candidate];
            last_presented_index = candidate;
            force_present = false;
        }
        total_dropped += update.dropped_video_frames;
        update.total_dropped_video_frames = total_dropped;
        auto lateness = core::checked_subtract(
            head.value(), configuration.video_frames[*candidate].time_ns);
        if (!lateness) {
            return core::Result<PreviewUpdate>::failure(lateness.error());
        }
        update.video_lateness_ns = lateness.value();
        if (update.dropped_video_frames > 0) {
            update.diagnostics.push_back(
                {"video-frames-dropped",
                 {{"count", std::to_string(update.dropped_video_frames)},
                  {"playheadTimeNs", std::to_string(head.value())},
                  {"frameTimeNs", std::to_string(
                       configuration.video_frames[*candidate].time_ns)}}});
        }
        if (update.video_lateness_ns > configuration.drift_warning_threshold_ns) {
            update.diagnostics.push_back(
                {"video-clock-drift",
                 {{"latenessNs", std::to_string(update.video_lateness_ns)},
                  {"thresholdNs",
                   std::to_string(configuration.drift_warning_threshold_ns)},
                  {"clockMode",
                   clock_mode == PreviewClockMode::audio_played_samples
                       ? "audio-played-samples"
                       : "monotonic"}}});
        }
        return core::Result<PreviewUpdate>::success(std::move(update));
    }
};

PreviewSynchronizer::PreviewSynchronizer(std::unique_ptr<Impl> implementation)
    : impl_(std::move(implementation))
{
}

PreviewSynchronizer::~PreviewSynchronizer() = default;
PreviewSynchronizer::PreviewSynchronizer(PreviewSynchronizer&&) noexcept = default;
PreviewSynchronizer& PreviewSynchronizer::operator=(PreviewSynchronizer&&) noexcept = default;

core::Result<std::unique_ptr<PreviewSynchronizer>> PreviewSynchronizer::create(
    PreviewConfiguration configuration,
    MonotonicTimePoint now)
{
    if (configuration.timeline_range.start_ns < 0
        || configuration.timeline_range.end_ns
            <= configuration.timeline_range.start_ns
        || configuration.initial_playhead_time_ns
            < configuration.timeline_range.start_ns
        || configuration.initial_playhead_time_ns
            > configuration.timeline_range.end_ns
        || configuration.drift_warning_threshold_ns < 0
        || (configuration.audio_sample_rate.has_value()
            && *configuration.audio_sample_rate == 0)
        || !std::ranges::is_sorted(
            configuration.video_frames,
            [](const ScheduledVideoFrame& left, const ScheduledVideoFrame& right) {
                return std::tie(left.time_ns, left.frame_index)
                    < std::tie(right.time_ns, right.frame_index);
            })) {
        return core::Result<std::unique_ptr<PreviewSynchronizer>>::failure(
            invalid("media.preview.configuration"));
    }
    for (std::size_t index = 0; index < configuration.video_frames.size(); ++index) {
        const auto& frame = configuration.video_frames[index];
        if (!configuration.timeline_range.contains(frame.time_ns)
            || (index > 0
                && configuration.video_frames[index - 1].frame_index
                    >= frame.frame_index)) {
            return core::Result<std::unique_ptr<PreviewSynchronizer>>::failure(
                invalid("media.preview.frames"));
        }
    }
    auto implementation = std::make_unique<Impl>();
    implementation->clock_mode = configuration.audio_sample_rate.has_value()
        ? PreviewClockMode::audio_played_samples
        : PreviewClockMode::monotonic;
    implementation->monotonic_anchor = now;
    implementation->anchor_time_ns = configuration.initial_playhead_time_ns;
    implementation->audio_anchor_frames = configuration.initial_audio_played_frames;
    implementation->audio_played_frames = configuration.initial_audio_played_frames;
    implementation->configuration = std::move(configuration);
    return core::Result<std::unique_ptr<PreviewSynchronizer>>::success(
        std::unique_ptr<PreviewSynchronizer>(
            new PreviewSynchronizer(std::move(implementation))));
}

core::Result<bool> PreviewSynchronizer::set_audio_played_frames(
    std::uint64_t played_frames)
{
    if (impl_->clock_mode != PreviewClockMode::audio_played_samples) {
        return core::Result<bool>::failure(invalid("media.preview.audio-clock"));
    }
    if (played_frames < impl_->audio_played_frames) {
        return core::Result<bool>::failure(
            invalid("media.preview.audio-clock",
                    core::ErrorCode::timestamp_discontinuity));
    }
    impl_->audio_played_frames = played_frames;
    return core::Result<bool>::success(true);
}

core::Result<PreviewUpdate> PreviewSynchronizer::tick(MonotonicTimePoint now)
{
    return impl_->make_update(now);
}

core::Result<PreviewUpdate> PreviewSynchronizer::pause(MonotonicTimePoint now)
{
    if (impl_->state == PreviewState::paused) {
        return impl_->make_update(now);
    }
    auto head = impl_->current_playhead(now);
    if (!head) {
        return core::Result<PreviewUpdate>::failure(head.error());
    }
    impl_->anchor_time_ns = head.value();
    impl_->state = PreviewState::paused;
    return impl_->make_update(now);
}

core::Result<bool> PreviewSynchronizer::resume(MonotonicTimePoint now)
{
    if (impl_->state == PreviewState::playing) {
        return core::Result<bool>::success(true);
    }
    impl_->monotonic_anchor = now;
    impl_->audio_anchor_frames = impl_->audio_played_frames;
    impl_->state = PreviewState::playing;
    return core::Result<bool>::success(true);
}

core::Result<PreviewUpdate> PreviewSynchronizer::seek(core::TimeNs target_time_ns,
                                                       MonotonicTimePoint now,
                                                       std::optional<std::uint64_t>
                                                           audio_played_frames)
{
    if (target_time_ns < impl_->configuration.timeline_range.start_ns
        || target_time_ns > impl_->configuration.timeline_range.end_ns
        || (audio_played_frames.has_value()
            && impl_->clock_mode != PreviewClockMode::audio_played_samples)) {
        return core::Result<PreviewUpdate>::failure(
            invalid("media.preview.seek", core::ErrorCode::seek_unreachable));
    }
    if (audio_played_frames.has_value()) {
        impl_->audio_played_frames = *audio_played_frames;
    }
    impl_->anchor_time_ns = target_time_ns;
    impl_->monotonic_anchor = now;
    impl_->audio_anchor_frames = impl_->audio_played_frames;
    impl_->last_presented_index = frame_at_or_before(
        impl_->configuration.video_frames, target_time_ns);
    impl_->force_present = impl_->last_presented_index.has_value();
    return impl_->make_update(now);
}

PreviewClockMode PreviewSynchronizer::clock_mode() const noexcept
{
    return impl_->clock_mode;
}

PreviewState PreviewSynchronizer::state() const noexcept
{
    return impl_->state;
}

core::Result<EventAlignmentReport> compare_event_positions(
    std::span<const core::TimeNs> preview_event_times_ns,
    std::span<const core::TimeNs> export_event_times_ns,
    rendering::FrameRate output_frame_rate)
{
    if (preview_event_times_ns.size() != export_event_times_ns.size()
        || output_frame_rate.numerator == 0 || output_frame_rate.denominator == 0
        || output_frame_rate.denominator
            > static_cast<std::uint32_t>(
                std::numeric_limits<std::int64_t>::max() / 1'000'000'000LL)) {
        return core::Result<EventAlignmentReport>::failure(
            invalid("media.export.event-alignment"));
    }
    auto frame_duration = core::scale_ticks(
        1,
        core::TimeBase{output_frame_rate.denominator,
                       output_frame_rate.numerator},
        core::RoundingMode::ceil);
    if (!frame_duration) {
        return core::Result<EventAlignmentReport>::failure(frame_duration.error());
    }
    EventAlignmentReport report;
    report.one_output_frame_ns = frame_duration.value();
    report.within_one_output_frame = true;
    report.absolute_errors_ns.reserve(preview_event_times_ns.size());
    for (std::size_t index = 0; index < preview_event_times_ns.size(); ++index) {
        auto difference = absolute_difference(preview_event_times_ns[index],
                                              export_event_times_ns[index]);
        if (!difference) {
            return core::Result<EventAlignmentReport>::failure(difference.error());
        }
        report.absolute_errors_ns.push_back(difference.value());
        report.maximum_error_ns = std::max(report.maximum_error_ns,
                                           difference.value());
        report.within_one_output_frame = report.within_one_output_frame
            && difference.value() <= report.one_output_frame_ns;
    }
    return core::Result<EventAlignmentReport>::success(std::move(report));
}

FrozenExportSnapshot::FrozenExportSnapshot(
    std::string job_id,
    std::shared_ptr<const rendering::RenderSnapshot> render_snapshot,
    rendering::RenderRecipe render_recipe,
    ExportMediaBinding media,
    audio::render::RenderParameters audio_parameters,
    std::vector<std::string> timbre_sha256,
    bool include_audio,
    std::string frozen_inputs_sha256)
    : job_id_(std::move(job_id)),
      render_snapshot_(std::move(render_snapshot)),
      render_recipe_(std::move(render_recipe)),
      media_(std::move(media)),
      audio_parameters_(std::move(audio_parameters)),
      timbre_sha256_(std::move(timbre_sha256)),
      include_audio_(include_audio),
      frozen_inputs_sha256_(std::move(frozen_inputs_sha256))
{
}

std::string_view FrozenExportSnapshot::job_id() const noexcept { return job_id_; }
const rendering::RenderSnapshot& FrozenExportSnapshot::render_snapshot() const noexcept
{
    return *render_snapshot_;
}
const rendering::RenderRecipe& FrozenExportSnapshot::render_recipe() const noexcept
{
    return render_recipe_;
}
const ExportMediaBinding& FrozenExportSnapshot::media() const noexcept { return media_; }
const audio::render::RenderParameters& FrozenExportSnapshot::audio_parameters() const noexcept
{
    return audio_parameters_;
}
const std::vector<std::string>& FrozenExportSnapshot::timbre_sha256() const noexcept
{
    return timbre_sha256_;
}
const core::TimeRange& FrozenExportSnapshot::time_range() const noexcept
{
    return render_recipe_.time_range;
}
const rendering::FrameRate& FrozenExportSnapshot::frame_rate() const noexcept
{
    return render_recipe_.frame_rate;
}
std::uint64_t FrozenExportSnapshot::deterministic_seed() const noexcept
{
    return render_recipe_.deterministic_seed;
}
bool FrozenExportSnapshot::includes_audio() const noexcept { return include_audio_; }
std::string_view FrozenExportSnapshot::frozen_inputs_sha256() const noexcept
{
    return frozen_inputs_sha256_;
}

core::Result<std::shared_ptr<const FrozenExportSnapshot>> freeze_export(
    ExportFreezeRequest request)
{
    if (request.schema_version != schema_version
        || request.contract_version != contract_version || !safe_job_id(request.job_id)
        || !request.render_snapshot
        || request.current_timeline_revision
            != request.render_snapshot->recipe().timeline_revision) {
        if (request.render_snapshot
            && request.current_timeline_revision
                != request.render_snapshot->recipe().timeline_revision) {
            return core::Result<std::shared_ptr<const FrozenExportSnapshot>>::failure(
                make_error(core::ErrorCategory::conflict,
                           core::ErrorCode::stale_revision,
                           "media.export.freeze-revision",
                           true));
        }
        return core::Result<std::shared_ptr<const FrozenExportSnapshot>>::failure(
            invalid("media.export.freeze"));
    }
    const auto& recipe = request.render_snapshot->recipe();
    if (recipe.time_range != request.time_range
        || recipe.frame_rate != request.frame_rate
        || recipe.deterministic_seed != request.deterministic_seed
        || request.render_snapshot->timeline().timeline_revision
            != recipe.timeline_revision
        || request.audio_parameters.schema_version != audio::render::schema_version
        || request.audio_parameters.contract_version != audio::render::contract_version
        || request.audio_parameters.algorithm_id != audio::render::algorithm_id
        || request.audio_parameters.algorithm_version != audio::render::algorithm_version
        || request.audio_parameters.backend_id != audio::render::backend_id
        || request.audio_parameters.backend_version != audio::render::backend_version
        || request.audio_parameters.parameter_set_id != audio::render::parameter_set_id
        || request.audio_parameters.parameter_set_version
            != audio::render::parameter_set_version
        || request.audio_parameters.deterministic_seed != request.deterministic_seed
        || request.audio_parameters.sample_rate != audio::render::render_sample_rate
        || (request.audio_parameters.channel_count != 1
            && request.audio_parameters.channel_count != 2)
        || !is_sha256(request.media.source_fingerprint_sha256)
        || !valid_selection(request.media.selection.video,
                            request.media.source_fingerprint_sha256)
        || !valid_selection(request.media.selection.audio,
                            request.media.source_fingerprint_sha256)
        || !std::ranges::all_of(request.timbre_sha256, is_sha256)) {
        return core::Result<std::shared_ptr<const FrozenExportSnapshot>>::failure(
            invalid("media.export.freeze-inputs"));
    }
    std::ranges::sort(request.timbre_sha256);
    if (std::adjacent_find(request.timbre_sha256.begin(),
                           request.timbre_sha256.end())
        != request.timbre_sha256.end()) {
        return core::Result<std::shared_ptr<const FrozenExportSnapshot>>::failure(
            invalid("media.export.freeze-timbres"));
    }
    const auto canonical = canonical_frozen_inputs(request);
    const auto bytes = std::as_bytes(std::span{canonical.data(), canonical.size()});
    const auto digest = audio::render::sha256_hex(bytes);
    auto frozen = std::shared_ptr<const FrozenExportSnapshot>(new FrozenExportSnapshot(
        std::move(request.job_id),
        std::move(request.render_snapshot),
        recipe,
        std::move(request.media),
        std::move(request.audio_parameters),
        std::move(request.timbre_sha256),
        request.include_audio,
        digest));
    return core::Result<std::shared_ptr<const FrozenExportSnapshot>>::success(
        std::move(frozen));
}

ExportFormatDescriptor ffmpeg_test_format()
{
    return {std::string{ffmpeg_test_format_id},
            true,
            "nut",
            "rawvideo-rgba8",
            "pcm-f32le"};
}

std::unique_ptr<ExportEncoder> make_ffmpeg_test_encoder()
{
    return std::make_unique<FfmpegTestEncoder>();
}

ExportSession::ExportSession(std::shared_ptr<const FrozenExportSnapshot> snapshot,
                             std::filesystem::path target_path,
                             std::filesystem::path temporary_path,
                             std::unique_ptr<ExportEncoder> encoder,
                             ExportFormatDescriptor format,
                             const core::CancellationToken* cancellation)
    : snapshot_(std::move(snapshot)),
      target_path_(std::move(target_path)),
      temporary_path_(std::move(temporary_path)),
      encoder_(std::move(encoder)),
      format_(std::move(format)),
      cancellation_(cancellation != nullptr
                        ? std::optional<core::CancellationToken>{*cancellation}
                        : std::nullopt)
{
}

core::Result<std::unique_ptr<ExportSession>> ExportSession::create(
    std::shared_ptr<const FrozenExportSnapshot> snapshot,
    std::filesystem::path target_path,
    std::unique_ptr<ExportEncoder> encoder,
    const core::CancellationToken* cancellation)
{
    if (!snapshot || target_path.empty() || !target_path.has_filename() || !encoder) {
        return core::Result<std::unique_ptr<ExportSession>>::failure(
            invalid("media.export.session"));
    }
    if (cancellation != nullptr && cancellation->is_cancelled()) {
        return core::Result<std::unique_ptr<ExportSession>>::failure(
            cancelled("media.export.create"));
    }
    const auto format = encoder->format();
    if (format != ffmpeg_test_format() || !format.test_only) {
        return core::Result<std::unique_ptr<ExportSession>>::failure(
            make_error(core::ErrorCategory::compatibility,
                       core::ErrorCode::unsupported_feature,
                       "media.export.format"));
    }
    std::error_code error;
    auto absolute_target = std::filesystem::absolute(target_path, error).lexically_normal();
    if (error || absolute_target.parent_path().empty()
        || !std::filesystem::is_directory(absolute_target.parent_path(), error) || error) {
        return core::Result<std::unique_ptr<ExportSession>>::failure(
            invalid("media.export.target"));
    }
    const auto temporary_name = absolute_target.filename().wstring() + L"."
        + std::filesystem::path{std::string{snapshot->job_id()}}.wstring()
        + L".space-rhythm-export.tmp";
    const auto temporary_path = absolute_target.parent_path() / temporary_name;
    if (std::filesystem::exists(temporary_path, error) || error) {
        return core::Result<std::unique_ptr<ExportSession>>::failure(
            make_error(core::ErrorCategory::conflict,
                       core::ErrorCode::history_conflict,
                       "media.export.temporary-exists"));
    }
    auto opened = encoder->open(temporary_path, *snapshot);
    if (!opened) {
        encoder->abort();
        std::filesystem::remove(temporary_path, error);
        return core::Result<std::unique_ptr<ExportSession>>::failure(opened.error());
    }
    if (cancellation != nullptr && cancellation->is_cancelled()) {
        encoder->abort();
        std::filesystem::remove(temporary_path, error);
        return core::Result<std::unique_ptr<ExportSession>>::failure(
            cancelled("media.export.create"));
    }
    return core::Result<std::unique_ptr<ExportSession>>::success(
        std::unique_ptr<ExportSession>(new ExportSession(std::move(snapshot),
                                                         std::move(absolute_target),
                                                         temporary_path,
                                                         std::move(encoder),
                                                         format,
                                                         cancellation)));
}

ExportSession::~ExportSession()
{
    if (state_ == ExportSessionState::open) {
        encoder_->abort();
        cleanup_temporary();
    }
}

core::Result<bool> ExportSession::write_video_frame(
    const rendering::RenderedFrame& frame)
{
    if (state_ != ExportSessionState::open) {
        return core::Result<bool>::failure(invalid("media.export.video-state"));
    }
    if (is_cancelled()) {
        cancel();
        return core::Result<bool>::failure(cancelled("media.export.video-cancelled"));
    }
    if (frame.frame_index != next_video_frame_) {
        return fail_and_cleanup(
            invalid("media.export.video-sequence", core::ErrorCode::timestamp_mismatch));
    }
    auto validated = rendering::validate_rendered_frame(
        frame, snapshot_->render_snapshot());
    if (!validated) {
        return fail_and_cleanup(validated.error());
    }
    auto written = encoder_->write_video_frame(frame);
    if (!written) {
        return fail_and_cleanup(written.error());
    }
    ++next_video_frame_;
    return core::Result<bool>::success(true);
}

core::Result<bool> ExportSession::write_audio_pcm(
    const audio::render::RenderedPcm& pcm)
{
    if (state_ != ExportSessionState::open || !snapshot_->includes_audio()) {
        return core::Result<bool>::failure(invalid("media.export.audio-state"));
    }
    if (is_cancelled()) {
        cancel();
        return core::Result<bool>::failure(cancelled("media.export.audio-cancelled"));
    }
    const auto& parameters = snapshot_->audio_parameters();
    if (pcm.sample_rate != parameters.sample_rate
        || pcm.channel_count != parameters.channel_count
        || pcm.first_frame != next_audio_frame_
        || pcm.frame_count
            > std::numeric_limits<std::size_t>::max() / pcm.channel_count
        || pcm.interleaved_f32.size()
            != static_cast<std::size_t>(pcm.frame_count * pcm.channel_count)
        || !std::ranges::all_of(pcm.interleaved_f32, [](float sample) {
               return std::isfinite(sample) && sample >= -1.0F && sample <= 1.0F;
           })) {
        return fail_and_cleanup(
            invalid("media.export.audio-pcm", core::ErrorCode::invalid_pcm_buffer));
    }
    auto written = encoder_->write_audio_pcm(pcm);
    if (!written) {
        return fail_and_cleanup(written.error());
    }
    if (pcm.frame_count > std::numeric_limits<std::uint64_t>::max() - next_audio_frame_) {
        return fail_and_cleanup(
            invalid("media.export.audio-sequence", core::ErrorCode::time_overflow));
    }
    next_audio_frame_ += pcm.frame_count;
    return core::Result<bool>::success(true);
}

core::Result<bool> ExportSession::finish()
{
    if (state_ != ExportSessionState::open) {
        return core::Result<bool>::failure(invalid("media.export.finish-state"));
    }
    if (is_cancelled()) {
        cancel();
        return core::Result<bool>::failure(cancelled("media.export.finish-cancelled"));
    }
    if (rendering::frame_time_ns(snapshot_->render_recipe(), next_video_frame_)) {
        return fail_and_cleanup(invalid("media.export.video-incomplete"));
    }
    if (snapshot_->includes_audio()) {
        auto expected_frames = time_ns_to_sample_index(
            snapshot_->time_range().end_ns,
            snapshot_->time_range().start_ns,
            snapshot_->audio_parameters().sample_rate,
            core::RoundingMode::ceil);
        if (!expected_frames || expected_frames.value() < 0
            || static_cast<std::uint64_t>(expected_frames.value())
                != next_audio_frame_) {
            return fail_and_cleanup(invalid("media.export.audio-incomplete"));
        }
    }
    auto finished = encoder_->finish();
    if (!finished) {
        return fail_and_cleanup(finished.error());
    }
    if (!MoveFileExW(temporary_path_.c_str(),
                     target_path_.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const auto win32 = GetLastError();
        return fail_and_cleanup(internal_error(
            "media.export.commit", {{"win32Error", std::to_string(win32)}}));
    }
    state_ = ExportSessionState::committed;
    return core::Result<bool>::success(true);
}

void ExportSession::cancel() noexcept
{
    if (state_ != ExportSessionState::open) {
        return;
    }
    encoder_->abort();
    cleanup_temporary();
    state_ = ExportSessionState::cancelled;
}

ExportSessionState ExportSession::state() const noexcept { return state_; }
const std::filesystem::path& ExportSession::target_path() const noexcept
{
    return target_path_;
}
const std::filesystem::path& ExportSession::temporary_path() const noexcept
{
    return temporary_path_;
}
const ExportFormatDescriptor& ExportSession::format() const noexcept { return format_; }

core::Result<bool> ExportSession::fail_and_cleanup(core::ErrorInfo error)
{
    if (state_ == ExportSessionState::open) {
        encoder_->abort();
        cleanup_temporary();
        state_ = ExportSessionState::failed;
    }
    return core::Result<bool>::failure(std::move(error));
}

bool ExportSession::is_cancelled() const noexcept
{
    return cancellation_.has_value() && cancellation_->is_cancelled();
}

void ExportSession::cleanup_temporary() noexcept
{
    std::error_code ignored;
    std::filesystem::remove(temporary_path_, ignored);
}

} // namespace space_rhythm::media::playback_export
