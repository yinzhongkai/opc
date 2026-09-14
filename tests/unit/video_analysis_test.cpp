#include <space_rhythm/video/analysis.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace
{

namespace core = space_rhythm::core;
namespace media = space_rhythm::media;
namespace video = space_rhythm::video;

struct DecodedFixture
{
    std::string fingerprint;
    media::StreamKey stream_key;
    std::vector<media::VideoFrame> frames;
};

[[nodiscard]] std::filesystem::path golden(const std::string_view name)
{
    return std::filesystem::path{SPACE_RHYTHM_GOLDEN_VIDEO_DIR} / name;
}

[[nodiscard]] DecodedFixture decode_fixture(const std::string_view name)
{
    const auto source = media::MediaSource::open(golden(name));
    if (!source)
    {
        throw std::runtime_error{"unable to open video golden"};
    }
    const auto selection = source.value()->select(
        {media::SelectionMode::required_default_then_lowest_index, std::nullopt, false},
        {media::SelectionMode::none, std::nullopt, false});
    if (!selection || selection.value().video.selected.size() != 1U)
    {
        throw std::runtime_error{"unable to select video golden stream"};
    }
    DecodedFixture fixture;
    fixture.fingerprint = source.value()->info().source_fingerprint_sha256;
    fixture.stream_key = selection.value().video.selected.front();
    const auto decoded = source.value()->decode_video(
        selection.value(), fixture.stream_key, {160, 90, true}, media::DecodeLimits{},
        {{},
         [&](media::VideoFrame frame)
         {
             fixture.frames.push_back(std::move(frame));
             return media::PublishResult::accepted;
         }});
    if (!decoded || !decoded.value().end_of_stream)
    {
        throw std::runtime_error{"unable to decode video golden"};
    }
    return fixture;
}

[[nodiscard]] video::AnalysisRequest make_request(const DecodedFixture& fixture,
                                                  const video::AnalysisParameters& parameters,
                                                  std::string job_id = "T-028-unit")
{
    video::AnalysisRequest request;
    request.job_id = std::move(job_id);
    request.source_fingerprint_sha256 = fixture.fingerprint;
    request.stream_key = fixture.stream_key;
    request.parameters_digest_sha256 = parameters.parameters_digest_sha256;
    request.analysis_revision.value = "video-analysis-revision-1";
    return request;
}

[[nodiscard]] bool has_diagnostic(const video::AnalysisResult& result, const std::string_view code)
{
    return std::ranges::any_of(result.diagnostics,
                               [&](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] std::vector<const core::AnalysisCandidate*>
candidates_of(const video::AnalysisResult& result, const core::EventKind kind)
{
    std::vector<const core::AnalysisCandidate*> candidates;
    for (const auto& candidate : result.candidates)
    {
        if (candidate.kind == kind)
        {
            candidates.push_back(&candidate);
        }
    }
    return candidates;
}

[[nodiscard]] bool has_event_near(const video::AnalysisResult& result, const core::EventKind kind,
                                  const core::TimeNs expected, const core::DurationNs tolerance)
{
    return std::ranges::any_of(result.candidates,
                               [&](const auto& candidate)
                               {
                                   return candidate.kind == kind &&
                                          candidate.time_ns >= expected - tolerance &&
                                          candidate.time_ns <= expected + tolerance;
                               });
}

[[nodiscard]] video::AnalysisResult analyze_fixture(const DecodedFixture& fixture)
{
    const auto parameters = video::production_parameters();
    return video::Analyzer{}.analyze(fixture.frames, make_request(fixture, parameters), parameters);
}

TEST(VideoAnalysisContract, FreezesVersionsParametersAndOpenCvRuntime)
{
    const auto parameters = video::production_parameters();
    EXPECT_EQ(video::contract_version, "0.1.0");
    EXPECT_EQ(video::algorithm_id, "space-rhythm.video-analysis.classic");
    EXPECT_EQ(video::algorithm_version, "1.0.0");
    EXPECT_EQ(parameters.id, "space-rhythm.video-analysis.production");
    EXPECT_EQ(parameters.version, "1.0.0");
    EXPECT_EQ(parameters.parameters_digest_sha256.size(), 64U);
    EXPECT_EQ(video::opencv_runtime_version(), "4.12.0");
    EXPECT_EQ(video::production_parameters(), parameters);
    EXPECT_EQ(video::canonical_parameters_json(parameters),
              video::canonical_parameters_json(video::production_parameters()));
}

TEST(VideoAnalysisValidation, RejectsContractAndFrameViolationsWithoutCandidates)
{
    auto fixture = decode_fixture("static-grid-30fps.mkv");
    const auto parameters = video::production_parameters();
    auto request = make_request(fixture, parameters);
    request.algorithm_version = "unexpected";
    auto result = video::Analyzer{}.analyze(fixture.frames, request, parameters);
    ASSERT_EQ(result.status, video::AnalysisStatus::failed);
    ASSERT_TRUE(result.error);
    EXPECT_EQ(result.error->code, core::ErrorCode::invalid_dto);
    EXPECT_TRUE(result.candidates.empty());

    request = make_request(fixture, parameters);
    fixture.frames.front().time_ns = -1;
    result = video::Analyzer{}.analyze(fixture.frames, request, parameters);
    ASSERT_EQ(result.status, video::AnalysisStatus::failed);
    ASSERT_TRUE(result.error);
    EXPECT_EQ(result.error->code, core::ErrorCode::invalid_feature_frame);
    EXPECT_TRUE(result.candidates.empty());

    auto discontinuous = decode_fixture("global-pan-ramp-30fps.mkv");
    ASSERT_GT(discontinuous.frames.size(), 20U);
    discontinuous.frames[20].time_ns = discontinuous.frames[19].time_ns - 1;
    request = make_request(discontinuous, parameters, "T-028-pts-discontinuity");
    result = video::Analyzer{}.analyze(discontinuous.frames, request, parameters);
    EXPECT_EQ(result.status, video::AnalysisStatus::low_quality);
    EXPECT_TRUE(has_diagnostic(result, "timestamp_discontinuity"));
    EXPECT_GE(result.segment_count, 2U);
}

TEST(VideoAnalysisGolden, DetectsHardAndGradualCutsAndSuppressesFlash)
{
    const auto hard = analyze_fixture(decode_fixture("hard-cut-30fps.mkv"));
    ASSERT_EQ(hard.status, video::AnalysisStatus::completed);
    EXPECT_EQ(candidates_of(hard, core::EventKind::shot).size(), 1U);
    EXPECT_TRUE(has_event_near(hard, core::EventKind::shot, 1'000'000'000, 34'000'000));
    EXPECT_TRUE(candidates_of(hard, core::EventKind::motion_peak).empty());
    EXPECT_TRUE(candidates_of(hard, core::EventKind::action_peak).empty());

    const auto dissolve = analyze_fixture(decode_fixture("dissolve-30fps.mkv"));
    ASSERT_EQ(dissolve.status, video::AnalysisStatus::completed);
    const auto gradual = candidates_of(dissolve, core::EventKind::shot);
    ASSERT_EQ(gradual.size(), 1U);
    EXPECT_EQ(gradual.front()->payload.payload.at("boundaryKind"), "gradual_transition");
    EXPECT_NEAR(static_cast<double>(gradual.front()->time_ns), 1'000'000'000.0, 34'000'000.0);
    EXPECT_NEAR(static_cast<double>(gradual.front()->duration_ns), 1'000'000'000.0, 70'000'000.0);
    EXPECT_TRUE(candidates_of(dissolve, core::EventKind::motion_peak).empty());
    EXPECT_TRUE(candidates_of(dissolve, core::EventKind::action_peak).empty());

    const auto flash = analyze_fixture(decode_fixture("single-frame-flash-30fps.mkv"));
    ASSERT_EQ(flash.status, video::AnalysisStatus::completed);
    EXPECT_TRUE(flash.candidates.empty());
    EXPECT_TRUE(has_diagnostic(flash, "flash_ambiguous"));
}

TEST(VideoAnalysisGolden, SeparatesGlobalMotionAndLocalAction)
{
    const auto global_fixture = decode_fixture("global-pan-ramp-30fps.mkv");
    const auto global = analyze_fixture(global_fixture);
    ASSERT_EQ(global.status, video::AnalysisStatus::completed);
    ASSERT_EQ(global.motion_curve_samples.size(), global_fixture.frames.size() - 1U);
    const auto maximum_global = std::ranges::max_element(
        global.motion_curve_samples, {}, &video::MotionCurveSample::global_motion_ppm);
    ASSERT_NE(maximum_global, global.motion_curve_samples.end());
    EXPECT_NEAR(static_cast<double>(maximum_global->time_ns), 500'000'000.0, 67'000'000.0);
    EXPECT_GT(maximum_global->global_motion_ppm, maximum_global->local_residual_ppm);
    ASSERT_TRUE(has_event_near(global, core::EventKind::motion_peak, 500'000'000, 100'000'000));
    const auto global_peaks = candidates_of(global, core::EventKind::motion_peak);
    ASSERT_EQ(global_peaks.size(), 1U);
    EXPECT_TRUE(
        std::ranges::any_of(global_peaks, [](const auto* candidate)
                            { return candidate->payload.payload.at("motionClass") == "global"; }));
    EXPECT_TRUE(candidates_of(global, core::EventKind::shot).empty());
    EXPECT_TRUE(candidates_of(global, core::EventKind::action_peak).empty());

    const auto local_fixture = decode_fixture("local-impact-30fps.mkv");
    const auto local = analyze_fixture(local_fixture);
    ASSERT_EQ(local.status, video::AnalysisStatus::completed);
    ASSERT_EQ(local.motion_curve_samples.size(), local_fixture.frames.size() - 1U);
    const auto local_impact =
        std::ranges::find_if(local.motion_curve_samples,
                             [](const auto& sample) { return sample.time_ns == 1'500'000'000; });
    ASSERT_NE(local_impact, local.motion_curve_samples.end());
    EXPECT_GT(local_impact->local_residual_ppm, local_impact->global_motion_ppm);
    EXPECT_TRUE(has_event_near(local, core::EventKind::motion_peak, 1'500'000'000, 67'000'000));
    EXPECT_TRUE(has_event_near(local, core::EventKind::action_peak, 1'500'000'000, 67'000'000));
    EXPECT_EQ(candidates_of(local, core::EventKind::motion_peak).size(), 1U);
    EXPECT_EQ(candidates_of(local, core::EventKind::action_peak).size(), 1U);
    EXPECT_TRUE(candidates_of(local, core::EventKind::shot).empty());
}

TEST(VideoAnalysisGolden, CoversStaticSlowMotionVfrNoiseAndMixedEvents)
{
    const auto static_result = analyze_fixture(decode_fixture("static-grid-30fps.mkv"));
    ASSERT_EQ(static_result.status, video::AnalysisStatus::completed);
    EXPECT_TRUE(static_result.candidates.empty());
    EXPECT_TRUE(has_diagnostic(static_result, "near_static"));

    const auto slow = analyze_fixture(decode_fixture("slow-motion-stop-60fps.mkv"));
    ASSERT_EQ(slow.status, video::AnalysisStatus::completed);
    EXPECT_TRUE(has_event_near(slow, core::EventKind::motion_peak, 2'500'000'000, 120'000'000));
    EXPECT_TRUE(has_event_near(slow, core::EventKind::action_peak, 2'500'000'000, 120'000'000));
    EXPECT_EQ(candidates_of(slow, core::EventKind::motion_peak).size(), 1U);
    EXPECT_EQ(candidates_of(slow, core::EventKind::action_peak).size(), 1U);
    EXPECT_TRUE(candidates_of(slow, core::EventKind::shot).empty());

    const auto vfr_fixture = decode_fixture("vfr-reversal.mkv");
    std::vector<core::TimeNs> actual_times;
    std::ranges::transform(vfr_fixture.frames, std::back_inserter(actual_times),
                           &media::VideoFrame::time_ns);
    EXPECT_EQ(actual_times, (std::vector<core::TimeNs>{0, 40'000'000, 100'000'000, 140'000'000,
                                                       240'000'000, 400'000'000, 600'000'000}));
    const auto vfr = analyze_fixture(vfr_fixture);
    ASSERT_EQ(vfr.status, video::AnalysisStatus::completed);
    std::vector<core::TimeNs> curve_times;
    std::ranges::transform(vfr.motion_curve_samples, std::back_inserter(curve_times),
                           &video::MotionCurveSample::time_ns);
    EXPECT_EQ(curve_times, (std::vector<core::TimeNs>{40'000'000, 100'000'000, 140'000'000,
                                                      240'000'000, 400'000'000, 600'000'000}));
    EXPECT_TRUE(has_event_near(vfr, core::EventKind::motion_peak, 240'000'000, 60'000'000));
    EXPECT_TRUE(has_event_near(vfr, core::EventKind::action_peak, 240'000'000, 60'000'000));
    EXPECT_EQ(candidates_of(vfr, core::EventKind::motion_peak).size(), 1U);
    EXPECT_EQ(candidates_of(vfr, core::EventKind::action_peak).size(), 1U);
    EXPECT_TRUE(candidates_of(vfr, core::EventKind::shot).empty());

    const auto compression = analyze_fixture(decode_fixture("compression-noise-static-30fps.mkv"));
    ASSERT_EQ(compression.status, video::AnalysisStatus::low_quality);
    EXPECT_TRUE(compression.candidates.empty());
    EXPECT_TRUE(has_diagnostic(compression, "compression_noise"));

    const auto mixed = analyze_fixture(decode_fixture("fast-cut-action-30fps.mkv"));
    ASSERT_EQ(mixed.status, video::AnalysisStatus::completed);
    for (const auto time : {500'000'000LL, 1'000'000'000LL, 1'500'000'000LL})
    {
        EXPECT_TRUE(has_event_near(mixed, core::EventKind::shot, time, 34'000'000));
    }
    EXPECT_TRUE(has_event_near(mixed, core::EventKind::motion_peak, 1'250'000'000, 67'000'000));
    EXPECT_TRUE(has_event_near(mixed, core::EventKind::action_peak, 1'250'000'000, 67'000'000));
    EXPECT_EQ(candidates_of(mixed, core::EventKind::shot).size(), 3U);
    EXPECT_EQ(candidates_of(mixed, core::EventKind::motion_peak).size(), 1U);
    EXPECT_EQ(candidates_of(mixed, core::EventKind::action_peak).size(), 1U);
}

TEST(VideoAnalysisProperties, IsDeterministicAndEmitsTraceableOrderedCandidates)
{
    const auto fixture = decode_fixture("fast-cut-action-30fps.mkv");
    const auto parameters = video::production_parameters();
    const auto request = make_request(fixture, parameters, "T-028-determinism");
    video::Analyzer analyzer;
    const auto first = analyzer.analyze(fixture.frames, request, parameters);
    const auto second = analyzer.analyze(fixture.frames, request, parameters);
    ASSERT_EQ(first.status, video::AnalysisStatus::completed);
    EXPECT_EQ(first.status, second.status);
    EXPECT_EQ(first.schema_version, video::schema_version);
    EXPECT_EQ(first.video_analysis_contract_version, video::contract_version);
    EXPECT_EQ(first.core_contract_version, core::contract_version);
    EXPECT_EQ(first.core_schema_version, core::schema_version);
    EXPECT_EQ(first.media_contract_version, media::contract_version);
    EXPECT_EQ(first.media_schema_version, media::schema_version);
    EXPECT_EQ(first.job_id, request.job_id);
    EXPECT_EQ(first.source_fingerprint_sha256, request.source_fingerprint_sha256);
    EXPECT_EQ(first.stream_key, request.stream_key);
    EXPECT_EQ(first.deterministic_seed, request.deterministic_seed);
    EXPECT_EQ(first.analysis_revision, request.analysis_revision);
    EXPECT_EQ(first.input_range_ns, request.input_range_ns);
    EXPECT_EQ(first.parameter_set_id, request.parameter_set_id);
    EXPECT_EQ(first.parameter_schema_version, request.parameter_schema_version);
    EXPECT_EQ(first.motion_curve_schema_version, video::motion_curve_schema_version);
    EXPECT_EQ(first.motion_curve_samples, second.motion_curve_samples);
    EXPECT_EQ(first.candidates, second.candidates);
    EXPECT_EQ(first.diagnostics, second.diagnostics);
    EXPECT_EQ(first.estimated_peak_working_bytes, second.estimated_peak_working_bytes);
    EXPECT_FALSE(first.candidates.empty());
    for (const auto& candidate : first.candidates)
    {
        EXPECT_GE(candidate.time_ns, 0);
        EXPECT_LE(candidate.strength_ppm, core::norm_ppm_max);
        EXPECT_LE(candidate.confidence_ppm, core::norm_ppm_max);
        EXPECT_EQ(candidate.source.producer_id, "space-rhythm.video-analysis");
        EXPECT_EQ(candidate.source.producer_version, video::algorithm_version);
        ASSERT_TRUE(candidate.source.input_fingerprint);
        EXPECT_EQ(*candidate.source.input_fingerprint, fixture.fingerprint);
        ASSERT_TRUE(candidate.source.parameters_digest);
        EXPECT_EQ(*candidate.source.parameters_digest, parameters.parameters_digest_sha256);
        ASSERT_TRUE(candidate.source.analysis_revision);
        EXPECT_EQ(candidate.source.analysis_revision->value, request.analysis_revision.value);
        EXPECT_EQ(candidate.payload.owner, "space-rhythm.video-analysis");
        EXPECT_EQ(candidate.payload.payload.at("detectorVersion"), video::algorithm_version);
        EXPECT_TRUE(candidate.id.value.starts_with("va:"));
    }
    EXPECT_TRUE(
        std::ranges::is_sorted(first.candidates,
                               [](const auto& left, const auto& right)
                               {
                                   return std::tuple{left.time_ns, left.kind, left.id.value} <
                                          std::tuple{right.time_ns, right.kind, right.id.value};
                               }));
}

TEST(VideoAnalysisResources, EnforcesFrameAndWorkingMemoryBounds)
{
    const auto fixture = decode_fixture("global-pan-ramp-30fps.mkv");
    const auto parameters = video::production_parameters();
    const auto request = make_request(fixture, parameters, "T-028-limits");
    video::AnalysisLimits frame_limit;
    frame_limit.max_input_frames = fixture.frames.size() - 1U;
    auto result = video::Analyzer{}.analyze(fixture.frames, request, parameters, frame_limit);
    ASSERT_EQ(result.status, video::AnalysisStatus::failed);
    ASSERT_TRUE(result.error);
    EXPECT_EQ(result.error->category, core::ErrorCategory::resource_limit);

    video::AnalysisLimits working_limit;
    working_limit.max_working_bytes = 1U;
    result = video::Analyzer{}.analyze(fixture.frames, request, parameters, working_limit);
    ASSERT_EQ(result.status, video::AnalysisStatus::failed);
    ASSERT_TRUE(result.error);
    EXPECT_EQ(result.error->category, core::ErrorCategory::resource_limit);

    const video::AnalysisLimits ordinary_limit;
    result = video::Analyzer{}.analyze(fixture.frames, request, parameters, ordinary_limit);
    EXPECT_NE(result.status, video::AnalysisStatus::failed);
    EXPECT_LE(result.estimated_peak_working_bytes, ordinary_limit.max_working_bytes);
}

TEST(VideoAnalysisCancellation, ObservesPreCancellationAndMidAnalysisCancellation)
{
    const auto fixture = decode_fixture("global-pan-ramp-30fps.mkv");
    const auto parameters = video::production_parameters();
    const auto request = make_request(fixture, parameters, "T-028-cancellation");
    core::CancellationToken cancellation;
    cancellation.cancel();
    auto result = video::Analyzer{}.analyze(fixture.frames, request, parameters, {}, &cancellation);
    ASSERT_EQ(result.status, video::AnalysisStatus::cancelled);
    EXPECT_TRUE(result.motion_curve_samples.empty());
    EXPECT_TRUE(result.candidates.empty());

    std::vector<media::VideoFrame> long_input;
    long_input.reserve(3'000U);
    for (std::size_t index = 0; index < 3'000U; ++index)
    {
        auto frame = fixture.frames[index % fixture.frames.size()];
        frame.time_ns = static_cast<core::TimeNs>(index) * 1'000'000;
        frame.decode_ordinal = index;
        long_input.push_back(std::move(frame));
    }
    core::CancellationToken mid_cancellation;
    std::atomic_bool started{false};
    std::thread worker(
        [&]()
        {
            started.store(true, std::memory_order_release);
            result =
                video::Analyzer{}.analyze(long_input, request, parameters, {}, &mid_cancellation);
        });
    while (!started.load(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
    mid_cancellation.cancel();
    worker.join();
    EXPECT_EQ(result.status, video::AnalysisStatus::cancelled);
    EXPECT_TRUE(result.motion_curve_samples.empty());
    EXPECT_TRUE(result.candidates.empty());
}

} // namespace
