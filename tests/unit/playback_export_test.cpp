#include <space_rhythm/media/playback_export.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace audio = space_rhythm::audio::render;
namespace core = space_rhythm::core;
namespace media = space_rhythm::media;
namespace playback = space_rhythm::media::playback_export;
namespace rendering = space_rhythm::rendering;

using namespace std::chrono_literals;

constexpr std::string_view digest_a{
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
constexpr std::string_view digest_b{
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"};
constexpr std::string_view digest_c{
    "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"};

std::shared_ptr<const rendering::RenderSnapshot> make_snapshot(
    core::TimelineRevision revision = 7,
    core::TimeRange range = {0, 80'000'000},
    rendering::FrameRate frame_rate = {25, 1})
{
    core::TimelineSnapshot timeline;
    timeline.project_id = core::ProjectId{"project-export"};
    timeline.timeline_revision = revision;

    rendering::RenderRecipe recipe;
    recipe.recipe_id = rendering::RenderRecipeId{"recipe-export"};
    recipe.project_id = timeline.project_id;
    recipe.timeline_revision = revision;
    recipe.template_parameters.template_id = "template.export-fixture";
    recipe.template_parameters.template_version = "0.1.0";
    recipe.template_parameters.parameters_digest_sha256 = std::string{digest_a};
    recipe.output.width_px = 2;
    recipe.output.height_px = 2;
    recipe.time_range = range;
    recipe.frame_rate = frame_rate;
    recipe.deterministic_seed = 42;
    recipe.required_features = {"render.rgba8-srgb-v1", "render.snapshot-v1"};

    auto result = rendering::make_render_snapshot(
        rendering::RenderSnapshotId{"snapshot-export"},
        std::move(recipe),
        std::move(timeline),
        {});
    EXPECT_TRUE(result) << (result ? "" : result.error().message_key);
    return result.value();
}

playback::ExportFreezeRequest make_freeze_request(
    std::shared_ptr<const rendering::RenderSnapshot> snapshot,
    bool include_audio = true)
{
    playback::ExportFreezeRequest request;
    request.job_id = "job-t019";
    request.current_timeline_revision = snapshot->recipe().timeline_revision;
    request.render_snapshot = std::move(snapshot);
    request.media.source_fingerprint_sha256 = std::string{digest_b};
    request.media.selection.video.mode = media::SelectionMode::explicit_stream;
    request.media.selection.video.selected.push_back(
        {std::string{digest_b}, 0});
    request.media.selection.audio.mode = include_audio
        ? media::SelectionMode::explicit_stream
        : media::SelectionMode::none;
    if (include_audio) {
        request.media.selection.audio.selected.push_back(
            {std::string{digest_b}, 1});
    }
    request.media.selection.presentation_origin.timestamp = {
        0, core::TimeBase{1, 1'000}, media::TimestampOrigin::stream_start};
    request.audio_parameters.sample_rate = 48'000;
    request.audio_parameters.channel_count = 2;
    request.audio_parameters.deterministic_seed = 42;
    request.timbre_sha256 = {std::string{digest_c}};
    request.time_range = request.render_snapshot->recipe().time_range;
    request.frame_rate = request.render_snapshot->recipe().frame_rate;
    request.deterministic_seed = request.render_snapshot->recipe().deterministic_seed;
    request.include_audio = include_audio;
    return request;
}

std::shared_ptr<const playback::FrozenExportSnapshot> make_frozen(bool include_audio = true)
{
    auto result = playback::freeze_export(make_freeze_request(make_snapshot(), include_audio));
    EXPECT_TRUE(result) << (result ? "" : result.error().message_key);
    return result.value();
}

rendering::RenderedFrame make_frame(const rendering::RenderSnapshot& snapshot,
                                    rendering::FrameIndex index,
                                    std::byte fill = std::byte{0x2a})
{
    rendering::RenderedFrame frame;
    frame.snapshot_id = snapshot.id();
    frame.timeline_revision = snapshot.recipe().timeline_revision;
    frame.device_generation = 1;
    frame.frame_index = index;
    frame.time_ns = rendering::frame_time_ns(snapshot.recipe(), index).value();
    frame.width_px = snapshot.recipe().output.width_px;
    frame.height_px = snapshot.recipe().output.height_px;
    frame.pixel_format = snapshot.recipe().output.pixel_format;
    frame.color = snapshot.recipe().output.color;
    frame.alpha_mode = snapshot.recipe().output.alpha_mode;
    frame.row_order = snapshot.recipe().output.row_order;
    frame.stride_bytes = static_cast<std::uint64_t>(frame.width_px) * 4U;
    frame.valid_bytes = frame.stride_bytes * frame.height_px;
    frame.bytes = rendering::FrameLease(
        std::vector<std::byte>(static_cast<std::size_t>(frame.valid_bytes), fill));
    return frame;
}

audio::RenderedPcm make_pcm(std::uint64_t frame_count)
{
    audio::RenderRequest request;
    request.start_time_ns = 0;
    request.frame_count = frame_count;
    request.parameters.deterministic_seed = 42;
    audio::DeterministicMixer mixer;
    auto prepared = mixer.prepare(std::move(request), std::span<const audio::Timbre>{});
    EXPECT_TRUE(prepared) << (prepared ? "" : prepared.error().message_key);
    auto rendered = mixer.render(prepared.value());
    EXPECT_TRUE(rendered) << (rendered ? "" : rendered.error().message_key);
    return rendered.value();
}

std::filesystem::path test_directory(std::string_view name)
{
    auto path = std::filesystem::path{SPACE_RHYTHM_T019_TEST_WORK_ROOT}
        / std::filesystem::path{std::string{name}};
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
    std::filesystem::create_directories(path);
    return path;
}

void write_text(const std::filesystem::path& path, std::string_view value)
{
    std::ofstream stream{path, std::ios::binary | std::ios::trunc};
    stream.write(value.data(), static_cast<std::streamsize>(value.size()));
}

std::string read_text(const std::filesystem::path& path)
{
    std::ifstream stream{path, std::ios::binary};
    return {std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}

core::ErrorInfo injected_error(core::ErrorCategory category,
                               core::ErrorCode code,
                               std::string stage)
{
    return {core::schema_version,
            category,
            code,
            std::move(stage),
            "t019:injected",
            false,
            "t019.injected",
            {},
            {}};
}

enum class FailurePoint { none, video, audio, finish };

struct EncoderTrace {
    bool opened{};
    bool video_called{};
    bool audio_called{};
    bool finished{};
    bool aborted{};
};

class InjectedEncoder final : public playback::ExportEncoder {
public:
    InjectedEncoder(std::shared_ptr<EncoderTrace> trace,
                    FailurePoint failure,
                    core::ErrorInfo error = injected_error(
                        core::ErrorCategory::internal,
                        core::ErrorCode::internal_error,
                        "media.export.injected"))
        : trace_(std::move(trace)), failure_(failure), error_(std::move(error))
    {
    }

    playback::ExportFormatDescriptor format() const override
    {
        return playback::ffmpeg_test_format();
    }

    core::Result<bool> open(const std::filesystem::path& path,
                            const playback::FrozenExportSnapshot&) override
    {
        path_ = path;
        trace_->opened = true;
        write_text(path_, "partial-test-export");
        return core::Result<bool>::success(true);
    }

    core::Result<bool> write_video_frame(const rendering::RenderedFrame&) override
    {
        trace_->video_called = true;
        if (failure_ == FailurePoint::video) {
            return core::Result<bool>::failure(error_);
        }
        return core::Result<bool>::success(true);
    }

    core::Result<bool> write_audio_pcm(const audio::RenderedPcm&) override
    {
        trace_->audio_called = true;
        if (failure_ == FailurePoint::audio) {
            return core::Result<bool>::failure(error_);
        }
        return core::Result<bool>::success(true);
    }

    core::Result<bool> finish() override
    {
        trace_->finished = true;
        if (failure_ == FailurePoint::finish) {
            return core::Result<bool>::failure(error_);
        }
        return core::Result<bool>::success(true);
    }

    void abort() noexcept override { trace_->aborted = true; }

private:
    std::shared_ptr<EncoderTrace> trace_;
    FailurePoint failure_;
    core::ErrorInfo error_;
    std::filesystem::path path_;
};

TEST(PreviewSynchronizer, AudioPlayedSamplesAreMasterAndVfrFramesDropWithDiagnostics)
{
    const auto start = playback::PreviewSynchronizer::MonotonicTimePoint{};
    playback::PreviewConfiguration configuration;
    configuration.timeline_range = {0, 500'000'000};
    configuration.initial_playhead_time_ns = 0;
    configuration.audio_sample_rate = 48'000;
    configuration.initial_audio_played_frames = 1'000;
    configuration.drift_warning_threshold_ns = 20'000'000;
    configuration.video_frames = {
        {0, 0}, {1, 40'000'000}, {2, 100'000'000}, {3, 180'000'000}, {4, 260'000'000}};
    auto created = playback::PreviewSynchronizer::create(configuration, start);
    ASSERT_TRUE(created);
    auto synchronizer = std::move(created.value());

    ASSERT_TRUE(synchronizer->set_audio_played_frames(5'800));
    auto first = synchronizer->tick(start + 10h);
    ASSERT_TRUE(first);
    EXPECT_EQ(first.value().clock_mode, playback::PreviewClockMode::audio_played_samples);
    EXPECT_EQ(first.value().playhead_time_ns, 100'000'000);
    ASSERT_TRUE(first.value().video_frame.has_value());
    EXPECT_EQ(first.value().video_frame->frame_index, 2U);
    EXPECT_EQ(first.value().dropped_video_frames, 2U);

    ASSERT_TRUE(synchronizer->set_audio_played_frames(8'200));
    auto drifted = synchronizer->tick(start + 20h);
    ASSERT_TRUE(drifted);
    EXPECT_EQ(drifted.value().playhead_time_ns, 150'000'000);
    EXPECT_EQ(drifted.value().video_lateness_ns, 50'000'000);
    EXPECT_FALSE(drifted.value().video_frame.has_value());
    ASSERT_EQ(drifted.value().diagnostics.size(), 1U);
    EXPECT_EQ(drifted.value().diagnostics.front().code, "video-clock-drift");

    auto sought = synchronizer->seek(200'000'000, start + 21h, 0);
    ASSERT_TRUE(sought);
    EXPECT_EQ(sought.value().playhead_time_ns, 200'000'000);
    ASSERT_TRUE(synchronizer->set_audio_played_frames(2'400));
    auto after_device_restart = synchronizer->tick(start + 22h);
    ASSERT_TRUE(after_device_restart);
    EXPECT_EQ(after_device_restart.value().playhead_time_ns, 250'000'000);
}

TEST(PreviewSynchronizer, MonotonicFallbackPauseResumeAndSeekAreCppDriven)
{
    const auto start = playback::PreviewSynchronizer::MonotonicTimePoint{};
    playback::PreviewConfiguration configuration;
    configuration.timeline_range = {0, 500'000'000};
    configuration.video_frames = {
        {0, 0}, {1, 40'000'000}, {2, 100'000'000}, {3, 180'000'000}};
    auto created = playback::PreviewSynchronizer::create(configuration, start);
    ASSERT_TRUE(created);
    auto synchronizer = std::move(created.value());

    auto running = synchronizer->tick(start + 75ms);
    ASSERT_TRUE(running);
    EXPECT_EQ(running.value().clock_mode, playback::PreviewClockMode::monotonic);
    EXPECT_EQ(running.value().playhead_time_ns, 75'000'000);
    ASSERT_TRUE(running.value().video_frame.has_value());
    EXPECT_EQ(running.value().video_frame->time_ns, 40'000'000);

    auto paused = synchronizer->pause(start + 80ms);
    ASSERT_TRUE(paused);
    EXPECT_EQ(paused.value().playhead_time_ns, 80'000'000);
    auto still_paused = synchronizer->tick(start + 2s);
    ASSERT_TRUE(still_paused);
    EXPECT_EQ(still_paused.value().playhead_time_ns, 80'000'000);

    ASSERT_TRUE(synchronizer->resume(start + 2s));
    auto resumed = synchronizer->tick(start + 2030ms);
    ASSERT_TRUE(resumed);
    EXPECT_EQ(resumed.value().playhead_time_ns, 110'000'000);

    auto sought = synchronizer->seek(180'000'000, start + 3s);
    ASSERT_TRUE(sought);
    EXPECT_EQ(sought.value().playhead_time_ns, 180'000'000);
    ASSERT_TRUE(sought.value().video_frame.has_value());
    EXPECT_EQ(sought.value().video_frame->frame_index, 3U);
    EXPECT_EQ(sought.value().dropped_video_frames, 0U);
}

TEST(PreviewSynchronizer, EventPositionsAreMeasuredAgainstOneExactOutputFrame)
{
    const std::vector<core::TimeNs> preview{100'000'000, 205'000'000, 300'000'000};
    const std::vector<core::TimeNs> exported{100'000'000, 200'000'000, 333'000'000};
    auto aligned = playback::compare_event_positions(
        preview, exported, rendering::FrameRate{30, 1});
    ASSERT_TRUE(aligned);
    EXPECT_EQ(aligned.value().one_output_frame_ns, 33'333'334);
    EXPECT_EQ(aligned.value().maximum_error_ns, 33'000'000);
    EXPECT_TRUE(aligned.value().within_one_output_frame);

    const std::vector<core::TimeNs> too_late{140'000'000};
    auto misaligned = playback::compare_event_positions(
        std::span{preview}.first<1>(), too_late, rendering::FrameRate{30, 1});
    ASSERT_TRUE(misaligned);
    EXPECT_FALSE(misaligned.value().within_one_output_frame);
}

TEST(ExportFreeze, DeepCopiesEveryInputAndRejectsStaleRevision)
{
    auto request = make_freeze_request(make_snapshot());
    auto frozen = playback::freeze_export(request);
    ASSERT_TRUE(frozen);
    EXPECT_EQ(frozen.value()->render_recipe().timeline_revision, 7U);
    EXPECT_EQ(frozen.value()->media().source_fingerprint_sha256, digest_b);
    EXPECT_EQ(frozen.value()->audio_parameters().sample_rate, 48'000U);
    EXPECT_EQ(frozen.value()->timbre_sha256(),
              std::vector<std::string>{std::string{digest_c}});
    EXPECT_EQ(frozen.value()->time_range(), (core::TimeRange{0, 80'000'000}));
    EXPECT_EQ(frozen.value()->frame_rate(), (rendering::FrameRate{25, 1}));
    EXPECT_EQ(frozen.value()->deterministic_seed(), 42U);
    EXPECT_TRUE(frozen.value()->includes_audio());
    EXPECT_EQ(frozen.value()->frozen_inputs_sha256().size(), 64U);

    request.media.source_fingerprint_sha256 = std::string{digest_a};
    request.audio_parameters.sample_rate = 44'100;
    request.timbre_sha256.front() = std::string{digest_a};
    EXPECT_EQ(frozen.value()->media().source_fingerprint_sha256, digest_b);
    EXPECT_EQ(frozen.value()->audio_parameters().sample_rate, 48'000U);
    EXPECT_EQ(frozen.value()->timbre_sha256().front(), digest_c);

    auto stale = make_freeze_request(make_snapshot());
    stale.current_timeline_revision = 8;
    auto rejected = playback::freeze_export(std::move(stale));
    ASSERT_FALSE(rejected);
    EXPECT_EQ(rejected.error().category, core::ErrorCategory::conflict);
    EXPECT_EQ(rejected.error().code, core::ErrorCode::stale_revision);
}

TEST(ExportTransaction, FfmpegTestFormatConsumesRenderedFrameAndDeterministicPcm)
{
    auto frozen = make_frozen();
    const auto directory = test_directory("ffmpeg-success-existing-target");
    const auto target = directory / "preview-test.nut";
    write_text(target, "existing-target-must-be-replaced-only-on-success");

    auto created = playback::ExportSession::create(
        frozen, target, playback::make_ffmpeg_test_encoder());
    ASSERT_TRUE(created) << (created ? "" : created.error().message_key);
    auto session = std::move(created.value());
    EXPECT_EQ(session->temporary_path().parent_path(), session->target_path().parent_path());
    EXPECT_TRUE(session->format().test_only);
    EXPECT_EQ(session->format().id, playback::ffmpeg_test_format_id);

    ASSERT_TRUE(session->write_video_frame(
        make_frame(frozen->render_snapshot(), 0, std::byte{0x11})));
    ASSERT_TRUE(session->write_video_frame(
        make_frame(frozen->render_snapshot(), 1, std::byte{0x77})));
    ASSERT_TRUE(session->write_audio_pcm(make_pcm(3'840)));
    auto finished = session->finish();
    ASSERT_TRUE(finished) << (finished ? "" : finished.error().message_key);
    EXPECT_EQ(session->state(), playback::ExportSessionState::committed);
    EXPECT_FALSE(std::filesystem::exists(session->temporary_path()));
    EXPECT_NE(read_text(target), "existing-target-must-be-replaced-only-on-success");

    auto probed = media::MediaSource::open(target);
    ASSERT_TRUE(probed) << (probed ? "" : probed.error().message_key);
    const auto& info = probed.value()->info();
    EXPECT_TRUE(std::ranges::any_of(info.streams, [](const media::StreamInfo& stream) {
        return stream.kind == media::StreamKind::video;
    }));
    EXPECT_TRUE(std::ranges::any_of(info.streams, [](const media::StreamInfo& stream) {
        return stream.kind == media::StreamKind::audio;
    }));
}

TEST(ExportTransaction, CancellationPreservesExistingTargetAndDeletesTemporary)
{
    auto frozen = make_frozen(false);
    const auto directory = test_directory("cancel");
    const auto target = directory / "existing.nut";
    write_text(target, "old-target");
    auto trace = std::make_shared<EncoderTrace>();
    core::CancellationToken token;
    auto created = playback::ExportSession::create(
        frozen,
        target,
        std::make_unique<InjectedEncoder>(trace, FailurePoint::none),
        &token);
    ASSERT_TRUE(created);
    auto session = std::move(created.value());
    const auto temporary = session->temporary_path();
    token.cancel();

    auto result = session->write_video_frame(make_frame(frozen->render_snapshot(), 0));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().category, core::ErrorCategory::cancelled);
    EXPECT_EQ(session->state(), playback::ExportSessionState::cancelled);
    EXPECT_TRUE(trace->aborted);
    EXPECT_FALSE(std::filesystem::exists(temporary));
    EXPECT_EQ(read_text(target), "old-target");
}

TEST(ExportTransaction, DiskFailureAndEncoderFailurePreserveExistingTarget)
{
    for (const auto [name, error] : std::vector<std::pair<std::string, core::ErrorInfo>>{
             {"disk-full",
              injected_error(core::ErrorCategory::resource_limit,
                             core::ErrorCode::resource_limit,
                             "media.export.disk-full")},
             {"encode-failed",
              injected_error(core::ErrorCategory::internal,
                             core::ErrorCode::internal_error,
                             "media.export.encode-failed")}}) {
        auto frozen = make_frozen(false);
        const auto directory = test_directory(name);
        const auto target = directory / "existing.nut";
        write_text(target, "old-target");
        auto trace = std::make_shared<EncoderTrace>();
        auto created = playback::ExportSession::create(
            frozen,
            target,
            std::make_unique<InjectedEncoder>(trace, FailurePoint::video, error));
        ASSERT_TRUE(created);
        auto session = std::move(created.value());
        const auto temporary = session->temporary_path();

        auto result = session->write_video_frame(
            make_frame(frozen->render_snapshot(), 0));
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().category, error.category);
        EXPECT_TRUE(trace->video_called);
        EXPECT_TRUE(trace->aborted);
        EXPECT_FALSE(std::filesystem::exists(temporary));
        EXPECT_EQ(read_text(target), "old-target");
    }
}

TEST(ExportTransaction, WorkerInterruptionAbortsAndNeverTouchesExistingTarget)
{
    auto frozen = make_frozen(false);
    const auto directory = test_directory("worker-interruption");
    const auto target = directory / "existing.nut";
    write_text(target, "old-target");
    auto trace = std::make_shared<EncoderTrace>();
    std::filesystem::path temporary;
    {
        auto created = playback::ExportSession::create(
            frozen,
            target,
            std::make_unique<InjectedEncoder>(trace, FailurePoint::none));
        ASSERT_TRUE(created);
        auto session = std::move(created.value());
        temporary = session->temporary_path();
        ASSERT_TRUE(session->write_video_frame(
            make_frame(frozen->render_snapshot(), 0)));
        EXPECT_TRUE(std::filesystem::exists(temporary));
    }
    EXPECT_TRUE(trace->aborted);
    EXPECT_FALSE(trace->finished);
    EXPECT_FALSE(std::filesystem::exists(temporary));
    EXPECT_EQ(read_text(target), "old-target");
}

TEST(ExportTransaction, MissingFramesAndWrongPcmSequenceFailClosed)
{
    {
        auto frozen = make_frozen();
        const auto directory = test_directory("wrong-pcm-sequence");
        const auto target = directory / "existing.nut";
        write_text(target, "old-target");
        auto trace = std::make_shared<EncoderTrace>();
        auto created = playback::ExportSession::create(
            frozen,
            target,
            std::make_unique<InjectedEncoder>(trace, FailurePoint::none));
        ASSERT_TRUE(created);
        auto session = std::move(created.value());

        auto pcm = make_pcm(100);
        pcm.first_frame = 1;
        auto result = session->write_audio_pcm(pcm);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().code, core::ErrorCode::invalid_pcm_buffer);
        EXPECT_EQ(read_text(target), "old-target");
    }
    {
        auto frozen = make_frozen(false);
        const auto directory = test_directory("missing-video-frame");
        const auto target = directory / "existing.nut";
        write_text(target, "old-target");
        auto trace = std::make_shared<EncoderTrace>();
        auto created = playback::ExportSession::create(
            frozen,
            target,
            std::make_unique<InjectedEncoder>(trace, FailurePoint::none));
        ASSERT_TRUE(created);
        auto session = std::move(created.value());
        ASSERT_TRUE(session->write_video_frame(
            make_frame(frozen->render_snapshot(), 0)));

        auto result = session->finish();
        ASSERT_FALSE(result);
        EXPECT_FALSE(trace->finished);
        EXPECT_TRUE(trace->aborted);
        EXPECT_EQ(read_text(target), "old-target");
    }
}

} // namespace
