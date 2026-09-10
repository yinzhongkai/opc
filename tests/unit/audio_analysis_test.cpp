#include <gtest/gtest.h>

#include <space_rhythm/audio/analysis.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

using space_rhythm::audio::AnalysisResult;
using space_rhythm::audio::AnalysisStatus;
using space_rhythm::audio::CandidateKind;
using space_rhythm::audio::DspPcmBuffer;
using space_rhythm::audio::PcmAdapterContext;
using space_rhythm::audio::PcmNarrowAdapter;
using space_rhythm::audio::ResampleTrace;
using space_rhythm::core::ErrorCode;
using space_rhythm::media::BufferLease;
using space_rhythm::media::PcmBuffer;
using space_rhythm::media::PlaneView;
using space_rhythm::media::StreamKey;

struct FixtureSpec {
    const char* file;
    const char* id;
    const char* sha256;
    std::uint32_t sample_rate;
    std::uint32_t channels;
    std::uint64_t frame_count;
    std::int64_t first_sample_index;
};

constexpr FixtureSpec kImpulse{"impulse-48000-mono.f32le",
                               "AV-IMPULSE-001",
                               "bb58d0b6ca0c2b73b36b5629f64b2a141155f3592e50008099cd088f84ed5ba1",
                               48'000,
                               1,
                               48'000,
                               0};
constexpr FixtureSpec kFixedBeat{"fixed-beat-120bpm-48000-mono.f32le",
                                 "AV-FIXED-BEAT-120-001",
                                 "c44e3d6c621a7bd8c3d481e941688cb4a31a036a6c915b376594a616b23dcf0a",
                                 48'000,
                                 1,
                                 96'000,
                                 0};
constexpr FixtureSpec kTempoChange{"tempo-change-48000-mono.f32le",
                                   "AV-TEMPO-CHANGE-001",
                                   "9a023e968e7539bb17eb86147596378b800e1579e485b1de03597079ca72b6e3",
                                   48'000,
                                   1,
                                   144'000,
                                   0};
constexpr FixtureSpec kSilence{"silence-44100-mono.f32le",
                               "AV-SILENCE-44100-001",
                               "fd6f479534cdd14635e88dfedf25c3859c01062b645f2a85570f20451b4a95bc",
                               44'100,
                               1,
                               44'100,
                               0};
constexpr FixtureSpec kNoise{"noise-48000-mono.f32le",
                             "AV-NOISE-001",
                             "d0a412e27b90675e067edf2aa674209a543601f6b5639dfcd91f407500aaa9d7",
                             48'000,
                             1,
                             48'000,
                             0};
constexpr FixtureSpec kBoundary{"boundary-48000-stereo.f32le",
                                "AV-BOUNDARY-001",
                                "4a4bc46ced2d02248933f73d233ab82cea0c43ce50561f22abba85b4beed0266",
                                48'000,
                                2,
                                16,
                                -3};
constexpr FixtureSpec kNonFinite{"nonfinite-48000-mono.f32le",
                                 "AV-NONFINITE-001",
                                 "9759c8518a6bc15a409b93899ec0b9b17013e2b747af489364d4ef42f4e95d54",
                                 48'000,
                                 1,
                                 4,
                                 0};

[[nodiscard]] std::filesystem::path fixture_path(const FixtureSpec& spec)
{
    return std::filesystem::path{SPACE_RHYTHM_GOLDEN_AUDIO_DIR} / spec.file;
}

[[nodiscard]] std::vector<std::byte> read_bytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        throw std::runtime_error{"unable to open fixture: " + path.string()};
    }
    const auto size = input.tellg();
    if (size < 0) {
        throw std::runtime_error{"unable to size fixture: " + path.string()};
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input) {
        throw std::runtime_error{"unable to read fixture: " + path.string()};
    }
    return bytes;
}

[[nodiscard]] PcmBuffer make_media_pcm(std::vector<std::byte> bytes,
                                       const FixtureSpec& spec,
                                       std::string segment_id = {})
{
    PcmBuffer pcm;
    pcm.stream_key = StreamKey{spec.sha256, 0};
    const auto time = space_rhythm::media::sample_index_to_time_ns(
        spec.first_sample_index,
        spec.sample_rate,
        0,
        space_rhythm::core::RoundingMode::nearest_ties_to_even);
    const auto duration = space_rhythm::core::scale_ticks(
        static_cast<std::int64_t>(spec.frame_count),
        {1, spec.sample_rate},
        space_rhythm::core::RoundingMode::nearest_ties_to_even);
    if (!time || !duration) {
        throw std::runtime_error{"fixture time setup failed"};
    }
    pcm.time_ns = time.value();
    pcm.duration_ns = duration.value();
    pcm.segment_id = segment_id.empty() ? std::string{spec.id} + "-segment-0"
                                        : std::move(segment_id);
    pcm.first_sample_index = spec.first_sample_index;
    pcm.sample_count = spec.frame_count;
    pcm.sample_rate = spec.sample_rate;
    pcm.sample_format = "flt";
    pcm.channel_layout = spec.channels == 1U ? "mono" : "stereo";
    pcm.planar = false;
    const auto stride = static_cast<std::uint32_t>(spec.channels * sizeof(float));
    pcm.planes.push_back(PlaneView{0,
                                   stride,
                                   static_cast<std::uint32_t>(spec.frame_count),
                                   spec.frame_count * stride});
    pcm.lease = BufferLease::from_bytes(std::move(bytes), 1, std::string{spec.id});
    return pcm;
}

[[nodiscard]] DspPcmBuffer adapt_fixture(const FixtureSpec& spec)
{
    auto pcm = make_media_pcm(read_bytes(fixture_path(spec)), spec);
    PcmNarrowAdapter adapter;
    const auto adapted = adapter.adapt(
        pcm, PcmAdapterContext{0, 0, ResampleTrace::identity(spec.sample_rate)});
    if (!adapted) {
        throw std::runtime_error{"fixture adaptation failed: "
                                 + std::string{space_rhythm::core::to_string(adapted.error().code)}};
    }
    return adapted.value();
}

[[nodiscard]] DspPcmBuffer adapt_samples(const std::vector<float>& samples,
                                         const std::uint32_t sample_rate,
                                         const std::uint32_t channels,
                                         std::string fingerprint,
                                         std::string segment_id,
                                         const std::int64_t first_sample_index = 0)
{
    std::vector<std::byte> bytes(samples.size() * sizeof(float));
    std::memcpy(bytes.data(), samples.data(), bytes.size());
    const FixtureSpec spec{"memory.f32le",
                           "memory",
                           fingerprint.c_str(),
                           sample_rate,
                           channels,
                           samples.size() / channels,
                           first_sample_index};
    auto pcm = make_media_pcm(std::move(bytes), spec, std::move(segment_id));
    PcmNarrowAdapter adapter;
    const auto adapted = adapter.adapt(
        pcm, PcmAdapterContext{0, 0, ResampleTrace::identity(sample_rate)});
    if (!adapted) {
        throw std::runtime_error{"memory fixture adaptation failed"};
    }
    return adapted.value();
}

[[nodiscard]] AnalysisResult analyze_fixture(const FixtureSpec& spec)
{
    const auto buffer = adapt_fixture(spec);
    return space_rhythm::audio::Analyzer{}.analyze(
        std::span<const DspPcmBuffer>{&buffer, 1},
        space_rhythm::audio::production_parameters(spec.sample_rate));
}

[[nodiscard]] std::vector<std::int64_t> candidate_indices(const AnalysisResult& result,
                                                          const CandidateKind kind)
{
    std::vector<std::int64_t> indices;
    for (const auto& candidate : result.candidates) {
        if (candidate.kind == kind) {
            indices.push_back(candidate.sample_index);
        }
    }
    return indices;
}

[[nodiscard]] std::vector<std::uint32_t> beat_tempos(const AnalysisResult& result)
{
    std::vector<std::uint32_t> tempos;
    for (const auto& candidate : result.candidates) {
        if (candidate.kind == CandidateKind::beat && candidate.tempo_millibpm.has_value()) {
            tempos.push_back(*candidate.tempo_millibpm);
        }
    }
    return tempos;
}

TEST(AudioPcmAdapter, RequiresExplicitResamplerProvenance)
{
    auto pcm = make_media_pcm(read_bytes(fixture_path(kImpulse)), kImpulse);
    PcmNarrowAdapter adapter;
    const auto result = adapter.adapt(pcm, PcmAdapterContext{});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::resample_timing_unavailable);
}

TEST(AudioPcmAdapter, AcceptsExplicitIdentityTraceAndRetainsLease)
{
    const auto buffer = adapt_fixture(kBoundary);
    EXPECT_EQ(buffer.channel_order, (std::vector<std::string>{"FL", "FR"}));
    EXPECT_EQ(buffer.first_sample_index, -3);
    EXPECT_EQ(buffer.valid_frame_count, 16U);
    EXPECT_EQ(buffer.sample_format, "f32_le");
    EXPECT_TRUE(buffer.interleaved);
    EXPECT_EQ(buffer.frame_stride_bytes, 8U);
    ASSERT_TRUE(buffer.lease);
    ASSERT_TRUE(buffer.sample(0, 0));
    ASSERT_TRUE(buffer.sample(0, 1));
    ASSERT_TRUE(buffer.sample(7, 0));
    ASSERT_TRUE(buffer.sample(8, 1));
    EXPECT_FLOAT_EQ(buffer.sample(0, 0).value(), -1.0F);
    EXPECT_FLOAT_EQ(buffer.sample(0, 1).value(), -1.0F);
    EXPECT_FLOAT_EQ(buffer.sample(7, 0).value(), 1.0F);
    EXPECT_FLOAT_EQ(buffer.sample(8, 1).value(), 1.0F);
}

TEST(AudioPcmAdapter, RejectsIncompleteNonIdentityTraceWithoutGuessing)
{
    auto pcm = make_media_pcm(read_bytes(fixture_path(kImpulse)), kImpulse);
    auto trace = ResampleTrace::identity(48'000);
    trace.performed = true;
    trace.input_sample_rate = 44'100;
    trace.implementation_version.clear();
    trace.delay_unit.clear();
    PcmNarrowAdapter adapter;
    const auto result = adapter.adapt(pcm, PcmAdapterContext{0, 0, trace});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::resample_timing_unavailable);
}

TEST(AudioPcmAdapter, RejectsSameSegmentIndexGap)
{
    constexpr auto hash =
        "66687aadf862bd776c8fc18b8e9f8e20089714856ee233b3902a591d0d5f2925";
    const FixtureSpec first_spec{"memory", "split-a", hash, 48'000, 1, 8, 0};
    const FixtureSpec second_spec{"memory", "split-b", hash, 48'000, 1, 8, 9};
    std::vector<float> values(8, 0.0F);
    std::vector<std::byte> first_bytes(values.size() * sizeof(float));
    std::vector<std::byte> second_bytes(values.size() * sizeof(float));
    std::memcpy(first_bytes.data(), values.data(), first_bytes.size());
    std::memcpy(second_bytes.data(), values.data(), second_bytes.size());
    auto first = make_media_pcm(std::move(first_bytes), first_spec, "same-segment");
    auto second = make_media_pcm(std::move(second_bytes), second_spec, "same-segment");
    PcmNarrowAdapter adapter;
    ASSERT_TRUE(adapter.adapt(first, {0, 0, ResampleTrace::identity(48'000)}));
    const auto result = adapter.adapt(second, {0, 0, ResampleTrace::identity(48'000)});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::pcm_discontinuity);
}

TEST(AudioAnalysisParameters, ProductionSetIsVersionedAndDigestStable)
{
    const auto first = space_rhythm::audio::production_parameters(48'000);
    const auto second = space_rhythm::audio::production_parameters(48'000);
    EXPECT_EQ(first, second);
    EXPECT_EQ(first.id, space_rhythm::audio::parameter_set_id);
    EXPECT_EQ(first.version, space_rhythm::audio::parameter_set_version);
    EXPECT_EQ(first.frame_length_frames, 1024U);
    EXPECT_EQ(first.hop_length_frames, 256U);
    EXPECT_EQ(first.fft_size, 1024U);
    EXPECT_EQ(first.window_coefficients_digest_sha256,
              "4906ede244241938adc967b93ef9d0502baea192dd78abbb644436c61000467c");
    EXPECT_EQ(first.parameters_digest_sha256,
              "3c7493a42ae78009be222d29754e6a11ca8eb0e234307dde0a8bfe73d33c22f8");
    EXPECT_EQ(space_rhythm::audio::production_parameters(44'100).parameters_digest_sha256,
              "c76b32e4cd30fdbc0f540235ce97401248b9d6cea5fbb9dc5985062c88b10a82");
    EXPECT_EQ(space_rhythm::audio::algorithm_version, "1.0.0");
    EXPECT_EQ(space_rhythm::audio::fft_backend_version,
              "131.2.0-vcpkg.port+cmake.131.1.0");
}

TEST(AudioAnalysisOracle, FixedBeatProducesSampleExactStableCandidates)
{
    SCOPED_TRACE("AUDIO-ORACLE-AV-FIXED-BEAT-120-001");
    const auto result = analyze_fixture(kFixedBeat);
    ASSERT_EQ(result.status, AnalysisStatus::success);
    EXPECT_EQ(candidate_indices(result, CandidateKind::onset),
              (std::vector<std::int64_t>{0, 24'000, 48'000, 72'000}));
    EXPECT_EQ(candidate_indices(result, CandidateKind::beat),
              (std::vector<std::int64_t>{0, 24'000, 48'000, 72'000}));
    EXPECT_EQ(beat_tempos(result),
              (std::vector<std::uint32_t>{120'000, 120'000, 120'000, 120'000}));
    ASSERT_TRUE(result.overall_confidence_ppm.has_value());
    EXPECT_EQ(*result.overall_confidence_ppm, 1'000'000U);
    ASSERT_FALSE(result.feature_frames.empty());
    EXPECT_FALSE(result.feature_frames.front().spectral_change.valid);
    EXPECT_EQ(result.feature_frames.front().spectral_change.invalid_reason,
              "no_previous_frame");
}

TEST(AudioAnalysisOracle, TempoChangePreservesOrderedLocalTempo)
{
    SCOPED_TRACE("AUDIO-ORACLE-AV-TEMPO-CHANGE-001");
    const auto result = analyze_fixture(kTempoChange);
    ASSERT_EQ(result.status, AnalysisStatus::success);
    EXPECT_EQ(candidate_indices(result, CandidateKind::beat),
              (std::vector<std::int64_t>{0,
                                         24'000,
                                         48'000,
                                         64'000,
                                         80'000,
                                         96'000,
                                         108'000,
                                         120'000,
                                         132'000}));
    EXPECT_EQ(beat_tempos(result),
              (std::vector<std::uint32_t>{120'000,
                                          120'000,
                                          120'000,
                                          180'000,
                                          180'000,
                                          180'000,
                                          240'000,
                                          240'000,
                                          240'000}));
    ASSERT_TRUE(result.overall_confidence_ppm.has_value());
    EXPECT_EQ(*result.overall_confidence_ppm, 850'000U);
}

TEST(AudioAnalysisOracle, ImpulseEnergyAndCandidateTimesAreDeterministic)
{
    SCOPED_TRACE("AUDIO-ORACLE-AV-IMPULSE-001");
    const auto first = analyze_fixture(kImpulse);
    const auto second = analyze_fixture(kImpulse);
    EXPECT_EQ(first.status, AnalysisStatus::success);
    EXPECT_EQ(first.feature_frames, second.feature_frames);
    EXPECT_EQ(first.candidates, second.candidates);
    EXPECT_EQ(candidate_indices(first, CandidateKind::onset),
              (std::vector<std::int64_t>{0, 24'000, 47'999}));
    ASSERT_FALSE(first.feature_frames.empty());
    EXPECT_NEAR(static_cast<double>(first.feature_frames.front().short_time_energy.mantissa),
                549'316'406.0,
                1.0);
    ASSERT_FALSE(first.candidates.empty());
    for (std::size_t index = 1; index < first.candidates.size(); ++index) {
        const auto& previous = first.candidates[index - 1U];
        const auto& current = first.candidates[index];
        const auto previous_key =
            std::tuple{previous.time_ns,
                       std::string{space_rhythm::audio::to_string(previous.kind)},
                       previous.id.value};
        const auto current_key =
            std::tuple{current.time_ns,
                       std::string{space_rhythm::audio::to_string(current.kind)},
                       current.id.value};
        EXPECT_LE(previous_key, current_key);
    }
}

TEST(AudioAnalysisOracle, SilenceAt44100IsNoSignal)
{
    SCOPED_TRACE("AUDIO-ORACLE-AV-SILENCE-44100-001");
    const auto result = analyze_fixture(kSilence);
    EXPECT_EQ(result.status, AnalysisStatus::no_signal);
    EXPECT_EQ(result.reason_codes, (std::vector<std::string>{"silence"}));
    EXPECT_TRUE(result.candidates.empty());
    ASSERT_FALSE(result.feature_frames.empty());
    EXPECT_EQ(result.feature_frames.front().short_time_energy.mantissa, 0);
}

TEST(AudioAnalysisOracle, DeterministicNoiseDoesNotInventBeat)
{
    SCOPED_TRACE("AUDIO-ORACLE-AV-NOISE-001");
    const auto result = analyze_fixture(kNoise);
    EXPECT_EQ(result.status, AnalysisStatus::low_confidence);
    EXPECT_NE(std::ranges::find(result.reason_codes, "noisy"), result.reason_codes.end());
    EXPECT_TRUE(candidate_indices(result, CandidateKind::beat).empty());
}

TEST(AudioAnalysisOracle, StereoBoundarySignalDoesNotCrossProjectOrigin)
{
    SCOPED_TRACE("AUDIO-ORACLE-AV-BOUNDARY-001");
    const auto result = analyze_fixture(kBoundary);
    EXPECT_EQ(result.status, AnalysisStatus::low_confidence);
    EXPECT_NE(std::ranges::find(result.reason_codes, "insufficient_duration"),
              result.reason_codes.end());
    ASSERT_FALSE(result.candidates.empty());
    EXPECT_LT(result.candidates.front().time_ns, 0);
    EXPECT_EQ(result.feature_frames.front().schema_version, 1U);
    EXPECT_EQ(result.feature_frames.front().feature_schema_version, 1U);
    EXPECT_EQ(result.feature_frames.front().channel_aggregation, "mean_energy");
    EXPECT_EQ(result.candidates.front().schema_version, 1U);
    EXPECT_EQ(result.candidates.front().candidate_schema_version, 1U);
    EXPECT_EQ(result.candidates.front().duration_ns, 0);
}

TEST(AudioAnalysisBehavior, RejectsUnrepresentableSegmentEnd)
{
    auto buffer = adapt_fixture(kBoundary);
    buffer.first_sample_index = std::numeric_limits<std::int64_t>::max();
    buffer.segment_origin_sample_index = std::numeric_limits<std::int64_t>::max();
    buffer.valid_frame_count = 1U;
    const auto result = space_rhythm::audio::Analyzer{}.analyze(
        std::span<const DspPcmBuffer>{&buffer, 1},
        space_rhythm::audio::production_parameters(48'000));
    EXPECT_EQ(result.status, AnalysisStatus::failed);
    ASSERT_TRUE(result.error.has_value());
    EXPECT_EQ(result.error->code, ErrorCode::time_overflow);
}

TEST(AudioAnalysisOracle, NonFinitePcmFailsWithoutPartialResult)
{
    SCOPED_TRACE("AUDIO-ORACLE-AV-NONFINITE-001");
    const auto result = analyze_fixture(kNonFinite);
    EXPECT_EQ(result.status, AnalysisStatus::failed);
    ASSERT_TRUE(result.error.has_value());
    EXPECT_EQ(result.error->code, ErrorCode::non_finite_pcm);
    EXPECT_TRUE(result.feature_frames.empty());
    EXPECT_TRUE(result.candidates.empty());
}

TEST(AudioAnalysisBehavior, FreeRhythmAndWeakTransientsRemainLowConfidence)
{
    constexpr auto free_hash =
        "a5e138387e61b220e59d6d2f21fc6d2faec05825d03c2654acec7fbbb7df2d7b";
    constexpr auto weak_hash =
        "c45be13bdf92fc1fd1af188cc0776bb4058cec130db622492265ad6523ef9b4e";
    std::vector<float> free_rhythm(80'000, 0.0F);
    for (const auto index : {4'000U, 13'000U, 27'000U, 46'000U, 71'000U}) {
        free_rhythm[index] = 0.75F;
    }
    const auto free_buffer = adapt_samples(free_rhythm, 48'000, 1, free_hash, "free-rhythm");
    const auto free_result = space_rhythm::audio::Analyzer{}.analyze(
        std::span<const DspPcmBuffer>{&free_buffer, 1},
        space_rhythm::audio::production_parameters(48'000));
    EXPECT_EQ(free_result.status, AnalysisStatus::low_confidence);
    EXPECT_NE(std::ranges::find(free_result.reason_codes, "aperiodic"),
              free_result.reason_codes.end());
    EXPECT_TRUE(candidate_indices(free_result, CandidateKind::beat).empty());

    std::vector<float> weak(48'000, 0.0F);
    for (const auto index : {0U, 12'000U, 24'000U, 36'000U}) {
        weak[index] = 0.05F;
    }
    const auto weak_buffer = adapt_samples(weak, 48'000, 1, weak_hash, "weak-transients");
    const auto weak_result = space_rhythm::audio::Analyzer{}.analyze(
        std::span<const DspPcmBuffer>{&weak_buffer, 1},
        space_rhythm::audio::production_parameters(48'000));
    EXPECT_EQ(weak_result.status, AnalysisStatus::low_confidence);
    EXPECT_EQ(weak_result.reason_codes, (std::vector<std::string>{"weak_transients"}));
    EXPECT_TRUE(weak_result.candidates.empty());
}

TEST(AudioAnalysisBehavior, SegmentBoundaryPreventsCrossSegmentBeatInference)
{
    constexpr auto hash =
        "539bbe9ded8c74d1b29d3a4dc873f1cd2094dbd3ee0b6849481b87e052ef3cb9";
    std::vector<float> first_samples(12'000, 0.0F);
    std::vector<float> second_samples(12'000, 0.0F);
    first_samples[1'000] = 0.75F;
    first_samples[8'000] = 0.75F;
    second_samples[1'000] = 0.75F;
    second_samples[8'000] = 0.75F;
    const auto first = adapt_samples(first_samples, 48'000, 1, hash, "segment-a");
    const auto second = adapt_samples(second_samples, 48'000, 1, hash, "segment-b", 12'000);
    const std::array buffers{first, second};
    const auto result = space_rhythm::audio::Analyzer{}.analyze(
        buffers, space_rhythm::audio::production_parameters(48'000));
    EXPECT_EQ(result.status, AnalysisStatus::low_confidence);
    EXPECT_TRUE(candidate_indices(result, CandidateKind::beat).empty());
    EXPECT_EQ(candidate_indices(result, CandidateKind::onset).size(), 4U);
}

TEST(AudioAnalysisBehavior, CancellationAndResourceLimitsAreTerminal)
{
    const auto buffer = adapt_fixture(kFixedBeat);
    const auto parameters = space_rhythm::audio::production_parameters(48'000);
    space_rhythm::core::CancellationToken cancellation;
    cancellation.cancel();
    const auto cancelled = space_rhythm::audio::Analyzer{}.analyze(
        std::span<const DspPcmBuffer>{&buffer, 1}, parameters, {}, &cancellation);
    EXPECT_EQ(cancelled.status, AnalysisStatus::cancelled);
    EXPECT_TRUE(cancelled.candidates.empty());
    ASSERT_TRUE(cancelled.error.has_value());
    EXPECT_EQ(cancelled.error->code, ErrorCode::cancelled);

    space_rhythm::audio::AnalysisLimits limits;
    limits.max_input_frames = 1;
    const auto limited = space_rhythm::audio::Analyzer{}.analyze(
        std::span<const DspPcmBuffer>{&buffer, 1}, parameters, limits);
    EXPECT_EQ(limited.status, AnalysisStatus::failed);
    ASSERT_TRUE(limited.error.has_value());
    EXPECT_EQ(limited.error->category, space_rhythm::core::ErrorCategory::resource_limit);
    EXPECT_EQ(limited.error->code, ErrorCode::resource_limit);
}

} // namespace
