#pragma once

#include <space_rhythm/audio/render.hpp>
#include <space_rhythm/core/timeline.hpp>
#include <space_rhythm/media/media.hpp>
#include <space_rhythm/rendering/render_contract.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace space_rhythm::media::playback_export {

inline constexpr std::uint32_t schema_version = 1;
inline constexpr std::string_view contract_version{"0.1.0"};
inline constexpr std::string_view ffmpeg_test_format_id{
    "space-rhythm.test.nut.raw-rgba-pcm-f32le"};

enum class PreviewClockMode { audio_played_samples, monotonic };
enum class PreviewState { playing, paused };

struct ScheduledVideoFrame {
    rendering::FrameIndex frame_index{};
    core::TimeNs time_ns{};

    bool operator==(const ScheduledVideoFrame&) const = default;
};

struct PreviewConfiguration {
    core::TimeRange timeline_range;
    core::TimeNs initial_playhead_time_ns{};
    std::vector<ScheduledVideoFrame> video_frames;
    std::optional<std::uint32_t> audio_sample_rate;
    std::uint64_t initial_audio_played_frames{};
    core::DurationNs drift_warning_threshold_ns{50'000'000};
};

struct PreviewDiagnostic {
    std::string code;
    std::map<std::string, std::string> context;

    bool operator==(const PreviewDiagnostic&) const = default;
};

struct PreviewUpdate {
    PreviewClockMode clock_mode{PreviewClockMode::monotonic};
    PreviewState state{PreviewState::playing};
    core::TimeNs playhead_time_ns{};
    std::optional<ScheduledVideoFrame> video_frame;
    std::uint64_t dropped_video_frames{};
    std::uint64_t total_dropped_video_frames{};
    core::DurationNs video_lateness_ns{};
    std::vector<PreviewDiagnostic> diagnostics;
};

class PreviewSynchronizer final {
public:
    using MonotonicTimePoint = std::chrono::steady_clock::time_point;

    static core::Result<std::unique_ptr<PreviewSynchronizer>> create(
        PreviewConfiguration configuration,
        MonotonicTimePoint now);

    ~PreviewSynchronizer();
    PreviewSynchronizer(const PreviewSynchronizer&) = delete;
    PreviewSynchronizer& operator=(const PreviewSynchronizer&) = delete;
    PreviewSynchronizer(PreviewSynchronizer&&) noexcept;
    PreviewSynchronizer& operator=(PreviewSynchronizer&&) noexcept;

    core::Result<bool> set_audio_played_frames(std::uint64_t played_frames);
    core::Result<PreviewUpdate> tick(MonotonicTimePoint now);
    core::Result<PreviewUpdate> pause(MonotonicTimePoint now);
    core::Result<bool> resume(MonotonicTimePoint now);
    core::Result<PreviewUpdate> seek(core::TimeNs target_time_ns,
                                     MonotonicTimePoint now,
                                     std::optional<std::uint64_t> audio_played_frames =
                                         std::nullopt);

    [[nodiscard]] PreviewClockMode clock_mode() const noexcept;
    [[nodiscard]] PreviewState state() const noexcept;

private:
    struct Impl;
    explicit PreviewSynchronizer(std::unique_ptr<Impl> implementation);
    std::unique_ptr<Impl> impl_;
};

struct EventAlignmentReport {
    core::DurationNs one_output_frame_ns{};
    core::DurationNs maximum_error_ns{};
    std::vector<core::DurationNs> absolute_errors_ns;
    bool within_one_output_frame{false};
};

core::Result<EventAlignmentReport> compare_event_positions(
    std::span<const core::TimeNs> preview_event_times_ns,
    std::span<const core::TimeNs> export_event_times_ns,
    rendering::FrameRate output_frame_rate);

struct ExportMediaBinding {
    std::string source_fingerprint_sha256;
    MediaSelection selection;
};

struct ExportFreezeRequest {
    std::uint32_t schema_version{playback_export::schema_version};
    std::string contract_version{playback_export::contract_version};
    std::string job_id;
    core::TimelineRevision current_timeline_revision{};
    std::shared_ptr<const rendering::RenderSnapshot> render_snapshot;
    ExportMediaBinding media;
    audio::render::RenderParameters audio_parameters;
    std::vector<std::string> timbre_sha256;
    core::TimeRange time_range;
    rendering::FrameRate frame_rate;
    std::uint64_t deterministic_seed{};
    bool include_audio{true};
};

class FrozenExportSnapshot final {
public:
    [[nodiscard]] std::string_view job_id() const noexcept;
    [[nodiscard]] const rendering::RenderSnapshot& render_snapshot() const noexcept;
    [[nodiscard]] const rendering::RenderRecipe& render_recipe() const noexcept;
    [[nodiscard]] const ExportMediaBinding& media() const noexcept;
    [[nodiscard]] const audio::render::RenderParameters& audio_parameters() const noexcept;
    [[nodiscard]] const std::vector<std::string>& timbre_sha256() const noexcept;
    [[nodiscard]] const core::TimeRange& time_range() const noexcept;
    [[nodiscard]] const rendering::FrameRate& frame_rate() const noexcept;
    [[nodiscard]] std::uint64_t deterministic_seed() const noexcept;
    [[nodiscard]] bool includes_audio() const noexcept;
    [[nodiscard]] std::string_view frozen_inputs_sha256() const noexcept;

private:
    FrozenExportSnapshot(std::string job_id,
                         std::shared_ptr<const rendering::RenderSnapshot> render_snapshot,
                         rendering::RenderRecipe render_recipe,
                         ExportMediaBinding media,
                         audio::render::RenderParameters audio_parameters,
                         std::vector<std::string> timbre_sha256,
                         bool include_audio,
                         std::string frozen_inputs_sha256);

    std::string job_id_;
    std::shared_ptr<const rendering::RenderSnapshot> render_snapshot_;
    rendering::RenderRecipe render_recipe_;
    ExportMediaBinding media_;
    audio::render::RenderParameters audio_parameters_;
    std::vector<std::string> timbre_sha256_;
    bool include_audio_{};
    std::string frozen_inputs_sha256_;

    friend core::Result<std::shared_ptr<const FrozenExportSnapshot>> freeze_export(
        ExportFreezeRequest request);
};

core::Result<std::shared_ptr<const FrozenExportSnapshot>> freeze_export(
    ExportFreezeRequest request);

struct ExportFormatDescriptor {
    std::string id;
    bool test_only{true};
    std::string container;
    std::string video_codec;
    std::string audio_codec;

    bool operator==(const ExportFormatDescriptor&) const = default;
};

[[nodiscard]] ExportFormatDescriptor ffmpeg_test_format();

class ExportEncoder {
public:
    virtual ~ExportEncoder() = default;
    [[nodiscard]] virtual ExportFormatDescriptor format() const = 0;
    virtual core::Result<bool> open(
        const std::filesystem::path& temporary_path,
        const FrozenExportSnapshot& snapshot) = 0;
    virtual core::Result<bool> write_video_frame(
        const rendering::RenderedFrame& frame) = 0;
    virtual core::Result<bool> write_audio_pcm(
        const audio::render::RenderedPcm& pcm) = 0;
    virtual core::Result<bool> finish() = 0;
    virtual void abort() noexcept = 0;
};

[[nodiscard]] std::unique_ptr<ExportEncoder> make_ffmpeg_test_encoder();

enum class ExportSessionState { open, committed, cancelled, failed };

class ExportSession final {
public:
    static core::Result<std::unique_ptr<ExportSession>> create(
        std::shared_ptr<const FrozenExportSnapshot> snapshot,
        std::filesystem::path target_path,
        std::unique_ptr<ExportEncoder> encoder,
        const core::CancellationToken* cancellation = nullptr);

    ~ExportSession();
    ExportSession(const ExportSession&) = delete;
    ExportSession& operator=(const ExportSession&) = delete;
    ExportSession(ExportSession&&) = delete;
    ExportSession& operator=(ExportSession&&) = delete;

    core::Result<bool> write_video_frame(const rendering::RenderedFrame& frame);
    core::Result<bool> write_audio_pcm(const audio::render::RenderedPcm& pcm);
    core::Result<bool> finish();
    void cancel() noexcept;

    [[nodiscard]] ExportSessionState state() const noexcept;
    [[nodiscard]] const std::filesystem::path& target_path() const noexcept;
    [[nodiscard]] const std::filesystem::path& temporary_path() const noexcept;
    [[nodiscard]] const ExportFormatDescriptor& format() const noexcept;

private:
    ExportSession(std::shared_ptr<const FrozenExportSnapshot> snapshot,
                  std::filesystem::path target_path,
                  std::filesystem::path temporary_path,
                  std::unique_ptr<ExportEncoder> encoder,
                  ExportFormatDescriptor format,
                  const core::CancellationToken* cancellation);

    core::Result<bool> fail_and_cleanup(core::ErrorInfo error);
    [[nodiscard]] bool is_cancelled() const noexcept;
    void cleanup_temporary() noexcept;

    std::shared_ptr<const FrozenExportSnapshot> snapshot_;
    std::filesystem::path target_path_;
    std::filesystem::path temporary_path_;
    std::unique_ptr<ExportEncoder> encoder_;
    ExportFormatDescriptor format_;
    std::optional<core::CancellationToken> cancellation_;
    ExportSessionState state_{ExportSessionState::open};
    rendering::FrameIndex next_video_frame_{};
    std::uint64_t next_audio_frame_{};
};

} // namespace space_rhythm::media::playback_export
